/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 of the License.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Taken from sdr-j-fm, with bug fixes and enhancements.
 *
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */
#include "fm-demodulator.h"

// Just to play around a little, I implemented 5 common
// fm decoders. The main source of inspiration is found in
// a Diploma Thesis "Implementation of FM demodulator Algorithms
// on a High Performance Digital Signal Processor", especially
// chapter 3.
fmDemodulator::fmDemodulator(int32_t rateIn,
    trigTabs* fastTrigTabs,
    DSPFLOAT K_FM) {
    int32_t i;

    this->rateIn = rateIn;
    this->fastTrigTabs = fastTrigTabs;
    this->K_FM = 2 * K_FM;

    this->selectedDecoder = FM4DECODER;
    this->max_freq_deviation = 0.95 * (0.5 * rateIn);
    myfm_pll = new pll(rateIn,
        0,
        -max_freq_deviation,
        +max_freq_deviation,
        0.85 * rateIn,
        fastTrigTabs);
    ArcsineSize = 4 * 8192;
    Arcsine = new DSPFLOAT[ArcsineSize];
    for (i = 0; i < ArcsineSize; i++)
        Arcsine[i] = asin(2.0 * i / ArcsineSize - 1.0) / 2.0;
    Imin1 = 0.2;
    Qmin1 = 0.2;
    Imin2 = 0.2;
    Qmin2 = 0.2;
    fm_afc = 0;
    fm_cvt = 1.0;
    // fm_cvt = 0.50 * (rateIn / (M_PI * 150000));
}

fmDemodulator::~fmDemodulator(void) {
    delete [] Arcsine;
    delete myfm_pll;
}

void fmDemodulator::setDecoder(int8_t nc) {
    this->selectedDecoder = nc;
}

const char* fmDemodulator::nameOfDecoder(void) {
    switch (selectedDecoder) {
    default:
    case FM1DECODER:
        return "Difference based";
    case FM2DECODER:
        return "Complex Baseband Delay";
    case FM3DECODER:
        return "Mixed Demodulator";
    case FM4DECODER:
        return "Pll decoder";
    case FM5DECODER:
        return "Real Baseband Delay";
    }
}

#define DCAlpha 0.0001

DSPFLOAT fmDemodulator::demodulate(DSPCOMPLEX z) {
    DSPFLOAT res;
    DSPFLOAT I, Q;

    if (abs(z) <= 0.001)
        I = Q = 0.001; // do not make these 0 too often
    else {
        I = real(z) / abs(z);
        Q = imag(z) / abs(z);
    }

    z = DSPCOMPLEX(I, Q);
    switch (selectedDecoder) {
    default:
    case FM1DECODER:
        res = Imin1 * (Q - Qmin2) - Qmin1 * (I - Imin2);
        res /= Imin1 * Imin1 + Qmin1 * Qmin1;
        Imin2 = Imin1;
        Qmin2 = Qmin1;
        fm_afc = (1 - DCAlpha) * fm_afc + DCAlpha * res;
        res = (res - fm_afc) * fm_cvt;
        res /= K_FM;
        break;

    case FM2DECODER:
        res = fastTrigTabs->argX(z * DSPCOMPLEX(Imin1, -Qmin1));
        fm_afc = (1 - DCAlpha) * fm_afc + DCAlpha * res;
        res = (res - fm_afc) * fm_cvt;
        res /= K_FM;
        break;

    case FM3DECODER:
        res = fastTrigTabs->atan2(Q * Imin1 - I * Qmin1,
            I * Imin1 + Q * Qmin1);
        fm_afc = (1 - DCAlpha) * fm_afc + DCAlpha * res;
        res = (res - fm_afc) * fm_cvt;
        res /= K_FM;
        break;

    case FM4DECODER:
        myfm_pll->doPll(z);

        // lowpass the NCO frequency term to get a DC offset
        fm_afc = (1 - DCAlpha) * fm_afc + DCAlpha * myfm_pll->getPhaseIncr();
        res = (myfm_pll->getPhaseIncr() - fm_afc) * fm_cvt;
        res /= K_FM;
        break;

    case FM5DECODER:
        res = (Imin1 * Q - Qmin1 * I + 1.0) / 2.0;
        res = Arcsine[(int)(res * ArcsineSize)];
        fm_afc = (1 - DCAlpha) * fm_afc + DCAlpha * res;
        res = (res - fm_afc) * fm_cvt;
        res /= K_FM;
        break;
    }

    //	and shift ...
    Imin1 = I;
    Qmin1 = Q;
    return res;
}

DSPFLOAT fmDemodulator::get_DcComponent(void) {
    return fm_afc;
}

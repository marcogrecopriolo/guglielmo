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
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef IIR_FILTER_H
#define IIR_FILTER_H

#include "constants.h"
#include "fft.h"
#include "math-helper.h"

#define S_CHEBYSHEV 0100
#define S_BUTTERWORTH 0101
#define S_INV_CHEBYSHEV 0102
#define S_ELLIPTIC 0103

struct biquad {
    DSPFLOAT A0, A1, A2;
    DSPFLOAT B0, B1, B2;
};

typedef struct biquad element;

/*
 * Processing an element, i.e. passing data, is the same for all
 * recursive filters.
 * We write it out for the components of the complex data
 * profiling shows that that is much, much faster
 */
class Basic_IIR {
public:
    int16_t numofQuads;
    element* Quads;
    DSPFLOAT gain;
    DSPCOMPLEX* m1;
    DSPCOMPLEX* m2;
    Basic_IIR(int16_t n) {
        int16_t i;
        numofQuads = n;

        Quads = new element[2 * numofQuads];
        m1 = new DSPCOMPLEX[2 * numofQuads];
        m2 = new DSPCOMPLEX[2 * numofQuads];
        for (i = 0; i < numofQuads; i++) {
            m1[i] = 0;
            m2[i] = 0;
        }
    }
    ~Basic_IIR(void) {
        delete[] Quads;
        delete[] m1;
        delete[] m2;
    }

    DSPCOMPLEX Pass(DSPCOMPLEX z) {
        DSPCOMPLEX o, w;
        int16_t i;

        o = cmul(z, gain);
        for (i = 0; i < numofQuads; i++) {
            element* MyQuad = &Quads[i];
            w = o - cmul(m1[i], MyQuad->B1) - cmul(m2[i], MyQuad->B2);
            o = w + cmul(m1[i], MyQuad->A1) + cmul(m2[i], MyQuad->A2);
            m2[i] = m1[i];
            m1[i] = w;
        }

        return o;
    }

    DSPFLOAT Pass(DSPFLOAT v) {
        DSPFLOAT o, w;
        int16_t i;

        o = v * gain;
        for (i = 0; i < numofQuads; i++) {
            element* MyQuad = &Quads[i];
            DSPFLOAT rm1 = real(m1[i]);
            DSPFLOAT rm2 = real(m2[i]);
            w = o - rm1 * MyQuad->B1 - rm2 * MyQuad->B2;
            o = w + rm1 * MyQuad->A1 + rm2 * MyQuad->A2;
            m2[i] = m1[i];
            m1[i] = w;
        }

        return o;
    }
};

class LowPassIIR : public Basic_IIR {
public:
    LowPassIIR(int16_t, int32_t, int32_t, int16_t);
    LowPassIIR(int16_t, int16_t, int32_t, int32_t, int32_t);
    ~LowPassIIR(void);
};

class HighPassIIR : public Basic_IIR {
public:
    HighPassIIR(int16_t, int32_t, int32_t, int16_t);
    ~HighPassIIR(void);
};

class BandPassIIR : public Basic_IIR {
public:
    BandPassIIR(int16_t,
        int32_t, int32_t, int32_t,
        int16_t);
    ~BandPassIIR(void);
};
#endif

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

#ifndef FM_DEMODULATOR_H
#define FM_DEMODULATOR_H

#include "constants.h"
#include "pll.h"
#include "trigtabs.h"

#define PLL_PILOT_GAIN 3000

class fmDemodulator {
public:
    enum fm_demod {
        FM1DECODER = 0001,
        FM2DECODER = 0002,
        FM3DECODER = 0003,
        FM4DECODER = 0004,
        FM5DECODER = 0005,
        FM6DECODER = 0006
    };

private:
    int8_t selectedDecoder;
    DSPFLOAT max_freq_deviation;
    int32_t rateIn;
    DSPFLOAT fm_afc;
    DSPFLOAT fm_cvt;
    DSPFLOAT K_FM;
    pll* myfm_pll;
    trigTabs* fastTrigTabs;
    int32_t ArcsineSize;
    DSPFLOAT* Arcsine;
    DSPFLOAT Imin1;
    DSPFLOAT Qmin1;
    DSPFLOAT Imin2;
    DSPFLOAT Qmin2;

public:
    fmDemodulator(int32_t Rate_in,
        trigTabs* fastTrigTabs,
        DSPFLOAT K_FM);
    ~fmDemodulator(void);

    void setDecoder(int8_t);
    const char* nameOfDecoder(void);
    DSPFLOAT demodulate(DSPCOMPLEX);
    DSPFLOAT get_DcComponent(void);
};
#endif

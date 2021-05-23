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
#ifndef	__PLL_H
#define	__PLL_H

#include "constants.h"
#include "trigtabs.h"

class pll {
public:
    pll(int32_t rate, DSPFLOAT freq, DSPFLOAT lofreq, DSPFLOAT hifreq,
        DSPFLOAT bandwidth, trigTabs *table);

    ~pll(void);

    DSPFLOAT doPll(DSPCOMPLEX signal);
    DSPFLOAT doPll(DSPCOMPLEX signal, DSPFLOAT phase);
    DSPFLOAT getPhaseIncr(void);
    DSPFLOAT getNcoPhase(void);
    void reset(void);

private:
    int32_t rate;
    int32_t cf;
    DSPFLOAT ncoPhase;
    DSPFLOAT phaseIncr;
    DSPFLOAT ncoHLimit;
    DSPFLOAT ncoLLimit;
    DSPFLOAT beta;
    trigTabs *fastTrigTabs;
};
#endif	/* __PLL_H */

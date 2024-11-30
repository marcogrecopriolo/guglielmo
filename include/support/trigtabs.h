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
 *    Lazy Chair Programming
 *
 *	This LUT implementation of atan2 is a C++ translation of
 *	a Java discussion on the net
 *	http://www.java-gaming.org/index.php?topic=14647.0
 */

#ifndef _TRIGTABS_H
#define _TRIGTABS_H

#include "constants.h"
#include "math-helper.h"
#include <cstdlib>
#include <limits>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

DSPFLOAT toBaseRadians(DSPFLOAT phase);

class trigTabs {
public:
    trigTabs(int32_t);
    ~trigTabs(void);
    DSPFLOAT getSin(DSPFLOAT);
    DSPFLOAT getCos(DSPFLOAT);
    float atan2(float, float);
    float argX(DSPCOMPLEX);

private:
    int32_t fromPhasetoIndex(DSPFLOAT);
    DSPCOMPLEX* table;
    int32_t rate;
    double C;
    float* ATAN2_TABLE_PPY;
    float* ATAN2_TABLE_PPX;
    float* ATAN2_TABLE_PNY;
    float* ATAN2_TABLE_PNX;
    float* ATAN2_TABLE_NPY;
    float* ATAN2_TABLE_NPX;
    float* ATAN2_TABLE_NNY;
    float* ATAN2_TABLE_NNX;
    float stretch;
};

#endif

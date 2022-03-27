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
 *
 *	This LUT implementation of atan2 is a C++ translation of
 *	a Java discussion on the net
 *	http://www.java-gaming.org/index.php?topic=14647.0
 */

// SIZE conflicts with a mingw32 header file
#define	TABSIZE		8192
#define	NEGSIZE		(-TABSIZE)

#include	"trigtabs.h"

float toBaseRadians(DSPFLOAT phase) {
    DSPFLOAT cycles;

    if (phase>=0 && phase<2*M_PI)
	return phase;
    cycles = phase/(2*M_PI);
    if (phase<0)
	return 2*M_PI+(cycles-int(cycles))*2*M_PI;
    else 
        return (cycles-int(cycles))*2*M_PI;
}

trigTabs::trigTabs (int32_t rate) {
    int32_t i;

   this->rate = rate;
   this->table = new DSPCOMPLEX[rate];
   for (i=0; i<rate; i++) 
	table [i] = DSPCOMPLEX(cos(2*M_PI*i/rate), sin(2*M_PI*i/rate));
   this->C = rate/(2*M_PI);
   stretch = M_PI;

    ATAN2_TABLE_PPY = new float[TABSIZE+1];
    ATAN2_TABLE_PPX = new float[TABSIZE+1];
    ATAN2_TABLE_PNY = new float[TABSIZE+1];
    ATAN2_TABLE_PNX = new float[TABSIZE+1];
    ATAN2_TABLE_NPY = new float[TABSIZE+1];
    ATAN2_TABLE_NPX = new float[TABSIZE+1];
    ATAN2_TABLE_NNY = new float[TABSIZE+1];
    ATAN2_TABLE_NNX = new float[TABSIZE+1];
    for (int i=0; i<=TABSIZE; i++) {
	float f = (float) i/TABSIZE;

	ATAN2_TABLE_PPY[i] = atan(f)*stretch/M_PI;
	ATAN2_TABLE_PPX[i] = stretch*0.5f-ATAN2_TABLE_PPY[i];
	ATAN2_TABLE_PNY[i] = -ATAN2_TABLE_PPY[i];
	ATAN2_TABLE_PNX[i] = ATAN2_TABLE_PPY[i]-stretch*0.5f;
	ATAN2_TABLE_NPY[i] = stretch-ATAN2_TABLE_PPY[i];
	ATAN2_TABLE_NPX[i] = ATAN2_TABLE_PPY[i]+stretch*0.5f;
	ATAN2_TABLE_NNY[i] = ATAN2_TABLE_PPY[i]-stretch;
	ATAN2_TABLE_NNX[i] = -stretch*0.5f-ATAN2_TABLE_PPY[i];
    }
}

trigTabs::~trigTabs (void) {
    delete []table;
    delete ATAN2_TABLE_PPY;
    delete ATAN2_TABLE_PPX;
    delete ATAN2_TABLE_PNX;
    delete ATAN2_TABLE_PNY;
    delete ATAN2_TABLE_NPY;
    delete ATAN2_TABLE_NPX;
    delete ATAN2_TABLE_NNY;
    delete ATAN2_TABLE_NNX;
}

int32_t	trigTabs::fromPhasetoIndex (DSPFLOAT phase) {	
    if (phase >= 0)
	return (int32_t(phase*C))%rate;
    else
	return rate-(int32_t(phase*C))%rate;
}

DSPFLOAT trigTabs::getSin(DSPFLOAT phase) {
    if (phase<0)
	return -getSin(-phase);
    return imag(table[fromPhasetoIndex(phase)]);
}

DSPFLOAT trigTabs::getCos(DSPFLOAT phase) {
    if (phase>=0)
	return real(table[(int32_t(phase*C))%rate]);
    else
	return real(table[rate-(int32_t(-phase*C))% rate]);
}

DSPFLOAT trigTabs::atan2(float y, float x) {

    if (isinf (x) || isinf (y))
	return 0;
    if (isnan (x) || isnan (y))
	return 0;
    if (isnan (-x) || isnan (-y))
	return 0;
    if (x==0) {
	if (y==0) 
	    return 0;
	else if (y>0)
	    return M_PI/2;
	else
	    return -M_PI/2;
    }

    if (x>0) {
	if (y>=0) {
	    if (x>=y) 
		return ATAN2_TABLE_PPY[(int)(TABSIZE*y/x+0.5)];
	    else
		return ATAN2_TABLE_PPX[(int)(TABSIZE*x/y+0.5)];
	      
	} else {
	      if (x>=-y) 
	         return ATAN2_TABLE_PNY[(int)(NEGSIZE*y/x+0.5)];
	      else
	         return ATAN2_TABLE_PNX[(int)(NEGSIZE*x/y+0.5)];
	}
    } else {
	if (y>=0) {
	    if (-x>=y) 
		return ATAN2_TABLE_NPY[(int)(NEGSIZE*y/x+0.5)];
	    else
		return ATAN2_TABLE_NPX[(int)(NEGSIZE*x/y+0.5)];
	} else {
	    if (x<=y)
		return ATAN2_TABLE_NNY[(int)(TABSIZE*y/x+0.5)];
	    else
		return ATAN2_TABLE_NNX[(int)(TABSIZE*x/y+0.5)];
	}
    }
}

float	trigTabs::argX	(DSPCOMPLEX v) {
	return this -> atan2 (imag (v), real (v));
}

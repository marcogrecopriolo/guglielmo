/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of sdr-j-fm
 *
 *    sdr-j-fm is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    sdr-j-fm is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdr-j-fm; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	This LUT implementation of atan2 is a C++ translation of
 *	a Java discussion on the net
 *	http://www.java-gaming.org/index.php?topic=14647.0
 */

#define	SIZE		8192
#define	EZIS		(-SIZE)

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

	trigTabs::trigTabs (int32_t Rate) {
int32_t	i;
	   this	-> Rate		= Rate;
	   this	-> Table	= new DSPCOMPLEX [Rate];
	   for (i = 0; i < Rate; i ++) 
	      Table [i] = DSPCOMPLEX (cos (2 * M_PI * i / Rate),
	                              sin (2 * M_PI * i / Rate));
	   this	->	C	= Rate / (2 * M_PI);
	Stretch		= M_PI;
// Output will swing from -Stretch to Stretch (default: Math.PI)
// Useful to change to 1 if you would normally do "atan2(y, x) / Math.PI"

	ATAN2_TABLE_PPY    = new float [SIZE + 1];
	ATAN2_TABLE_PPX    = new float [SIZE + 1];
	ATAN2_TABLE_PNY    = new float [SIZE + 1];
	ATAN2_TABLE_PNX    = new float [SIZE + 1];
	ATAN2_TABLE_NPY    = new float [SIZE + 1];
	ATAN2_TABLE_NPX    = new float [SIZE + 1];
	ATAN2_TABLE_NNY    = new float [SIZE + 1];
	ATAN2_TABLE_NNX    = new float [SIZE + 1];
        for (int i = 0; i <= SIZE; i++) {
            float f = (float)i / SIZE;
            ATAN2_TABLE_PPY [i] = atan(f) * Stretch / M_PI;
            ATAN2_TABLE_PPX [i] = Stretch * 0.5f - ATAN2_TABLE_PPY[i];
            ATAN2_TABLE_PNY [i] = -ATAN2_TABLE_PPY [i];
            ATAN2_TABLE_PNX [i] = ATAN2_TABLE_PPY [i] - Stretch * 0.5f;
            ATAN2_TABLE_NPY [i] = Stretch - ATAN2_TABLE_PPY [i];
            ATAN2_TABLE_NPX [i] = ATAN2_TABLE_PPY [i] + Stretch * 0.5f;
            ATAN2_TABLE_NNY [i] = ATAN2_TABLE_PPY [i] - Stretch;
            ATAN2_TABLE_NNX [i] = -Stretch * 0.5f - ATAN2_TABLE_PPY [i];
        }
}

	trigTabs::~trigTabs (void) {
	delete [] Table;
	delete	ATAN2_TABLE_PPY;
	delete	ATAN2_TABLE_PPX;
	delete	ATAN2_TABLE_PNX;
	delete	ATAN2_TABLE_PNY;
	delete	ATAN2_TABLE_NPY;
	delete	ATAN2_TABLE_NPX;
	delete	ATAN2_TABLE_NNY;
	delete	ATAN2_TABLE_NNX;
}
//	Heavy code: executed millions of times
//	we get all kinds of very strange values here, so
//	testing over the whole domain is needed
int32_t	trigTabs::fromPhasetoIndex (DSPFLOAT Phase) {	
	if (Phase >= 0)
	   return (int32_t (Phase * C)) % Rate;
	else
	   return Rate - (int32_t (Phase * C)) % Rate;
/*
	if (0 <= Phase && Phase < 2 * M_PI)
	   return Phase / (2 * M_PI) * Rate;

	if (Phase >= 2 * M_PI)
//	   return fmod (Phase, 2 * M_PI) / (2 * M_PI) * Rate;
	   return (int32_t (Phase / (2 * M_PI) * Rate)) % Rate;

	if (Phase >= -2 * M_PI) {
	   Phase = -Phase;
	   return Rate - Phase / (2 * M_PI) * Rate;
	}

	Phase = -Phase;
	return Rate - fmod (Phase, 2 * M_PI) / (2 * M_PI) * Rate;
*/
}

DSPFLOAT	trigTabs::getSin (DSPFLOAT Phase) {
	if (Phase < 0)
	   return -getSin (- Phase);
	return imag (Table [fromPhasetoIndex (Phase)]);
}

DSPFLOAT	trigTabs::getCos (DSPFLOAT Phase) {
	if (Phase >= 0)
	   return real (Table [(int32_t (Phase * C)) % Rate]);
	else
	   return real (Table [Rate - (int32_t ( - Phase * C)) % Rate]);
/*
	if (Phase < 0)
	   Phase = -Phase;
	return real (Table [fromPhasetoIndex (Phase)]);
*/
}

DSPCOMPLEX	trigTabs::getComplex (DSPFLOAT Phase) {
	if (Phase >= 0)
	   return Table [(int32_t (Phase * C)) % Rate];
	else
	   return Table [Rate - (int32_t ( - Phase * C)) % Rate];
//	return Table [fromPhasetoIndex (Phase)];
}


/**
  * ATAN2 : performance degrades due to the many "0" tests
  */

float	trigTabs::atan2 (float y, float x) {

	if (isinf (x) || isinf (y)) return 0;
	if (isnan (x) || isnan (y)) return 0;
	if (isnan (-x) || isnan (-y)) return 0;
	if (x == 0) {
	   if (y == 0)  return 0;
//	      return std::numeric_limits<float>::infinity ();
	   else
	   if (y > 0)
	      return  M_PI / 2;
	   else		// y < 0
	      return  - M_PI / 2;
	}

	if (x > 0) {
	   if (y >= 0) {
	      if (x >= y) 
	         return ATAN2_TABLE_PPY[(int)(SIZE * y / x + 0.5)];
	      else
	         return ATAN2_TABLE_PPX[(int)(SIZE * x / y + 0.5)];
	      
	   }
	   else {
	      if (x >= -y) 
	         return ATAN2_TABLE_PNY[(int)(EZIS * y / x + 0.5)];
	      else
	         return ATAN2_TABLE_PNX[(int)(EZIS * x / y + 0.5)];
	   }
        }
	else {
	   if (y >= 0) {
	      if (-x >= y) 
	         return ATAN2_TABLE_NPY[(int)(EZIS * y / x + 0.5)];
	      else
	         return ATAN2_TABLE_NPX[(int)(EZIS * x / y + 0.5)];
	   }
	   else {
	      if (x <= y) // (-x >= -y)
	         return ATAN2_TABLE_NNY[(int)(SIZE * y / x + 0.5)];
	      else
	         return ATAN2_TABLE_NNX[(int)(SIZE * x / y + 0.5)];
	   }
	}
}

float	trigTabs::argX	(DSPCOMPLEX v) {
	return this -> atan2 (imag (v), real (v));
}

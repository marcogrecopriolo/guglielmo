#
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
 */

#include	"sincos.h"
//
//	As it turns out, when using DAB sticks, this simple function is the
//	real CPU burner, with a usage of up to 22 %

	SinCos::SinCos (DSPCOMPLEX *Table, int32_t Rate) {
           this	-> Table	= Table;
	   this	-> localTable	= false;
	   this	-> Rate		= Rate;
	   this	->	C	= Rate / (2 * M_PI);
}

	SinCos::SinCos (int32_t Rate) {
int32_t	i;
	   this	-> Rate		= Rate;
	   this	-> localTable	= true;
	   this	-> Table	= new DSPCOMPLEX [Rate];
	   for (i = 0; i < Rate; i ++) 
	      Table [i] = DSPCOMPLEX (cos (2 * M_PI * i / Rate),
	                              sin (2 * M_PI * i / Rate));
	   this	->	C	= Rate / (2 * M_PI);
}

	SinCos::~SinCos (void) {
	   if (localTable)
	      delete [] Table;
}
//	Heavy code: executed millions of times
//	we get all kinds of very strange values here, so
//	testing over the whole domain is needed
int32_t	SinCos::fromPhasetoIndex (DSPFLOAT Phase) {	
	if (Phase >= 0)
	   return (int32_t (Phase * C)) % Rate;
	else
	   return Rate - (int32_t (Phase * C)) % Rate;
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
}

DSPFLOAT	SinCos::getSin (DSPFLOAT Phase) {
	if (Phase < 0)
	   return -getSin (- Phase);
	return imag (Table [fromPhasetoIndex (Phase)]);
}

DSPFLOAT	SinCos::getCos (DSPFLOAT Phase) {
	if (Phase >= 0)
	   return real (Table [(int32_t (Phase * C)) % Rate]);
	else
	   return real (Table [Rate - (int32_t ( - Phase * C)) % Rate]);
	if (Phase < 0)
	   Phase = -Phase;
	return real (Table [fromPhasetoIndex (Phase)]);
}

DSPCOMPLEX	SinCos::getComplex (DSPFLOAT Phase) {
	if (Phase >= 0)
	   return Table [(int32_t (Phase * C)) % Rate];
	else
	   return Table [Rate - (int32_t ( - Phase * C)) % Rate];
	return Table [fromPhasetoIndex (Phase)];
}



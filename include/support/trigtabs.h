/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    Many of the ideas as implemented in JSDR are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	This LUT implementation of atan2 is a C++ translation of
 *	a Java discussion on the net
 *	http://www.java-gaming.org/index.php?topic=14647.0
 */

#ifndef _SINCOS_H
#define _SINCOS_H

#include	"constants.h"
#include	<math.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<cstdlib>
#include	<limits>

DSPFLOAT toBaseRadians(DSPFLOAT phase);

class	trigTabs {
public:
			trigTabs	(int32_t);
			~trigTabs	(void);
	DSPFLOAT	getSin		(DSPFLOAT);
	DSPFLOAT	getCos		(DSPFLOAT);
	DSPCOMPLEX	getComplex	(DSPFLOAT);
	float	atan2		(float, float);
	float	argX		(DSPCOMPLEX);
	
private:
	DSPCOMPLEX	*Table;
	int32_t		Rate;
	bool		localTable;
	double		C;
	float	*ATAN2_TABLE_PPY;
	float	*ATAN2_TABLE_PPX;
	float	*ATAN2_TABLE_PNY;
	float	*ATAN2_TABLE_PNX;
	float	*ATAN2_TABLE_NPY;
	float	*ATAN2_TABLE_NPX;
	float	*ATAN2_TABLE_NNY;
	float	*ATAN2_TABLE_NNX;
	float	Stretch;
	int32_t		fromPhasetoIndex	(DSPFLOAT);
};

#endif

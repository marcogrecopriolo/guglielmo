#
/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sdr-j-fm
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

#include	"oscillator.h"

	Oscillator::Oscillator (int32_t rate) {
int32_t	i;
	Rate			= rate;
	OscillatorValues	= new DSPCOMPLEX [rate];
	for (i = 0; i < rate; i ++)
	   OscillatorValues [i] = DSPCOMPLEX (cos (2.0 * M_PI * i / rate),
	                                      sin (2.0 * M_PI * i / rate));
	LOPhase			= 0;
	localTable		= true;
}

	Oscillator::Oscillator (DSPCOMPLEX *Table, int32_t rate) {
	Rate			= rate;
	OscillatorValues	= Table;
	LOPhase			= 0;
	localTable		= false;
}

	Oscillator::~Oscillator () {
	if (localTable)
	   delete[]	OscillatorValues;
}

DSPCOMPLEX	Oscillator::nextValue (int32_t step) {
	LOPhase -= step;
	if (LOPhase < 0)
	   LOPhase += Rate;
	else
	if (LOPhase >= Rate)
	   LOPhase -= Rate;

	return OscillatorValues [LOPhase];
}


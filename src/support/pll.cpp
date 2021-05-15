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
 *   	This pll was found to give reasonable results.
 *	source DTTSP, all rights acknowledged
 */
#include	"pll.h"
//
//	rate	is the samplerate
//	lofreq and hifreq the frequencies (in Hz) where the lock is 
//	kept in between,
//	bandwidth the bandwidth of the signal to be received

		pll::pll (int32_t	rate,
	                    DSPFLOAT	freq,
	                    DSPFLOAT	lofreq, DSPFLOAT hifreq,
	                    DSPFLOAT	bandwidth,
	                    SinCos	*Table) {
DSPFLOAT	fac	= 2.0 * M_PI / rate;

	this	-> rate	= rate;
	this	-> cf	= freq;
//	for the control lowpass filter
	Beta		= exp (- 2.0 * M_PI * bandwidth / 2 / rate);
	NcoPhase	= 0;
	phaseIncr	= freq * fac ;		// this will change during runs
	NcoLLimit	= lofreq * fac;		// boundary for changes
	NcoHLimit	= hifreq * fac;
	this	-> mySinCos	= Table;
}

		pll::~pll (void) {
}

void	pll::reset (void) {
	NcoPhase        = 0;
        phaseIncr       = cf * 2.0 * M_PI / rate ;
}
//
//	It turned out that under Fedora we had from time
//	to time an infinite value for signal. Still have
//	to constrain this value
DSPFLOAT	pll::doPll (DSPCOMPLEX signal) {
DSPCOMPLEX	NcoSignal = DSPCOMPLEX(mySinCos -> getCos(NcoPhase), mySinCos -> getSin(NcoPhase)); 
DSPCOMPLEX	pll_Delay	= NcoSignal * signal;
//
//	we use a pretty fast atan here
	DSPFLOAT phaseError	= - myAtan. atan2 (imag (pll_Delay), real (pll_Delay));
//	... and a pretty simple filter
	phaseIncr	= (1 - Beta) * phaseError + Beta * phaseIncr;
	if (phaseIncr < NcoLLimit || phaseIncr > NcoHLimit)
	   phaseIncr	= cf * 2 * M_PI / rate;

	NcoPhase	+= phaseIncr;
	if (NcoPhase >= 2 * M_PI)
	   NcoPhase = fmod (NcoPhase, 2 * M_PI);
	else
	while (NcoPhase < 0)
	   NcoPhase += 2 * M_PI;
	return imag(NcoSignal);
}

DSPFLOAT	pll::doPll (DSPCOMPLEX signal, DSPFLOAT phase) {
DSPCOMPLEX	NcoSignal	= DSPCOMPLEX(mySinCos -> getCos(NcoPhase), mySinCos -> getSin(phase)); 
DSPCOMPLEX	pll_Delay	= NcoSignal * signal;
DSPFLOAT	ret		= mySinCos -> getSin(NcoPhase);

//	we use a pretty fast atan here
	DSPFLOAT phaseError	= - myAtan. atan2 (imag(pll_Delay), real(pll_Delay));
//	... and a pretty simple filter
	phaseIncr	= (1 - Beta) * phaseError + Beta * phaseIncr;
	if (phaseIncr < NcoLLimit || phaseIncr > NcoHLimit)
	   phaseIncr	= cf * 2 * M_PI / rate;

	NcoPhase	+= phaseIncr;
	if (NcoPhase >= 2 * M_PI)
	   NcoPhase = fmod (NcoPhase, 2 * M_PI);
	else
	while (NcoPhase < 0)
	   NcoPhase += 2 * M_PI;
	return ret;
}

DSPFLOAT	pll::getPhaseIncr(void) {
	return phaseIncr;
}

DSPFLOAT	pll::getNcoPhase (void) {
	return NcoPhase;
}

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
#include "pll.h"

pll::pll(int32_t rate, DSPFLOAT freq,
	DSPFLOAT lofreq, DSPFLOAT hifreq,
	DSPFLOAT bandwidth, trigTabs *table) {
    DSPFLOAT fac = 2.0*M_PI/rate;

    this->rate = rate;
    cf = freq;
    beta = exp(-2.0*M_PI*bandwidth/2/rate);
    ncoPhase = 0;
    phaseIncr = freq*fac;
    ncoLLimit = lofreq*fac;
    ncoHLimit = hifreq*fac;
    fastTrigTabs = table;
}

pll::~pll(void) {
}

void pll::reset (void) {
    ncoPhase = 0;
    phaseIncr = cf*2.0*M_PI/rate ;
}

DSPFLOAT pll::doPll(DSPCOMPLEX signal) {
    DSPCOMPLEX	ncoSignal = DSPCOMPLEX(fastTrigTabs->getCos(ncoPhase), fastTrigTabs->getSin(ncoPhase)); 
    DSPCOMPLEX	pllDelay = ncoSignal*signal;

    DSPFLOAT phaseError	= -fastTrigTabs->atan2(imag(pllDelay), real(pllDelay));
    phaseIncr	= (1-beta)*phaseError+beta*phaseIncr;
    if (phaseIncr<ncoLLimit || phaseIncr>ncoHLimit)
	phaseIncr = cf*2*M_PI/rate;

    ncoPhase = toBaseRadians(ncoPhase+phaseIncr);
    return imag(ncoSignal);
}

DSPFLOAT pll::doPll(DSPCOMPLEX signal, DSPFLOAT phase) {
    DSPCOMPLEX ncoSignal = DSPCOMPLEX(fastTrigTabs->getCos(ncoPhase), fastTrigTabs->getSin(phase)); 
    DSPCOMPLEX pllDelay = ncoSignal*signal;
    DSPFLOAT ret = fastTrigTabs->getSin(ncoPhase);

    DSPFLOAT phaseError	= -fastTrigTabs->atan2(imag(pllDelay), real(pllDelay));
    phaseIncr = (1-beta) * phaseError+beta*phaseIncr;
    if (phaseIncr<ncoLLimit || phaseIncr>ncoHLimit)
	   phaseIncr = cf*2*M_PI/rate;

    ncoPhase = toBaseRadians(ncoPhase+phaseIncr);
    return ret;
}

DSPFLOAT pll::getPhaseIncr(void) {
    return phaseIncr;
}

DSPFLOAT pll::getNcoPhase(void) {
    return ncoPhase;
}

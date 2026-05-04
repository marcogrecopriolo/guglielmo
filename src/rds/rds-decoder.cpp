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
#include "rds-decoder.h"
#include "iir-filters.h"
#include "radio.h"
#include "trigtabs.h"
#include <stdio.h>
#include <stdlib.h>

const DSPFLOAT RDS_BITCLK_HZ = 1187.5;

/*
 * RDS is a bpsk-like signal, with a baudrate 1187.5
 * on a carrier of  3 * 19 k.
 * 48 cycles per bit, 1187.5 bits per second.
 * With a reduced sample rate of 48K this would mean
 * 48000 / 1187.5 samples per bit, i.e. between 40 and 41
 * samples per bit.
 * Notice that mixing to zero IF has been done
 */
rdsDecoder::rdsDecoder(RadioInterface* radioInterface,
    bool partialText,
    int32_t rate,
    trigTabs* fastTrigTabs) {
    DSPFLOAT synchronizerSamples;
    int16_t i;
    int16_t length;

    this->sampleRate = rate;
    this->fastTrigTabs = fastTrigTabs;
    omegaRDS = (2 * M_PI * RDS_BITCLK_HZ) / (DSPFLOAT)rate;

    // for the decoder a la FMStack we need:
    synchronizerSamples = sampleRate / (DSPFLOAT)RDS_BITCLK_HZ;
    symbolCeiling = ceil(synchronizerSamples);
    symbolFloor = floor(synchronizerSamples);
    syncBuffer = new DSPFLOAT[symbolCeiling];
    memset(syncBuffer, 0, symbolCeiling * sizeof(DSPFLOAT));
    inputIndex = 0;
    bitIntegrator = 0;
    bitClkPhase = 0;
    prevClkState = 0;

    // The matched filter is a borrowed from the cuteRDS, who in turn
    // borrowed it from course material
    //      http://courses.engr.illinois.edu/ece463/Projects/RBDS/RBDS_project.doc
    // Note that the formula down has a discontinuity for
    // two values of x, we better make the symbollength odd
    length = (symbolCeiling & ~01) + 1;
    rdsFilterSize = 2 * length + 1;
    rdsBuffer = new DSPFLOAT[rdsFilterSize];
    memset(rdsBuffer, 0, rdsFilterSize * sizeof(DSPFLOAT));
    matchIndex = 0;
    rdsKernel = new DSPFLOAT[rdsFilterSize];
    rdsKernel[length] = 0;
    for (i = 1; i <= length; i++) {
	DSPFLOAT x = ((DSPFLOAT)i) / rate * RDS_BITCLK_HZ;
	rdsKernel[length+i] = 0.75*cos(4*M_PI*x)*((1.0/(1.0/x-64*x))-((1.0/(9.0/x-64*x))));
	rdsKernel[length-i] = -0.75*cos(4*M_PI*x)*((1.0/(1.0/x-64*x))-((1.0/(9.0/x-64*x))));
    }

    // The matched filter is followed by a pretty sharp filter
    // to eliminate all remaining "noise".
    sharpFilter = new BandPassIIR(7, RDS_BITCLK_HZ-6, RDS_BITCLK_HZ+6, rate, S_BUTTERWORTH);
    rdsLastSyncSlope = 0;
    rdsLastSync = 0;
    previousBit = false;

    memset(sampleWindow, 0, sizeof(sampleWindow));
    sampleWindowIndex = 0;

    phaseAcc = 0;
    period = sampleRate / RDS_BITCLK_HZ;

    gardnerSample = 0;

    rdsGroup = new RDSGroup();
    rdsGroup->clear();
    blockSynchroniser = new rdsBlockSynchronizer(radioInterface);
    blockSynchroniser->setFecEnabled(true);
    groupDecoder = new rdsGroupDecoder(radioInterface, partialText);
}

rdsDecoder::~rdsDecoder(void) {
    delete [] syncBuffer;
    delete groupDecoder;
    delete rdsGroup;
    delete blockSynchroniser;
    delete [] rdsKernel;
    delete [] rdsBuffer;
    delete sharpFilter;
}

void rdsDecoder::reset(void) {
    groupDecoder->reset();
}

void rdsDecoder::setPartialText(bool p) {
    groupDecoder->setPartialText(p);
}

DSPFLOAT rdsDecoder::match(DSPFLOAT v) {
    int16_t i;
    DSPFLOAT tmp = 0;

    rdsBuffer[matchIndex] = v;
    for (i = 0; i<rdsFilterSize; i++) {
	int16_t index = (matchIndex-i);
	if (index<0)
	    index += rdsFilterSize;
	tmp += rdsBuffer[index]*rdsKernel[i];
    }

    matchIndex = (matchIndex+1)%rdsFilterSize;
    return tmp;
}

/*
 * Signal (i.e. "v") is already downconverted and lowpass filtered
 * when entering this stage.
 */
void rdsDecoder::doDecode(DSPFLOAT v, RdsMode mode) {
    if (mode == NO_RDS)
	return; // should not happen

    if (mode == RDS1)
	doDecode1(v);
    else
	doDecode2(v);
}

void rdsDecoder::doDecode1(DSPFLOAT v) {
    DSPFLOAT rdsMag;
    DSPFLOAT rdsSlope = 0;
    bool bit;

    v = match(v);

    sampleWindow[sampleWindowIndex] = v;
    sampleWindowIndex = (sampleWindowIndex + 1) % 3;

    rdsMag = sharpFilter->Pass(v * v);
    rdsSlope = rdsMag - rdsLastSync;
    rdsLastSync = rdsMag;

    // Continuous phase correction
    // The peak of rdsMag marks the bit boundary. The slope zero crossing
    // tells us when the peak occurred relative to our phase accumulator.
    // If the peak arrives early, are we running slow? nudge period down.
    // If the peak arrives late, are we running fast? nudge period up.
    if ((rdsSlope < 0.0) && (rdsLastSyncSlope >= 0.0)) {
	DSPFLOAT phaseError = phaseAcc - period;
	DSPFLOAT nominalPeriod = sampleRate / RDS_BITCLK_HZ;

	period -= 0.01 * phaseError;
	if (period < nominalPeriod * 0.95)
	    period = nominalPeriod * 0.95;
	if (period > nominalPeriod * 1.05)
	    period = nominalPeriod * 1.05;
	phaseAcc = 0;

	int votes = 0;
	for (int i = 0; i < 3; i++)
	    votes += (sampleWindow[i] >= 0)? 1: 0;
	bit = votes >= 2;

	processBit(bit ^ previousBit);
	previousBit = bit;
    }

    phaseAcc += 1.0;
    rdsLastSyncSlope = rdsSlope;
}

void rdsDecoder::doDecode2(DSPFLOAT v) {
    DSPFLOAT clkState;

    syncBuffer[inputIndex] = v;
    inputIndex = (inputIndex+1)%symbolCeiling;
    v = match(v);

    if ((blockSynchroniser->reSynchronise())) {
	synchronizeOnBitClk(syncBuffer, inputIndex);
	blockSynchroniser->resync();
	blockSynchroniser->resetResyncErrorCounter();
    }

    clkState = fastTrigTabs->getSin(bitClkPhase);
    bitIntegrator += v*clkState;

    sampleWindow[sampleWindowIndex] = v;
    sampleWindowIndex = (sampleWindowIndex + 1) % 3;

    // Capture the half-symbol sample for Gardner TED (improvement 2):
    // The Gardner timing error detector needs the sample at the midpoint
    // between bit boundaries, i.e. at the falling edge of clkState.
    if (prevClkState >= 0 && clkState < 0)
	gardnerSample = v;

    // rising edge -> look at integrator
    if (prevClkState <= 0 && clkState > 0) {
	// Base bit decision from integrator
	bool currentBit = bitIntegrator >= 0;

	int votes = 0;
	for (int i = 0; i < 3; i++)
	    votes += (sampleWindow[i] >= 0)? 1: 0;
	bool votedBit = votes >= 2;

	// If integrator and majority vote agree, use integrator result.
	// If they disagree, use the majority vote as a tiebreaker.
	if (currentBit != votedBit)
	    currentBit = votedBit;

	// Gardner timing error detector
	// e = x[n-0.5] * (x[n] - x[n-1])
	// where x[n] is the current bit sample, x[n-1] the previous,
	// and x[n-0.5] the half-symbol sample captured at the falling edge.
	// This is more robust than the early-late gate under low SNR.
	DSPFLOAT gardnerError = gardnerSample *
	    ((currentBit? 1.0: -1.0) - (previousBit? 1.0: -1.0));
	bitClkPhase += 0.005 * gardnerError;

	bitClkPhase = fmod(bitClkPhase, 2*M_PI);
	if (bitClkPhase < 0)
	    bitClkPhase += 2*M_PI;

	processBit(currentBit^previousBit);
	bitIntegrator = 0;
	previousBit = currentBit;
    }

    prevClkState = clkState;
    bitClkPhase = fmod(bitClkPhase+omegaRDS, 2*M_PI);
}

void rdsDecoder::processBit(bool bit) {
    switch (blockSynchroniser->pushBit(bit, rdsGroup)) {
    case rdsBlockSynchronizer::RDS_WAITING_FOR_BLOCK_A:
	break; // still waiting in block A

    case rdsBlockSynchronizer::RDS_BUFFERING:
	break; // just buffer

    case rdsBlockSynchronizer::RDS_NO_SYNC:

	// resync if the last sync failed
	blockSynchroniser->resync();
	break;

    case rdsBlockSynchronizer::RDS_NO_CRC:
	blockSynchroniser->resync();
	break;

    case rdsBlockSynchronizer::RDS_COMPLETE_GROUP:
	if (!groupDecoder->decode(rdsGroup)) {
	    ; // error decoding the rds group
	}

	rdsGroup->clear();
	break;
    }
}

void rdsDecoder::synchronizeOnBitClk(DSPFLOAT* v, int16_t first) {
    bool isHigh = false;
    int32_t k = 0;
    int32_t i;
    DSPFLOAT phase;
    DSPFLOAT* correlationVector = (DSPFLOAT*)alloca(symbolCeiling * sizeof(DSPFLOAT));

    memset(correlationVector, 0, symbolCeiling * sizeof(DSPFLOAT));

    // synchronizerSamples = sampleRate / (DSPFLOAT)RDS_BITCLK_HZ;
    for (i = 0; i < symbolCeiling; i++) {
	phase = fmod(i * (omegaRDS / 2), 2 * M_PI);

	// reset index on phase change
	if (fastTrigTabs->getSin(phase) > 0 && !isHigh) {
	    isHigh = true;
	    k = 0;
	} else if (fastTrigTabs->getSin(phase) < 0 && isHigh) {
	    isHigh = false;
	    k = 0;
	}

	correlationVector[k++] += v[(first + i) % symbolCeiling];
    }

    // detect rising edge in correlation window
    int32_t iMin = 0;
    while (iMin < symbolFloor && correlationVector[iMin++] > 0)
	;
    while (iMin < symbolFloor && correlationVector[iMin++] < 0)
	;

    // set the phase, previous sample (iMin - 1) is obviously the one
    bitClkPhase = fmod(-omegaRDS * (iMin - 1), 2 * M_PI);
    while (bitClkPhase < 0)
	bitClkPhase += 2 * M_PI;
}

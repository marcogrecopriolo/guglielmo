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
rdsDecoder::rdsDecoder(RadioInterface* myRadio,
    int32_t rate,
    trigTabs* fastTrigTabs) {
    DSPFLOAT synchronizerSamples;
    int16_t i;
    int16_t length;

    this->MyRadioInterface = myRadio;
    this->sampleRate = rate;
    this->fastTrigTabs = fastTrigTabs;
    omegaRDS = (2 * M_PI * RDS_BITCLK_HZ) / (DSPFLOAT)rate;

    // for the decoder a la FMStack we need:
    synchronizerSamples = sampleRate / (DSPFLOAT)RDS_BITCLK_HZ;
    symbolCeiling = ceil(synchronizerSamples);
    symbolFloor = floor(synchronizerSamples);
    syncBuffer = new DSPFLOAT[symbolCeiling];
    memset(syncBuffer, 0, symbolCeiling * sizeof(DSPFLOAT));
    p = 0;
    bitIntegrator = 0;
    bitClkPhase = 0;
    prev_clkState = 0;
    prevBit = 0;
    Resync = true;

    // The matched filter is a borrowed from the cuteRDS, who in turn
    // borrowed it from course material
    //      http://courses.engr.illinois.edu/ece463/Projects/RBDS/RBDS_project.doc
    // Note that the formula down has a discontinuity for
    // two values of x, we better make the symbollength odd
    length = (symbolCeiling & ~01) + 1;
    rdsfilterSize = 2 * length + 1;
    rdsBuffer = new DSPFLOAT[rdsfilterSize];
    memset(rdsBuffer, 0, rdsfilterSize * sizeof(DSPFLOAT));
    ip = 0;
    rdsKernel = new DSPFLOAT[rdsfilterSize];
    rdsKernel[length] = 0;
    for (i = 1; i <= length; i++) {
        DSPFLOAT x = ((DSPFLOAT)i) / rate * RDS_BITCLK_HZ;
        rdsKernel[length + i] = 0.75 * cos(4 * M_PI * x) * ((1.0 / (1.0 / x - 64 * x)) - ((1.0 / (9.0 / x - 64 * x))));
        rdsKernel[length - i] = -0.75 * cos(4 * M_PI * x) * ((1.0 / (1.0 / x - 64 * x)) - ((1.0 / (9.0 / x - 64 * x))));
    }

    // The matched filter is followed by a pretty sharp filter
    // to eliminate all remaining "noise".
    sharpFilter = new BandPassIIR(9, RDS_BITCLK_HZ - 6,
        RDS_BITCLK_HZ + 6,
        rate, S_BUTTERWORTH);
    rdsLastSyncSlope = 0;
    rdsLastSync = 0;
    rdsLastData = 0;
    rdsPrevSync = 0;
    rdsPrevData = 0;
    previousBit = false;

    my_rdsGroup = new RDSGroup();
    my_rdsGroup->clear();
    my_rdsBlockSync = new rdsBlockSynchronizer(MyRadioInterface);
    my_rdsBlockSync->setFecEnabled(true);
    my_rdsGroupDecoder = new rdsGroupDecoder(MyRadioInterface);
}

rdsDecoder::~rdsDecoder(void) {
    delete[] syncBuffer;
    delete my_rdsGroupDecoder;
    delete my_rdsGroup;
    delete my_rdsBlockSync;
    delete rdsKernel;
    delete rdsBuffer;
    delete sharpFilter;
}

void rdsDecoder::reset(void) {
    my_rdsGroupDecoder->reset();
}

DSPFLOAT rdsDecoder::Match(DSPFLOAT v) {
    int16_t i;
    DSPFLOAT tmp = 0;

    rdsBuffer[ip] = v;
    for (i = 0; i < rdsfilterSize; i++) {
        int16_t index = (ip - i);
        if (index < 0)
            index += rdsfilterSize;
        tmp += rdsBuffer[index] * rdsKernel[i];
    }

    ip = (ip + 1) % rdsfilterSize;
    return tmp;
}

/*
 * Signal (i.e. "v") is already downconverted and lowpass filtered
 * when entering this stage. The return value stored in "*m" is used
 * to display things to the user
 */
void rdsDecoder::doDecode(DSPFLOAT v, DSPFLOAT* m, RdsMode mode) {
    if (mode == NO_RDS)
        return; // should not happen

    if (mode == RDS1)
        doDecode1(v, m);
    else
        doDecode2(v, m);
}

void rdsDecoder::doDecode1(DSPFLOAT v, DSPFLOAT* m) {
    DSPFLOAT rdsMag;
    DSPFLOAT rdsSlope = 0;
    bool bit, prevBit, thisBit;

    v = Match(v);
    rdsMag = sharpFilter->Pass(v * v);
    *m = (20 * rdsMag + 1.0);
    rdsSlope = rdsMag - rdsLastSync;
    rdsPrevSync = rdsLastSync;
    rdsLastSync = rdsMag;
    if ((rdsSlope < 0.0) && (rdsLastSyncSlope >= 0.0)) {

        // top of the sine wave: get the data
        bit = rdsLastData >= 0;

        // vote on the bit using the previous and next sample
        thisBit = v >= 0;
        prevBit = rdsPrevData >= 0;
        if (bit != prevBit && bit != thisBit)
            bit = prevBit;

        processBit(bit ^ previousBit);
        previousBit = bit;
    }

    rdsPrevData = rdsLastData;
    rdsLastData = v;
    rdsLastSyncSlope = rdsSlope;
    my_rdsBlockSync->resetResyncErrorCounter();
}

void rdsDecoder::doDecode2(DSPFLOAT v, DSPFLOAT* mag) {
    DSPFLOAT clkState;

    syncBuffer[p] = v;
    *mag = syncBuffer[p] + 1;
    p = (p + 1) % symbolCeiling;
    v = syncBuffer[p];			// an old one

    if (Resync || (my_rdsBlockSync->getNumSyncErrors() > 3)) {
        synchronizeOnBitClk(syncBuffer, p);
        my_rdsBlockSync->resync();
        my_rdsBlockSync->resetResyncErrorCounter();
        Resync = false;
    }

    clkState = fastTrigTabs->getSin(bitClkPhase);
    bitIntegrator += v * clkState;

    // rising edge -> look at integrator
    if (prev_clkState <= 0 && clkState > 0) {
        bool currentBit = bitIntegrator >= 0;
        processBit(currentBit ^ previousBit);
        bitIntegrator = 0;		// we start all over
        previousBit = currentBit;
    }

    prev_clkState = clkState;
    bitClkPhase = fmod(bitClkPhase + omegaRDS, 2 * M_PI);
}

void rdsDecoder::processBit(bool bit) {
    switch (my_rdsBlockSync->pushBit(bit, my_rdsGroup)) {
    case rdsBlockSynchronizer::RDS_WAITING_FOR_BLOCK_A:
        break; // still waiting in block A

    case rdsBlockSynchronizer::RDS_BUFFERING:
        break; // just buffer

    case rdsBlockSynchronizer::RDS_NO_SYNC:

        // resync if the last sync failed
        setSyncErrors(my_rdsBlockSync->getNumSyncErrors());
        my_rdsBlockSync->resync();
        break;

    case rdsBlockSynchronizer::RDS_NO_CRC:
        setCRCErrors(my_rdsBlockSync->getNumCRCErrors());
        my_rdsBlockSync->resync();
        break;

    case rdsBlockSynchronizer::RDS_COMPLETE_GROUP:
        if (!my_rdsGroupDecoder->decode(my_rdsGroup)) {
            ; // error decoding the rds group
        }

        my_rdsGroup->clear();
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

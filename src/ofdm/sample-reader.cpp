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
 *    Taken from Qt-DAB, with bug fixes and enhancements.
 *
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */
#include "sample-reader.h"
#include "radio.h"

static inline int16_t valueFor(int16_t b) {
    int16_t res = 1;
    while (--b > 0)
        res <<= 1;
    return res;
}

static std::complex<float> oscillatorTable[INPUT_RATE];

sampleReader::sampleReader(RadioInterface *mr, deviceHandler *theRig) {
    int i;
    this->theRig = theRig;
    this->myRadioInterface = mr;
    bufferSize = 32768;
    localBuffer.resize(bufferSize);
    localCounter = 0;
    currentPhase = 0;
    sLevel = 0;
    sampleCount = 0;
    for (i = 0; i < INPUT_RATE; i++)
        oscillatorTable[i] = std::complex<float>(
            cos(2.0 * M_PI * i / INPUT_RATE), sin(2.0 * M_PI * i / INPUT_RATE));

    bufferContent = 0;
    corrector = 0;
    dumpfilePointer.store(nullptr);
    dumpIndex = 0;
    dumpScale = valueFor(theRig->bitDepth());
    running.store(true);
}

sampleReader::~sampleReader() {}

void sampleReader::reset(void) {
    localCounter = 0;
    currentPhase = 0;
    sLevel = 0;
    sampleCount = 0;
    bufferContent = 0;
    corrector = 0;
    dumpfilePointer.store(nullptr);
    dumpIndex = 0;
    dumpScale = valueFor(theRig->bitDepth());
}

void sampleReader::setRunning(bool b) { running.store(b); }

float sampleReader::get_sLevel() { return sLevel; }

std::complex<float> sampleReader::getSample(int32_t phaseOffset) {
    std::complex<float> temp;

    corrector = phaseOffset;
    if (!running.load())
        throw 21;

    ///	bufferContent is an indicator for the value of ... -> Samples()
    if (bufferContent == 0) {
        bufferContent = theRig->Samples();
        while ((bufferContent <= 2048) && running.load()) {
            usleep(10);
            bufferContent = theRig->Samples();
        }
    }

    if (!running.load())
        throw 20;

    //	so here, bufferContent > 0
    agcStats stats;
    int32_t n = theRig->getSamples(&temp, 1, &stats);
    myRadioInterface->processGain(&stats, n);
    bufferContent--;
    if (dumpfilePointer.load() != nullptr) {
        dumpBuffer[2 * dumpIndex] = real(temp) * dumpScale;
        dumpBuffer[2 * dumpIndex + 1] = imag(temp) * dumpScale;
        if (++dumpIndex >= DUMPSIZE / 2) {
            sf_writef_short(dumpfilePointer.load(), dumpBuffer, dumpIndex);
            dumpIndex = 0;
        }
    }

    if (localCounter < bufferSize)
        localBuffer[localCounter++] = temp;

    //	OK, we have a sample!!
    //	first: adjust frequency. We need Hz accuracy
    currentPhase -= phaseOffset;
    currentPhase = (currentPhase + INPUT_RATE) % INPUT_RATE;

    temp *= oscillatorTable[currentPhase];
    sLevel = 0.00001 * fastMagnitude(temp) + (1 - 0.00001) * sLevel;
#define N 5
    if (++sampleCount > INPUT_RATE / N) {
        emit showCorrector(corrector);
        sampleCount = 0;
        localCounter = 0;
    }
    return temp;
}

void sampleReader::getSamples(std::complex<float> *v, int32_t n,
                              int32_t phaseOffset) {
    int32_t i;

    corrector = phaseOffset;
    if (!running.load())
        throw 21;
    if (n > bufferContent) {
        bufferContent = theRig->Samples();
        while ((bufferContent < n) && running.load()) {
            usleep(10);
            bufferContent = theRig->Samples();
        }
    }

    if (!running.load())
        throw 20;

    //	so here, bufferContent >= n
    agcStats stats;
    n = theRig->getSamples(v, n, &stats);
    myRadioInterface->processGain(&stats, n);
    bufferContent -= n;
    if (dumpfilePointer.load() != nullptr) {
        for (i = 0; i < n; i++) {
            dumpBuffer[2 * dumpIndex] = real(v[i]) * dumpScale;
            dumpBuffer[2 * dumpIndex + 1] = imag(v[i]) * dumpScale;
            if (++dumpIndex >= DUMPSIZE / 2) {
                sf_writef_short(dumpfilePointer.load(), dumpBuffer, dumpIndex);
                dumpIndex = 0;
            }
        }
    }

    //	OK, we have samples!!
    //	first: adjust frequency. We need Hz accuracy
    for (i = 0; i < n; i++) {
        currentPhase -= phaseOffset;

        //	Note that "phase" itself might be negative
        currentPhase = (currentPhase + INPUT_RATE) % INPUT_RATE;
        if (localCounter < bufferSize)
            localBuffer[localCounter++] = v[i];
        v[i] *= oscillatorTable[currentPhase];
        sLevel = 0.00001 * fastMagnitude(v[i]) + (1 - 0.00001) * sLevel;
    }

    sampleCount += n;
    if (sampleCount > INPUT_RATE / N) {
        emit showCorrector(corrector);
        localCounter = 0;
        sampleCount = 0;
    }
}

void sampleReader::startDumping(SNDFILE *f) { dumpfilePointer.store(f); }

void sampleReader::stopDumping() { dumpfilePointer.store(nullptr); }

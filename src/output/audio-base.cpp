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
 *    Taken from Qt_DAB with bug fixes and enhancements.
 *
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "audio-base.h"
#include "constants.h"
#include <cstdio>

/*
 * The class is the abstract sink for the data generated
 * It will handle the "dumping" though and assume that 
 * someone else will handle the PCM samples, all made
 * on a rate of 48000
 */
audioBase::audioBase():
    converter_16(16000, 48000, 2 * 1600),
    converter_24(24000, 48000, 2 * 2400),
    converter_32(32000, 48000, 2 * 3200) {
    dumpFile = nullptr;
}

int32_t audioBase::putSample(DSPCOMPLEX v) {
    return putSamples(&v, int32_t(1));
}

int32_t audioBase::putSamples(DSPCOMPLEX* V, int32_t amount) {
    float* buffer = (float*)alloca(2 * amount * sizeof(float));
    int32_t i;

    for (i = 0; i < amount; i++) {
        buffer[2 * i] = real(V[i]);
        buffer[2 * i + 1] = imag(V[i]);
    }

    myLocker.lock();
    if (dumpFile != nullptr)
        sf_writef_float(dumpFile, (float*)buffer, amount);
    myLocker.unlock();
    audioOutput(buffer, amount);
    return amount;
}

// this one is a hack for handling different baudrates coming from
// the aac decoder. call is from the GUI, triggered by the
// aac decoder or the mp3 decoder
void audioBase::audioOut(int16_t* V, int32_t amount, int32_t rate) {

    switch (rate) {
    case 16000:
        audioOut_16000(V, amount / 2);
        return;
    case 24000:
        audioOut_24000(V, amount / 2);
        return;
    case 32000:
        audioOut_32000(V, amount / 2);
        return;
    default:
    case 48000:
        audioOut_48000(V, amount / 2);
        return;
    }
}

// scale up from 16 -> 48
// amount gives number of pairs
void audioBase::audioOut_16000(int16_t* V, int32_t amount) {
    _VLA(std::complex<float>, outputBuffer, converter_16.getOutputsize());
    _VLA(float, buffer, 2 * converter_16.getOutputsize());
    int16_t i, j;
    int32_t result;

    for (i = 0; i < amount; i++)
        if (converter_16.convert(std::complex<float>(V[2 * i] / 32767.0,
                                     V[2 * i + 1] / 32767.0),
                outputBuffer, &result)) {
            for (j = 0; j < result; j++) {
                buffer[2 * j] = real(outputBuffer[j]);
                buffer[2 * j + 1] = imag(outputBuffer[j]);
            }

            myLocker.lock();
            if (dumpFile != nullptr)
                sf_writef_float(dumpFile, (float*)buffer, result);
            myLocker.unlock();

            audioOutput(buffer, result);
        }
}

// scale up from 24000 -> 48000
// amount gives number of pairs
void audioBase::audioOut_24000(int16_t* V, int32_t amount) {
    _VLA(std::complex<float>, outputBuffer, converter_24.getOutputsize());
    _VLA(float, buffer, 2 * converter_24.getOutputsize());
    int16_t i, j;
    int32_t result;

    for (i = 0; i < amount; i++)
        if (converter_24.convert(std::complex<float>(V[2 * i] / 32767.0,
                                     V[2 * i + 1] / 32767.0),
                outputBuffer, &result)) {
            for (j = 0; j < result; j++) {
                buffer[2 * j] = real(outputBuffer[j]);
                buffer[2 * j + 1] = imag(outputBuffer[j]);
            }

            myLocker.lock();
            if (dumpFile != nullptr)
                sf_writef_float(dumpFile, (float*)buffer, result);
            myLocker.unlock();

            audioOutput(buffer, result);
        }
}

// scale up from 32000 -> 48000
// amount is number of pairs
void audioBase::audioOut_32000(int16_t* V, int32_t amount) {
    _VLA(std::complex<float>, outputBuffer, converter_32.getOutputsize());
    _VLA(float, buffer, 2 * converter_32.getOutputsize());
    int32_t i, j;
    int32_t result;

    for (i = 0; i < amount; i++) {
        if (converter_32.convert(std::complex<float>(V[2 * i] / 32767.0,
                                     V[2 * i + 1] / 32767.0),
                outputBuffer, &result)) {
            for (j = 0; j < result; j++) {
                buffer[2 * j] = real(outputBuffer[j]);
                buffer[2 * j + 1] = imag(outputBuffer[j]);
            }

            myLocker.lock();
            if (dumpFile != nullptr)
                sf_writef_float(dumpFile, (float*)buffer, result);
            myLocker.unlock();
            audioOutput(buffer, result);
        }
    }
}

void audioBase::audioOut_48000(int16_t* V, int32_t amount) {
    float* buffer = (float*)alloca(2 * amount * sizeof(float));
    int32_t i;

    for (i = 0; i < amount; i++) {
        buffer[2 * i] = V[2 * i] / 32767.0;
        buffer[2 * i + 1] = V[2 * i + 1] / 32767.0;
    }

    myLocker.lock();
    if (dumpFile != nullptr)
        sf_writef_float(dumpFile, (float*)buffer, amount);
    myLocker.unlock();
    audioOutput(buffer, amount);
}

// we ensure that no one is fiddling with the dumpfile
// while we are writing
void audioBase::startDumping(SNDFILE* f) {
    myLocker.lock();
    dumpFile = f;
    myLocker.unlock();
}

void audioBase::stopDumping() {
    myLocker.lock();
    dumpFile = nullptr;
    myLocker.unlock();
}

// the audioOut function is the one that really should be
// reimplemented in the offsprings of this class
void audioBase::audioOutput(float* v, int32_t amount) {
    (void)v;
    (void)amount;
}

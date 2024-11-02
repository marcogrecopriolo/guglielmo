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
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "audiosink.h"
#include "logging.h"
#include <QComboBox>
#include <cstdio>

audioSink::audioSink(int16_t latency): _O_Buffer(8 * 32768) {
    int32_t i;

    this->latency = latency;
    if (latency <= 0)
        latency = 1;
    volume = 1.0;

    this->CardRate = 48000;
    portAudio = false;
    writerRunning = false;
    if (Pa_Initialize() != paNoError) {
        log(LOG_SOUND, LOG_MIN, "Initializing Pa for output failed");
        return;
    }

    portAudio = true;

    for (i = 0; i < Pa_GetHostApiCount(); i++)
        log(LOG_SOUND, LOG_MIN, "Api %d: %s", i, Pa_GetHostApiInfo(i)->name);

    maxDevices = Pa_GetDeviceCount();
    outTable = new channelList[maxDevices + 1];
    for (i = 0; i < maxDevices; i++) {
        outTable[i].dev = -1;
        outTable[i].name = "";
    }
    ostream = nullptr;
    defDevice = -1;
    setupChannels();
}

audioSink::~audioSink() {
    if ((ostream != nullptr) && !Pa_IsStreamStopped(ostream)) {
        paCallbackReturn = paAbort;
        (void)Pa_AbortStream(ostream);
        while (!Pa_IsStreamStopped(ostream))
            Pa_Sleep(1);
        writerRunning = false;
    }

    if (ostream != nullptr)
        Pa_CloseStream(ostream);

    if (portAudio)
        Pa_Terminate();

    delete[] outTable;
}

void audioSink::setVolume(qreal v) {
    if (v <= 0)
        volume = 0;
    else if (v >= 1)
        volume = 1.0;
    else
        volume = v;
}

bool audioSink::selectDevice(int16_t idx) {
    PaError err;
    int16_t i, outputDevice;

    if ((idx < 0) || (idx >= numDevices)) {
        log(LOG_SOUND, LOG_MIN, "invalid device %d selected", idx);
        return false;
    } else
        outputDevice = outTable[idx].dev;
    if ((ostream != nullptr) && !Pa_IsStreamStopped(ostream)) {
        paCallbackReturn = paAbort;
        (void)Pa_AbortStream(ostream);
        while (!Pa_IsStreamStopped(ostream))
            Pa_Sleep(1);
        writerRunning = false;
    }

    if (ostream != nullptr)
        Pa_CloseStream(ostream);

    outputParameters.device = outputDevice;
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = outTable[idx].latency;
    bufSize = (int)((float)outputParameters.suggestedLatency * latency);

    // A small buffer causes more callback invocations, sometimes
    // causing underflows and intermittent output.
    // buffersize is
    //
    // bufSize	= latency * 512;

    outputParameters.hostApiSpecificStreamInfo = nullptr;

    log(LOG_SOUND, LOG_MIN, "Suggested size for outputbuffer = %d", bufSize);
    err = Pa_OpenStream(&ostream,
        nullptr,
        &outputParameters,
        CardRate,
        bufSize,
        0,
        this->paCallback_o,
        this);

    if (err != paNoError) {
        log(LOG_SOUND, LOG_MIN, "Open ostream error %i", err);
        return false;
    }
    paCallbackReturn = paContinue;
    err = Pa_StartStream(ostream);
    if (err != paNoError) {
        log(LOG_SOUND, LOG_MIN, "Open startstream error %i", err);
        return false;
    }
    log(LOG_SOUND, LOG_MIN, "opened device %i", idx);
    writerRunning = true;
    return true;
}

void audioSink::restart() {
    PaError err;

    if (!Pa_IsStreamStopped(ostream))
        return;

    _O_Buffer.FlushRingBuffer();
    paCallbackReturn = paContinue;
    err = Pa_StartStream(ostream);
    if (err == paNoError)
        writerRunning = true;
}

void audioSink::stop() {
    if (Pa_IsStreamStopped(ostream))
        return;

    paCallbackReturn = paAbort;
    (void)Pa_StopStream(ostream);
    while (!Pa_IsStreamStopped(ostream))
        Pa_Sleep(1);
    writerRunning = false;
}

bool audioSink::OutputrateIsSupported(int16_t device, int32_t Rate) {
    PaStreamParameters* outputParameters = (PaStreamParameters*)alloca(sizeof(PaStreamParameters));

    outputParameters->device = device;
    outputParameters->channelCount = 2; /* I and Q	*/
    outputParameters->sampleFormat = paFloat32;
    outputParameters->suggestedLatency = 0;
    outputParameters->hostApiSpecificStreamInfo = nullptr;

    return Pa_IsFormatSupported(nullptr, outputParameters, Rate) == paFormatIsSupported;
}

static int theMissed = 0;

int audioSink::paCallback_o(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    RingBuffer<float>* outB;
    float* outp = (float*)outputBuffer;
    audioSink* ud = reinterpret_cast<audioSink*>(userData);
    uint32_t actualSize;
    uint32_t i;
    (void)statusFlags;
    (void)inputBuffer;
    (void)timeInfo;
    if (ud->paCallbackReturn == paContinue) {
        outB = &((reinterpret_cast<audioSink*>(userData))->_O_Buffer);
        actualSize = outB->getDataFromBuffer(outp, 2 * framesPerBuffer);
        theMissed += 2 * framesPerBuffer - actualSize;
        for (i = actualSize; i < 2 * framesPerBuffer; i++)
            outp[i] = 0;
    }

    return ud->paCallbackReturn;
}

void audioSink::audioOutput(float* b, int32_t amount) {

    for (int i = 0; i < 2 * amount; i++)
        b[i] *= volume;
    _O_Buffer.putDataIntoBuffer(b, 2 * amount);
}

const char* audioSink::outputChannel(int16_t ch) {
    if ((ch < 0) || (ch >= numDevices))
        return "";
    return outTable[ch].name;
}

int32_t audioSink::cardRate() {
    return 48000;
}

bool audioSink::setupChannels() {
    uint16_t i;
    const PaDeviceInfo* deviceInfo;
    int16_t defDev = Pa_GetDefaultOutputDevice();

    log(LOG_SOUND, LOG_MIN, "setting up %i channels", maxDevices);
    numDevices = 0;
    for (i = 0; i < maxDevices; i++) {
        outTable[i].dev = -1;
        outTable[i].name = "";
    }
    for (i = 0; i < maxDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo == nullptr)
            continue;
        if (deviceInfo->maxOutputChannels <= 0)
            continue;
        if (!OutputrateIsSupported(i, CardRate))
            continue;
        if (i == defDev)
            defDevice = i;
        outTable[numDevices].dev = i;
        outTable[numDevices].name = (char*)deviceInfo->name;
        outTable[numDevices].latency = deviceInfo->defaultHighOutputLatency;
        log(LOG_SOUND, LOG_MIN, "channel %d stream %d (%s)", numDevices, i,
            deviceInfo->name);
        numDevices++;
    }
    return numDevices > 1;
}

int16_t audioSink::numberOfDevices() {
    return numDevices;
}

int16_t audioSink::defaultDevice() {
    return defDevice;
}

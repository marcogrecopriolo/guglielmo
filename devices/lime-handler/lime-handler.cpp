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
 *    Taken from qt-dab, with bug fixes and enhancements.
*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "lime-handler.h"
#include "logging.h"
#define DEV_LIME LOG_DEV

#define FIFO_SIZE 32768
#define MAX_GAIN 73
static int16_t localBuffer[4 * FIFO_SIZE];
lms_info_str_t limedevices[10];

limeHandler::limeHandler()
    : _I_Buffer(4 * 1024 * 1024) {

#if IS_WINDOWS
    const char* libraryString = "LimeSuite.dll";
    Handle = LoadLibraryA(libraryString);
#else
    const char* libraryString = "libLimeSuite" LIBEXT;
    Handle = dlopen(libraryString, RTLD_NOW);
#endif

    if (Handle == nullptr) {
#if IS_WINDOWS
        log(DEV_LIME, LOG_MIN, "failed to open %s - Error = %li", libraryString, GetLastError());
#else
        log(DEV_LIME, LOG_MIN, "failed to open %s - Error = %s", libraryString, dlerror());
#endif
        throw(20);
    }

    libraryLoaded = true;
    if (!load_limeFunctions()) {
        CLOSE_LIBRARY(Handle);
        throw(21);
    }

    log(DEV_LIME, LOG_MIN, "Lime library available");

    int ndevs = LMS_GetDeviceList(limedevices);
    if (ndevs == 0) { // no devices found
        CLOSE_LIBRARY(Handle);
        throw(21);
    }

    for (int i = 0; i < ndevs; i++)
        log(DEV_LIME, LOG_MIN, "device %s", limedevices[i]);

    int res = LMS_Open(&theDevice, nullptr, nullptr);
    if (res < 0) { // some error
        CLOSE_LIBRARY(Handle);
        throw(22);
    }

    res = LMS_Init(theDevice);
    if (res < 0) { // some error
        LMS_Close(&theDevice);
        CLOSE_LIBRARY(Handle);
        throw(23);
    }

    res = LMS_GetNumChannels(theDevice, LMS_CH_RX);
    if (res < 0) { // some error
        LMS_Close(&theDevice);
        CLOSE_LIBRARY(Handle);
        throw(24);
    }

    log(DEV_LIME, LOG_MIN, "device %s supports %d channels",
        limedevices[0], res);
    res = LMS_EnableChannel(theDevice, LMS_CH_RX, 0, true);
    if (res < 0) { // some error
        LMS_Close(theDevice);
        CLOSE_LIBRARY(Handle);
        throw(24);
    }

    res = LMS_SetSampleRate(theDevice, 2048000.0, 0);
    if (res < 0) {
        LMS_Close(theDevice);
        CLOSE_LIBRARY(Handle);
        throw(25);
    }

    float_type host_Hz, rf_Hz;
    res = LMS_GetSampleRate(theDevice, LMS_CH_RX, 0,
        &host_Hz, &rf_Hz);

    log(DEV_LIME, LOG_MIN, "samplerate = %f %f", (float)host_Hz, (float)rf_Hz);

    res = LMS_GetAntennaList(theDevice, LMS_CH_RX, 0, antennas);
    for (int i = 0; i < res; i++) {
        log(DEV_LIME, LOG_MIN, "antenna %s", antennas[i]);
        antennaList << antennas[i];
    }

    int index = antennaList.indexOf("Auto");

    // default antenna setting
    res = LMS_SetAntenna(theDevice, LMS_CH_RX, 0, index + 1);

    // default frequency
    res = LMS_SetLOFrequency(theDevice, LMS_CH_RX,
        0, 220000000.0);
    if (res < 0) {
        LMS_Close(theDevice);
        CLOSE_LIBRARY(Handle);
        throw(26);
    }

    res = LMS_SetLPFBW(theDevice, LMS_CH_RX,
        0, 1536000.0);
    if (res < 0) {
        LMS_Close(theDevice);
        CLOSE_LIBRARY(Handle);
        throw(27);
    }

    LMS_SetGaindB(theDevice, LMS_CH_RX, 0, 50);

    LMS_Calibrate(theDevice, LMS_CH_RX, 0, 2500000.0, 0);

    setIfGain(50);
    running.store(false);
}

limeHandler::~limeHandler() {
    stopReader();
    running.store(false);
    while (isRunning())
        usleep(100);
    LMS_Close(theDevice);
    CLOSE_LIBRARY(Handle);
}

void limeHandler::setIfGain(int g) {
    float_type gg;
    LMS_SetGaindB(theDevice, LMS_CH_RX, 0, g * MAX_GAIN / GAIN_SCALE);
    LMS_GetNormalizedGain(theDevice, LMS_CH_RX, 0, &gg);
}

bool limeHandler::restartReader(int32_t freq) {
    int res;

    if (isRunning())
        return true;
    LMS_SetLOFrequency(theDevice, LMS_CH_RX, 0, freq);
    stream.isTx = false;
    stream.channel = 0;
    stream.fifoSize = FIFO_SIZE;
    stream.throughputVsLatency = 0.1; // ???
    stream.dataFmt = lms_stream_t::LMS_FMT_I12; // 12 bit ints
    res = LMS_SetupStream(theDevice, &stream);
    if (res < 0)
        return false;
    res = LMS_StartStream(&stream);
    if (res < 0)
        return false;

    start();
    return true;
}

void limeHandler::stopReader() {
    if (!isRunning())
        return;
    running.store(false);
    while (isRunning())
        usleep(200);
    (void)LMS_StopStream(&stream);
    (void)LMS_DestroyStream(theDevice, &stream);
}

int limeHandler::getSamples(std::complex<float>* V, int32_t size, agcStats* stats) {
    std::complex<int16_t> temp[size];
    int i;
    int amount = _I_Buffer.getDataFromBuffer(temp, size);
    (void) stats;

    for (i = 0; i < amount; i++)
        V[i] = std::complex<float>(real(temp[i]) / 2048.0,
            imag(temp[i]) / 2048.0);
    return amount;
}

int limeHandler::Samples() {
    return _I_Buffer.GetRingBufferReadAvailable();
}

void limeHandler::resetBuffer() {
    _I_Buffer.FlushRingBuffer();
}

int16_t limeHandler::bitDepth() {
    return 12;
}

void limeHandler::run() {
    int res;
    lms_stream_status_t streamStatus;
    int underruns = 0;
    int overruns = 0;
    int amountRead = 0;

    running.store(true);
    while (running.load()) {
        res = LMS_RecvStream(&stream, localBuffer,
            FIFO_SIZE, &meta, 1000);
        if (res > 0) {
            _I_Buffer.putDataIntoBuffer(localBuffer, res);
            amountRead += res;
            res = LMS_GetStreamStatus(&stream, &streamStatus);
            underruns += streamStatus.underrun;
            overruns += streamStatus.overrun;
        }
        if (amountRead > 4 * 2048000) {
            amountRead = 0;
            underruns = 0;
            overruns = 0;
        }
    }
}

bool limeHandler::load_limeFunctions() {
    this->LMS_GetDeviceList = (pfn_LMS_GetDeviceList)
        GETPROCADDRESS(Handle, "LMS_GetDeviceList");
    if (this->LMS_GetDeviceList == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetdeviceList");
        return false;
    }
    this->LMS_Open = (pfn_LMS_Open)
        GETPROCADDRESS(Handle, "LMS_Open");
    if (this->LMS_Open == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_Open");
        return false;
    }
    this->LMS_Close = (pfn_LMS_Close)
        GETPROCADDRESS(Handle, "LMS_Close");
    if (this->LMS_Close == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_Close");
        return false;
    }
    this->LMS_Init = (pfn_LMS_Init)
        GETPROCADDRESS(Handle, "LMS_Init");
    if (this->LMS_Init == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_Init");
        return false;
    }
    this->LMS_GetNumChannels = (pfn_LMS_GetNumChannels)
        GETPROCADDRESS(Handle, "LMS_GetNumChannels");
    if (this->LMS_GetNumChannels == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetNumChannels");
        return false;
    }
    this->LMS_EnableChannel = (pfn_LMS_EnableChannel)
        GETPROCADDRESS(Handle, "LMS_EnableChannel");
    if (this->LMS_EnableChannel == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_EnableChannel");
        return false;
    }
    this->LMS_SetSampleRate = (pfn_LMS_SetSampleRate)
        GETPROCADDRESS(Handle, "LMS_SetSampleRate");
    if (this->LMS_SetSampleRate == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetSampleRate");
        return false;
    }
    this->LMS_GetSampleRate = (pfn_LMS_GetSampleRate)
        GETPROCADDRESS(Handle, "LMS_GetSampleRate");
    if (this->LMS_GetSampleRate == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetSampleRate");
        return false;
    }
    this->LMS_SetLOFrequency = (pfn_LMS_SetLOFrequency)
        GETPROCADDRESS(Handle, "LMS_SetLOFrequency");
    if (this->LMS_SetLOFrequency == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetLOFrequency");
        return false;
    }
    this->LMS_GetLOFrequency = (pfn_LMS_GetLOFrequency)
        GETPROCADDRESS(Handle, "LMS_GetLOFrequency");
    if (this->LMS_GetLOFrequency == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetLOFrequency");
        return false;
    }
    this->LMS_GetAntennaList = (pfn_LMS_GetAntennaList)
        GETPROCADDRESS(Handle, "LMS_GetAntennaList");
    if (this->LMS_GetAntennaList == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetAntennaList");
        return false;
    }
    this->LMS_SetAntenna = (pfn_LMS_SetAntenna)
        GETPROCADDRESS(Handle, "LMS_SetAntenna");
    if (this->LMS_SetAntenna == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetAntenna");
        return false;
    }
    this->LMS_GetAntenna = (pfn_LMS_GetAntenna)
        GETPROCADDRESS(Handle, "LMS_GetAntenna");
    if (this->LMS_GetAntenna == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetAntenna");
        return false;
    }
    this->LMS_GetAntennaBW = (pfn_LMS_GetAntennaBW)
        GETPROCADDRESS(Handle, "LMS_GetAntennaBW");
    if (this->LMS_GetAntennaBW == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetAntennaBW");
        return false;
    }
    this->LMS_SetNormalizedGain = (pfn_LMS_SetNormalizedGain)
        GETPROCADDRESS(Handle, "LMS_SetNormalizedGain");
    if (this->LMS_SetNormalizedGain == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetNormalizedGain");
        return false;
    }
    this->LMS_GetNormalizedGain = (pfn_LMS_GetNormalizedGain)
        GETPROCADDRESS(Handle, "LMS_GetNormalizedGain");
    if (this->LMS_GetNormalizedGain == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetNormalizedGain");
        return false;
    }
    this->LMS_SetGaindB = (pfn_LMS_SetGaindB)
        GETPROCADDRESS(Handle, "LMS_SetGaindB");
    if (this->LMS_SetGaindB == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetGaindB");
        return false;
    }
    this->LMS_GetGaindB = (pfn_LMS_GetGaindB)
        GETPROCADDRESS(Handle, "LMS_GetGaindB");
    if (this->LMS_GetGaindB == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetGaindB");
        return false;
    }
    this->LMS_SetLPFBW = (pfn_LMS_SetLPFBW)
        GETPROCADDRESS(Handle, "LMS_SetLPFBW");
    if (this->LMS_SetLPFBW == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetLPFBW");
        return false;
    }
    this->LMS_GetLPFBW = (pfn_LMS_GetLPFBW)
        GETPROCADDRESS(Handle, "LMS_GetLPFBW");
    if (this->LMS_GetLPFBW == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetLPFBW");
        return false;
    }
    this->LMS_Calibrate = (pfn_LMS_Calibrate)
        GETPROCADDRESS(Handle, "LMS_Calibrate");
    if (this->LMS_Calibrate == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_Calibrate");
        return false;
    }
    this->LMS_SetupStream = (pfn_LMS_SetupStream)
        GETPROCADDRESS(Handle, "LMS_SetupStream");
    if (this->LMS_SetupStream == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_SetupStream");
        return false;
    }
    this->LMS_DestroyStream = (pfn_LMS_DestroyStream)
        GETPROCADDRESS(Handle, "LMS_DestroyStream");
    if (this->LMS_DestroyStream == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_DestroyStream");
        return false;
    }
    this->LMS_StartStream = (pfn_LMS_StartStream)
        GETPROCADDRESS(Handle, "LMS_StartStream");
    if (this->LMS_StartStream == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_StartStream");
        return false;
    }
    this->LMS_StopStream = (pfn_LMS_StopStream)
        GETPROCADDRESS(Handle, "LMS_StopStream");
    if (this->LMS_StopStream == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_StopStream");
        return false;
    }
    this->LMS_RecvStream = (pfn_LMS_RecvStream)
        GETPROCADDRESS(Handle, "LMS_RecvStream");
    if (this->LMS_RecvStream == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_RecvStream");
        return false;
    }
    this->LMS_GetStreamStatus = (pfn_LMS_GetStreamStatus)
        GETPROCADDRESS(Handle, "LMS_GetStreamStatus");
    if (this->LMS_GetStreamStatus == nullptr) {
        log(DEV_LIME, LOG_MIN, "could not find LMS_GetStreamStatus");
        return false;
    }

    return true;
}

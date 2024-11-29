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
 *    Taken from dab-mini, with bug fixes and enhancements.
 *
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */
#ifndef SDRPLAY_HANDLER_V3_H
#define	SDRPLAY_HANDLER_V3_H

#include <atomic>
#include <stdio.h>
#include "constants.h"
#include "ringbuffer.h"
#include "device-handler.h"
#include <sdrplay_api.h>

class sdrplayHandler_v3: public deviceHandler {
Q_OBJECT
public:
    sdrplayHandler_v3(void);
    ~sdrplayHandler_v3(void);

    int32_t devices(deviceStrings *, int);
    bool setDevice(const char *);
    bool restartReader(int32_t);
    void stopReader(void);
    int32_t getSamples(std::complex<float> *, int32_t, agcStats *stats);
    int32_t Samples(void);
    void resetBuffer(void);
    int16_t bitDepth(void);
    int32_t amplitude(void);

    void update_PowerOverload(sdrplay_api_EventParamsT *params);
    void getIfRange(int *, int *);
    void getLnaRange(int *, int *);
    void setIfGain(int);
    void setLnaGain(int);
    void setAgcControl(int);

    RingBuffer<std::complex<int16_t>> _I_Buffer;
    std::atomic<bool> running;

private:
    bool configDevice(sdrplay_api_DeviceT *);
    void fetchLibrary();
    bool loadFunctions();

    sdrplay_api_DeviceT deviceDesc;
    sdrplay_api_DeviceT *chosenDevice;
    sdrplay_api_DeviceParamsT *deviceParams;
    sdrplay_api_CallbackFnsT cbFns;
    sdrplay_api_RxChannelParamsT *chParams;
    float signalAmplitude;
    int signalMin;
    int signalMax;
    bool agcMode;
    int GRdB;
    int16_t nrBits;
    int	lnaGainMax;
    int lnaState;
    HINSTANCE Handle;

    sdrplay_api_Open_t sdrplay_api_Open;
    sdrplay_api_Close_t sdrplay_api_Close;
    sdrplay_api_ApiVersion_t sdrplay_api_ApiVersion;
    sdrplay_api_LockDeviceApi_t sdrplay_api_LockDeviceApi;
    sdrplay_api_UnlockDeviceApi_t sdrplay_api_UnlockDeviceApi;
    sdrplay_api_GetDevices_t sdrplay_api_GetDevices;
    sdrplay_api_SelectDevice_t sdrplay_api_SelectDevice;
    sdrplay_api_ReleaseDevice_t sdrplay_api_ReleaseDevice;
    sdrplay_api_GetErrorString_t sdrplay_api_GetErrorString;
    sdrplay_api_GetLastError_t sdrplay_api_GetLastError;
    sdrplay_api_DebugEnable_t sdrplay_api_DebugEnable;
    sdrplay_api_GetDeviceParams_t sdrplay_api_GetDeviceParams;
    sdrplay_api_Init_t sdrplay_api_Init;
    sdrplay_api_Uninit_t sdrplay_api_Uninit;
    sdrplay_api_Update_t sdrplay_api_Update;
};
#endif

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
 *    Lazy Chair Computing
 */
#include "sdrplay-handler-v3.h"
#include "constants.h"
#include "logging.h"

#define DEV_PLAYV3 LOG_DEV

#define MIN_GAIN 20
#define MAX_GAIN 59

static
void StreamACallback(short *xi, short *xq,
		     sdrplay_api_StreamCbParamsT *params,
		     unsigned int numSamples,
		     unsigned int reset,
		     void *cbContext) {
    sdrplayHandler_v3 *p = static_cast<sdrplayHandler_v3 *> (cbContext);
    _VLA(std::complex<int16_t>, localBuf, numSamples);
    (void) params;

    if (reset)
	return;

    for (int i = 0; i < (int) numSamples; i ++) {
	std::complex<int16_t> symb = std::complex<int16_t>(xi[i], xq[i]);
	localBuf[i] = symb;
    }
    if (!p->running.load())
	return;
    p->_I_Buffer.putDataIntoBuffer(localBuf, numSamples);
}

static
void StreamBCallback(short *xi, short *xq,
		     sdrplay_api_StreamCbParamsT *params,
		     unsigned int numSamples, unsigned int reset,
		     void *cbContext) {
    (void) xi;
    (void) xq;
    (void) params;
    (void) cbContext;

    if (reset)
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_StreamBCallback: numSamples=%d", numSamples);
}

static
void EventCallback(sdrplay_api_EventT eventId,
		   sdrplay_api_TunerSelectT tuner,
		   sdrplay_api_EventParamsT *params,
		   void *cbContext) {
    sdrplayHandler_v3 *p = static_cast<sdrplayHandler_v3 *>(cbContext);
    (void)tuner;

    switch (eventId) {
	case sdrplay_api_GainChange:
	    break;
	case sdrplay_api_PowerOverloadChange:
	    p->update_PowerOverload(params);
	    break;
	default:
	    log(DEV_PLAYV3, LOG_MIN, "event %d", eventId);
	    break;
    }
}

sdrplayHandler_v3::sdrplayHandler_v3(): _I_Buffer(4 * 1024 * 1024) {
    sdrplay_api_ErrT err;
    sdrplay_api_DeviceT devs[MAX_DEVICES];
    uint32_t devCount;
    float apiVersion;

    running.store(false);
    nrBits = 12;
    signalMin = -2048;
    signalMax = -signalMin-1;
    signalAmplitude = -signalMin;
    GRdB = MIN_GAIN;
    lnaState = 0;
    agcMode = true;

    chosenDevice = NULL;
    deviceParams = NULL;
    cbFns.StreamACbFn = StreamACallback;
    cbFns.StreamBCbFn = StreamBCallback;
    cbFns.EventCbFn = EventCallback;

    fetchLibrary();
    if (Handle == NULL) {
	throw(23);
    }
    if (!loadFunctions()) {
	CLOSE_LIBRARY(Handle);
	throw(23);
    }
    log(DEV_PLAYV3, LOG_MIN, "functions loaded");

    err = sdrplay_api_Open();
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Open failed %s",
		sdrplay_api_GetErrorString (err));
	CLOSE_LIBRARY(Handle);
	throw(23);
    }
    log(DEV_PLAYV3, LOG_MIN, "api opened");

    err = sdrplay_api_ApiVersion(&apiVersion);
    if (err  != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_ApiVersion failed %s",
		sdrplay_api_GetErrorString (err));
	goto closeAPI;
    }
    if (apiVersion < (SDRPLAY_API_VERSION - 0.01)) {
	   log(DEV_PLAYV3, LOG_MIN, "API versions don't match (local=%.2f dll=%.2f)",
		SDRPLAY_API_VERSION, apiVersion);
	goto closeAPI;
    }
    log(DEV_PLAYV3, LOG_MIN, "api version %f detected", apiVersion);

    // We have made the conscious decision not to lock the API!
    err = sdrplay_api_GetDevices(devs, &devCount, MAX_DEVICES);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDevices failed %s",
		sdrplay_api_GetErrorString (err));
	goto closeAPI;
    }
    if (devCount == 0) {
	log(DEV_PLAYV3, LOG_MIN, "no valid device found");
	goto closeAPI;
    }
    log(DEV_PLAYV3, LOG_MIN, "%d devices detected", devCount);

    if (configDevice(&devs[0]))
	log(DEV_PLAYV3, LOG_MIN, "setup seems successfull");
    return;

closeAPI:
    sdrplay_api_Close();
    CLOSE_LIBRARY(Handle);
    throw(24);
}

sdrplayHandler_v3::~sdrplayHandler_v3() {
    sdrplay_api_ErrT err;

    if (chosenDevice != NULL) {
	err = sdrplay_api_ReleaseDevice(chosenDevice);
	if (err != sdrplay_api_Success) 
	    log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_ReleaseDevice failed %s",
		sdrplay_api_GetErrorString (err));
    }
}

bool sdrplayHandler_v3::configDevice(sdrplay_api_DeviceT *devDesc) {
    sdrplay_api_ErrT err;

    err = sdrplay_api_SelectDevice(devDesc);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_SelectDevice failed %s",
		sdrplay_api_GetErrorString(err));
	return false;
    }
    err = sdrplay_api_DebugEnable(devDesc->dev, (sdrplay_api_DbgLvl_t) 1);
 
    // retrieve device parameters, so they can be changed if needed
    err = sdrplay_api_GetDeviceParams(devDesc->dev, &deviceParams);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDeviceParams failed %s",
		sdrplay_api_GetErrorString(err));
	return false;
    }

    if (deviceParams == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDeviceParams return null as par");
	return false;
    }

    chParams = deviceParams->rxChannelA;
    deviceParams->devParams->fsFreq.fsHz = INPUT_RATE;
    chParams->tunerParams.bwType = sdrplay_api_BW_1_536;
    chParams->tunerParams.ifType = sdrplay_api_IF_Zero;
    chParams->tunerParams.gain.gRdB = GRdB;
    chParams->tunerParams.gain.LNAstate = lnaState;
    chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
    if (agcMode) {
	chParams->ctrlParams.agc.setPoint_dBfs = -30;
	chParams->ctrlParams.agc.enable = sdrplay_api_AGC_100HZ;
    } else
	chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;

    chosenDevice = &deviceDesc;
    memcpy(chosenDevice, devDesc, sizeof(deviceDesc));
    switch (devDesc->hwVer) {

	// RSP1
	case 1:
	    lnaGainMax = 3;
	    signalMin = -2048;
	    nrBits = 12;
	    break;

	// RSP2
	case 2:
	    lnaGainMax = 8;
	    signalMin = -8192;
	    nrBits = 14;
	    break;

	// RSP DUO
	case 3:
	    lnaGainMax = 9;
	    signalMin = -2048;
	    nrBits = 12;
	    break;

	// RSPDx
	case 4:
	    lnaGainMax = 26;
	    signalMin = -8192;
	    nrBits = 14;
	    break;

	// RSP1A
	case 255:
	default:
	      lnaGainMax = 9;
	      signalMin = -8192;
	      nrBits = 14;
	      break;
    }
    signalAmplitude = -signalMin;
    signalMax = -signalMin-1;
    return true;
}

const char *deviceName(unsigned char hwVer) {
    switch (hwVer) {
	case 1:
	    return "RSP1";
	case 2:
	    return "RSP2";
	case 3:
	    return "RSP Duo";
	case 4:
	    return "RSP Dx";
	default:
	    return "RSP1A";
    }
}

int sdrplayHandler_v3::devices(deviceStrings *devs, int max) {
    uint32_t count;
    sdrplay_api_DeviceT devDesc[MAX_DEVICES];

    sdrplay_api_GetDevices(devDesc, &count, max-1);
    for (uint32_t i = 0; i < count; i++) {
	strncpy((char *) &devs[i].name, deviceName(devDesc[i].hwVer), DEV_SHORT);
	strncpy((char *) &devs[i].id, devDesc[i].SerNo, DEV_SHORT);
	strncpy((char *) &devs[i].description, devDesc[i].SerNo, DEV_LONG);
	log(DEV_PLAYV3, LOG_CHATTY, "found device %s (%i)", (char *) &devs[i].name, i);
    }

    // This is why we select a device by id and not position: SDRPlay V3 omits the
    // busy devices from the device list, so selecting from position is unreliable
    if (chosenDevice != NULL) {
	strncpy((char *) &devs[count].name, deviceName(chosenDevice->hwVer), DEV_SHORT);
	strncpy((char *) &devs[count].id, chosenDevice->SerNo, DEV_SHORT);
	strncpy((char *) &devs[count].description, chosenDevice->SerNo, DEV_LONG);
	log(DEV_PLAYV3, LOG_CHATTY, "found device %s (%i)", (char *) &devs[count].name, count);
	count++;
    }
    return count;
}

bool sdrplayHandler_v3::setDevice(const char *id) {
    sdrplay_api_ErrT err;
    sdrplay_api_DeviceT devDesc[MAX_DEVICES];
    uint32_t devNo, count;

    if (chosenDevice != NULL && strncmp(id, (char *) chosenDevice->SerNo, DEV_SHORT) == 0) {
	log(DEV_PLAYV3, LOG_MIN, "Skipping device switching - same device: %s", id);
	return true;
    }
    sdrplay_api_GetDevices(devDesc, &count, uint32_t(MAX_DEVICES));
    for (devNo = 0; devNo < count; devNo++) {
	log(DEV_PLAYV3, LOG_CHATTY, "found device %i %s-%s", devNo,
		deviceName(devDesc[devNo].hwVer), (char *) devDesc[devNo].SerNo);
	if (strncmp(id, (char *) devDesc[devNo].SerNo, DEV_SHORT) == 0)
	   break;
    }
    if (devNo >= count) {
	log(DEV_PLAYV3, LOG_MIN, "device not found %s", id);
	return false;
    }

    if (running.load()) {
	log(DEV_PLAYV3, LOG_MIN, "stopping old device");
	stopReader();
    }
    if (chosenDevice != NULL) {
	err = sdrplay_api_ReleaseDevice(chosenDevice);
	if (err != sdrplay_api_Success)
	    log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Release failed with %s",
		sdrplay_api_GetErrorString (err));
	chosenDevice = NULL;
    }
    if (!configDevice(&devDesc[devNo])) {
	return false;
    }
    log(DEV_PLAYV3, LOG_MIN, "switched to device %s", id);
    return true;
}

bool sdrplayHandler_v3::restartReader(int32_t newFreq) {
    sdrplay_api_ErrT err;

    if (chosenDevice == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Restart reader failed: device unset");
	return false;
    }
    err = sdrplay_api_Init(chosenDevice->dev, &cbFns, this);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Init failed %s",
		sdrplay_api_GetErrorString (err));
	return false;
    }

    chParams->tunerParams.rfFreq.rfHz = newFreq;
    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,
			     sdrplay_api_Update_Tuner_Frf,
			     sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "restart: error %s",
	    sdrplay_api_GetErrorString (err));
	return false;
    }
    running.store(true);
    log(DEV_PLAYV3, LOG_MIN, "reader started");
    setIfGain(GRdB);
    setLnaGain(lnaState);
    setAgcControl(agcMode);
    return true;
}

void sdrplayHandler_v3::stopReader() {
    sdrplay_api_ErrT err;

    if (running.load() && chosenDevice != NULL) {
	err = sdrplay_api_Uninit(chosenDevice->dev);
	if (err != sdrplay_api_Success)
	    log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Uninit failed %s",
		sdrplay_api_GetErrorString (err));
    }
    running.store(false);
    log(DEV_PLAYV3, LOG_MIN, "reader stopped");
    _I_Buffer.FlushRingBuffer();
}

int32_t	sdrplayHandler_v3::getSamples(std::complex<float> *V, int32_t size, agcStats *stats) { 
    _VLA(std::complex<int16_t>, temp, size);
    int32_t overflow = 0, minVal = SHRT_MAX, maxVal = SHRT_MIN;
    int i;

    int amount = _I_Buffer.getDataFromBuffer(temp, size);
    for (i = 0; i < amount; i++) {
	int r = real(temp[i]), im = imag(temp[i]);

	if (r <= signalMin || r >= signalMax || im <= signalMin || im >= signalMax)
	    overflow++;
	if (r < minVal)
	    minVal = r;
	if (r > maxVal) 
	    maxVal = r;
	if (im < minVal)
	    minVal = im;
	if (im > maxVal)
	    maxVal = im;
	V[i] = std::complex<float>(r / signalAmplitude, im / signalAmplitude);
    }
    stats->overflows = overflow;
    stats->min = minVal;
    stats->max = maxVal;
    return amount;
}

int32_t sdrplayHandler_v3::Samples() {
    return _I_Buffer.GetRingBufferReadAvailable();
}

void sdrplayHandler_v3::resetBuffer() {
    _I_Buffer.FlushRingBuffer();
}

int16_t sdrplayHandler_v3::bitDepth() {
    return nrBits;
}

int32_t sdrplayHandler_v3::amplitude(void) {
    return signalMax*2+2;
}

void sdrplayHandler_v3::getIfRange(int *min, int *max) {
    *min = MIN_GAIN;
    *max = MAX_GAIN;
}

void sdrplayHandler_v3::getLnaRange(int *min, int *max) {
    *min = 0;
    *max = lnaGainMax;
}

void sdrplayHandler_v3::setIfGain(int newGRdB) {
    sdrplay_api_ErrT err;

    if (newGRdB > MAX_GAIN)
	newGRdB = MAX_GAIN;
    if (newGRdB < MIN_GAIN)
	newGRdB = MIN_GAIN;

    if (!running.load()) {
	GRdB = newGRdB;
	log(DEV_PLAYV3, LOG_MIN, "IF gain will change to val %i at restart",
		GRdB);
	return;
    }
    chParams->tunerParams.gain.gRdB = newGRdB;
    err = sdrplay_api_Update(chosenDevice->dev,
			      chosenDevice->tuner,
			      sdrplay_api_Update_Tuner_Gr,
			      sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "IF change: val %i error %s",
	    newGRdB, sdrplay_api_GetErrorString (err));
    } else  {
	GRdB = newGRdB;
	log(DEV_PLAYV3, LOG_MIN, "IF gain changed to %i", GRdB);
    }
}

void sdrplayHandler_v3::setLnaGain(int newLnaState) {
    sdrplay_api_ErrT err;

    if (newLnaState > lnaGainMax)
	newLnaState = lnaGainMax;
    if (newLnaState < 0)
	newLnaState = 0;
    if (!running.load()) {
	lnaState = newLnaState;
	log(DEV_PLAYV3, LOG_MIN, "Lna gain will change to val %i at restart",
		lnaState);
	return;
    }
    chParams->tunerParams.gain.LNAstate = newLnaState;
    err = sdrplay_api_Update(chosenDevice->dev,
			  chosenDevice->tuner,
			  sdrplay_api_Update_Tuner_Gr,
			  sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "lna change: val %i error %s",
    	newLnaState, sdrplay_api_GetErrorString (err));
    } else {
	lnaState = newLnaState;
	log(DEV_PLAYV3, LOG_MIN, "LNA gain changed to %i", lnaState);
    }
}

void sdrplayHandler_v3::setAgcControl(int mode) {
    sdrplay_api_ErrT err;

    if (!running.load()) {
	agcMode = (mode != 0);
	log(DEV_PLAYV3, LOG_MIN, "AGC will change to %i at restart", agcMode);
	return;
    }
    if (mode != 0) {
	chParams->ctrlParams.agc.setPoint_dBfs = -30;
	chParams->ctrlParams.agc.enable = sdrplay_api_AGC_100HZ;
    } else
	chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
    err = sdrplay_api_Update(chosenDevice->dev,
			     chosenDevice->tuner,
			     sdrplay_api_Update_Ctrl_Agc,
			     sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success) {
	log(DEV_PLAYV3, LOG_MIN, "agc: error %s",
	    sdrplay_api_GetErrorString (err));
    } else {
	agcMode = (mode != 0);
	log(DEV_PLAYV3, LOG_MIN, "AGC changed to %i", agcMode);
    }
}

void sdrplayHandler_v3::update_PowerOverload(sdrplay_api_EventParamsT *params) {
    sdrplay_api_Update(chosenDevice->dev,
		       chosenDevice->tuner,
		       sdrplay_api_Update_Ctrl_OverloadMsgAck,
		       sdrplay_api_Update_Ext1_None);
    if (params->powerOverloadParams.powerOverloadChangeType == sdrplay_api_Overload_Detected) {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Overload_Detected");
    } else {
	log(DEV_PLAYV3, LOG_MIN, "sdrplay_api_Overload Corrected");
    }
}

void sdrplayHandler_v3::fetchLibrary() {
#if IS_WINDOWS
    HKEY APIkey;
    wchar_t APIkeyValue [256];
    ULONG APIkeyValue_length = 255;

    wchar_t *libname = (wchar_t *)L"sdrplay_api.dll";
    Handle = LoadLibrary (libname);
    if (Handle == NULL) {
	if (RegOpenKey(HKEY_LOCAL_MACHINE,
		       TEXT("Software\\MiricsSDR\\API"),
		       &APIkey) != ERROR_SUCCESS) {
	      log(DEV_PLAYV3, LOG_MIN,
		   "failed to locate API registry entry, error = %d",
		   (int) GetLastError());
	      return;
	}
	RegQueryValueEx(APIkey, (wchar_t *)L"Install_Dir",
			NULL,
			NULL,
			(LPBYTE) &APIkeyValue,
			(LPDWORD) &APIkeyValue_length);

	wchar_t *x =
#ifdef  __BITS64__
		wcscat(APIkeyValue, (wchar_t *)L"\\x64\\sdrplay_api.dll");
#else
		wcscat(APIkeyValue, (wchar_t *)L"\\x86\\sdrplay_api.dll");
#endif
	RegCloseKey(APIkey);
	Handle = LoadLibraryW(x);
	if (Handle == NULL)
	   log(DEV_PLAYV3, LOG_MIN, "Failed to open sdrplay_api.dll");
    }
#else
    Handle = dlopen("libusb-1.0" LIBEXT, RTLD_NOW | RTLD_GLOBAL);
    Handle = dlopen("libsdrplay_api" LIBEXT, RTLD_NOW);
    if (Handle == NULL)
	log(DEV_PLAYV3, LOG_MIN, "we could not open libsdrplay_api, error %s", dlerror());
#endif
}

bool sdrplayHandler_v3::loadFunctions() {
    sdrplay_api_Open = (sdrplay_api_Open_t)
	GETPROCADDRESS(Handle, "sdrplay_api_Open");
    if ((void *) sdrplay_api_Open == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Open");
	return false;
    }
    sdrplay_api_Close = (sdrplay_api_Close_t)
	GETPROCADDRESS(Handle, "sdrplay_api_Close");
    if (sdrplay_api_Close == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Close");
	return false;
    }
    sdrplay_api_ApiVersion = (sdrplay_api_ApiVersion_t)
	GETPROCADDRESS(Handle, "sdrplay_api_ApiVersion");
    if (sdrplay_api_ApiVersion == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_ApiVersion");
	return false;
    }
    sdrplay_api_LockDeviceApi = (sdrplay_api_LockDeviceApi_t)
	GETPROCADDRESS(Handle, "sdrplay_api_LockDeviceApi");
    if (sdrplay_api_LockDeviceApi == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_LockdeviceApi");
	return false;
    }
    sdrplay_api_UnlockDeviceApi = (sdrplay_api_UnlockDeviceApi_t)
	GETPROCADDRESS(Handle, "sdrplay_api_UnlockDeviceApi");
    if (sdrplay_api_UnlockDeviceApi == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_UnlockdeviceApi");
	return false;
    }
    sdrplay_api_GetDevices = (sdrplay_api_GetDevices_t)
	GETPROCADDRESS(Handle, "sdrplay_api_GetDevices");
    if (sdrplay_api_GetDevices == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetDevices");
	return false;
    }
    sdrplay_api_SelectDevice = (sdrplay_api_SelectDevice_t)
	GETPROCADDRESS(Handle, "sdrplay_api_SelectDevice");
    if (sdrplay_api_SelectDevice == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_SelectDevice");
	return false;
    }
    sdrplay_api_ReleaseDevice = (sdrplay_api_ReleaseDevice_t)
	 GETPROCADDRESS(Handle, "sdrplay_api_ReleaseDevice");
    if (sdrplay_api_ReleaseDevice == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_ReleaseDevice");
	return false;
    }
    sdrplay_api_GetErrorString = (sdrplay_api_GetErrorString_t)
	GETPROCADDRESS(Handle, "sdrplay_api_GetErrorString");
    if (sdrplay_api_GetErrorString == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetErrorString");
	return false;
    }
    sdrplay_api_GetLastError = (sdrplay_api_GetLastError_t)
	GETPROCADDRESS(Handle, "sdrplay_api_GetLastError");
    if (sdrplay_api_GetLastError == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetLastError");
	return false;
    }
    sdrplay_api_DebugEnable = (sdrplay_api_DebugEnable_t)
	GETPROCADDRESS(Handle, "sdrplay_api_DebugEnable");
    if (sdrplay_api_DebugEnable == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_DebugEnable");
	return false;
    }
    sdrplay_api_GetDeviceParams = (sdrplay_api_GetDeviceParams_t)
	GETPROCADDRESS(Handle, "sdrplay_api_GetDeviceParams");
    if (sdrplay_api_GetDeviceParams == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetDeviceParams");
	return false;
    }
    sdrplay_api_Init = (sdrplay_api_Init_t)
	GETPROCADDRESS(Handle, "sdrplay_api_Init");
    if (sdrplay_api_Init == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Init");
	return false;
    }
    sdrplay_api_Uninit = (sdrplay_api_Uninit_t)
	GETPROCADDRESS(Handle, "sdrplay_api_Uninit");
    if (sdrplay_api_Uninit == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Uninit");
	return false;
    }
    sdrplay_api_Update = (sdrplay_api_Update_t)
	GETPROCADDRESS(Handle, "sdrplay_api_Update");
    if (sdrplay_api_Update == NULL) {
	log(DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Update");
	return false;
    }
    return true;
}

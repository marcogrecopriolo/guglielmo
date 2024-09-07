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
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "sdrplay-handler.h"
#include "logging.h"

#define DEV_PLAY LOG_DEV

#define MIN_GAIN 20
#define MAX_GAIN 59

sdrplayHandler::sdrplayHandler(): _I_Buffer (4 * 1024 * 1024) {
    int	err;
    float ver;
    mir_sdr_DeviceT devDesc[4];

    this-> GRdB	= MIN_GAIN;
    this-> lnaGain = 0;
    this-> agcMode = false;
    this-> inputRate = Khz (2048);
    libraryLoaded = false;

#if IS_WINDOWS
    HKEY APIkey;
    wchar_t APIkeyValue[256];
    ULONG APIkeyValue_length = 255;

    wchar_t *libname = (wchar_t *)L"mir_sdr_api.dll";
    Handle  = LoadLibrary(libname);
    if (Handle == NULL) {
	if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\MiricsSDR\\API"),
		      &APIkey) != ERROR_SUCCESS) {
	    log(DEV_PLAY, LOG_MIN, "failed to locate API registry entry, error = %d",
		(int) GetLastError());
	    throw(21);
	}

	RegQueryValueEx(APIkey, (wchar_t *) L"Install_Dir",
			NULL, NULL, (LPBYTE) &APIkeyValue,
			(LPDWORD) &APIkeyValue_length);

	wchar_t *x =
#ifdef	__BITS64__
		wcscat (APIkeyValue, (wchar_t *)L"\\x64\\mir_sdr_api.dll");
#else
		wcscat (APIkeyValue, (wchar_t *)L"\\x86\\mir_sdr_api.dll");
#endif
	RegCloseKey(APIkey);
	Handle	= LoadLibraryW (x);
	if (Handle == NULL) {
	    log(DEV_PLAY, LOG_MIN, "Failed to open mir_sdr_api.dll");
	    throw(22);
	}
    }
#else
    Handle = dlopen("libusb-1.0" LIBEXT, RTLD_NOW | RTLD_GLOBAL);
    Handle = dlopen("libmirsdrapi-rsp" LIBEXT, RTLD_NOW);
    if (Handle == NULL) {
	log (DEV_PLAY, LOG_MIN, "we could not load libmirsdrapi-rsp");
	throw(23);
    }
#endif
    libraryLoaded = true;

    if (!loadFunctions()) {
	CLOSE_LIBRARY(Handle);
	throw(23);
    }

    err = my_mir_sdr_ApiVersion(&ver);
    if (ver < 2.13) {
	log (DEV_PLAY, LOG_MIN, "please install mir_sdr library >= 2.13");
	CLOSE_LIBRARY(Handle);
	throw(24);
    }

    my_mir_sdr_GetDevices(devDesc, &numofDevs, uint32_t(4));
    if (numofDevs == 0) {
	log (DEV_PLAY, LOG_MIN, "no device found");
	CLOSE_LIBRARY(Handle);
	throw(25);
    }

    deviceIndex = 0;
    hwVersion = devDesc[deviceIndex].hwVer;
    log(DEV_PLAY, LOG_MIN, "sdrplay hwVer = %d", hwVersion);
    err = my_mir_sdr_SetDeviceIdx(deviceIndex);
    if (err != mir_sdr_Success) {
	log(DEV_PLAY, LOG_MIN, "SetDevice failed");
	throw(26);
    }

    // we know we are only in the frequency range 88 .. 230 Mhz,
    // so we can rely on a single table for the lna reductions
    switch (hwVersion) {

	// RSP1
	case 1:
	    lnaGainMax = 3;
	    nrBits = 12;
	    denominator	= 2048;
	    break;

	// RSP2
	case 2:
	    lnaGainMax = 8;
	    nrBits = 14;
	    denominator	= 8192;
	    break;

	// RSP DUO
	case 3:
	    lnaGainMax = 9;
	    nrBits = 14;
	    denominator	= 8192;
	    break;

	// RSP1A
	default:
	    lnaGainMax = 9;
	    nrBits = 14;
	    denominator	= 8192;
	    break;
    }

    if (hwVersion == 2) {
	   mir_sdr_ErrT err;
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	   if (err != mir_sdr_Success)
	      log (DEV_PLAY, LOG_MIN, "error %d in setting antenna", err);
	   
    }

    if (hwVersion == 3) {   // duo
	   err  = my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	   if (err != mir_sdr_Success)
	      log (DEV_PLAY, LOG_MIN, "error %d in setting of rspDuo", err);
    }

    running.store(false);
}

sdrplayHandler::~sdrplayHandler(void) {

    // should not happen, but still!
    if (!libraryLoaded)
	return;
    stopReader();
    CLOSE_LIBRARY(Handle);
}


void sdrplayHandler::getIfRange(int *min, int *max) {
    *min = MIN_GAIN;
    *max = MAX_GAIN;
}

void sdrplayHandler::getLnaRange(int *min, int *max) {
    *min = 0;
    *max = lnaGainMax;
}

void sdrplayHandler::setIfGain(int newGRdB) {
    mir_sdr_ErrT err;

    if (!running.load())
	return;

    if (newGRdB > MAX_GAIN)
	newGRdB = MAX_GAIN;
    if (newGRdB < MIN_GAIN)
	newGRdB = MIN_GAIN;
    err =  my_mir_sdr_RSP_SetGr(newGRdB, lnaGain, 1, 0);
    if (err != mir_sdr_Success) {
	log (DEV_PLAY, LOG_MIN, "error at set_ifgain %s (%d %d)",
	     errorCodes(err).toLatin1().data(), newGRdB, lnaGain);
	return;
    } else
	GRdB = newGRdB;
}

void sdrplayHandler::setLnaGain(int lnaState) {
    mir_sdr_ErrT err;

    if (!running.load())
	return;

    if (lnaState > lnaGainMax)
	lnaState = lnaGainMax;
    if (lnaState < 0)
	lnaState = 0;
    err = my_mir_sdr_AgcControl(agcMode? mir_sdr_AGC_100HZ: mir_sdr_AGC_DISABLE,
				-30, 0, 0, 0, 0, lnaState);
    if (err != mir_sdr_Success) {
	   log (DEV_PLAY, LOG_MIN, "error at set_lnagainReduction %s",
		errorCodes(err).toLatin1().data());
	return;
    } else
	lnaGain = lnaState;
}

void sdrplayHandler::setAgcControl(int mode) {
    agcMode = (mode > 0);
    my_mir_sdr_AgcControl(agcMode? mir_sdr_AGC_100HZ: mir_sdr_AGC_DISABLE,
			      -30, 0, 0, 0, 0, lnaGain);
}

static
void myStreamCallback(int16_t *xi,
		      int16_t *xq,
		      uint32_t firstSampleNum, 
		      int32_t grChanged,
		      int32_t rfChanged,
		      int32_t fsChanged,
		      uint32_t numSamples,
		      uint32_t reset,
		      uint32_t hwRemoved,
		      void *cbContext) {
    int16_t i;
    sdrplayHandler *p = static_cast<sdrplayHandler *> (cbContext);
    float denominator = p->denominator;
    std::complex<float> *localBuf =
	   (std::complex<float> *) alloca(numSamples * sizeof (std::complex<float>));

    if (reset || hwRemoved)
	return;

    for (i = 0; i < (int) numSamples;i ++)
	localBuf[i] = std::complex<float>(float (xi[i]) / denominator,
					  float (xq[i]) / denominator);
    p->_I_Buffer.putDataIntoBuffer(localBuf, numSamples);
    (void) firstSampleNum;
    (void) grChanged;
    (void) rfChanged;
    (void) fsChanged;
    (void) reset;
}

void myGainChangeCallback(uint32_t gRdB,
			  uint32_t lnaGRdB,
			  void *cbContext) {
    log(DEV_PLAY, LOG_VERBOSE, "GainChangeCallback gives %X", gRdB);
    (void) gRdB;
    (void) lnaGRdB;	
    (void) cbContext;
}

bool sdrplayHandler::restartReader(int32_t frequency) {
    int gRdBSystem;
    int samplesPerPacket;
    mir_sdr_ErrT err;

    if (running.load())
	return true;

    vfoFrequency = frequency;
    err	= my_mir_sdr_StreamInit(&GRdB,
				double (inputRate) / MHz (1),
				double (frequency) / Mhz (1),
				mir_sdr_BW_1_536,
				mir_sdr_IF_Zero,
				lnaGain,
				&gRdBSystem,
				mir_sdr_USE_RSP_SET_GR,
				&samplesPerPacket,
				(mir_sdr_StreamCallback_t) myStreamCallback,
				(mir_sdr_GainChangeCallback_t) myGainChangeCallback,
				this);
    if (err != mir_sdr_Success) {
	log(DEV_PLAY, LOG_MIN, "restart with error = %d", err);
	return false;
    }
	
    if (agcMode)
	my_mir_sdr_AgcControl(mir_sdr_AGC_100HZ, -30,
			       0, 0, 0, 0, lnaGain);
    else
	my_mir_sdr_AgcControl(mir_sdr_AGC_DISABLE, -30,
			       0, 0, 0, 0, lnaGain);

    err	= my_mir_sdr_SetDcMode(4, 1);
    err	= my_mir_sdr_SetDcTrackTime(63);
    running.store(true);
    return true;
}

void sdrplayHandler::stopReader(void) {
    if (!running.load())
	return;

    my_mir_sdr_StreamUninit();
    running.store(false);
    _I_Buffer.FlushRingBuffer();
}

int32_t	sdrplayHandler::getSamples(std::complex<float> *V, int32_t size, agcStats *stats) { 
    (void) stats;
    return _I_Buffer. getDataFromBuffer(V, size);
}

int32_t	sdrplayHandler::Samples(void) {
    return _I_Buffer.GetRingBufferReadAvailable();
}

void sdrplayHandler::resetBuffer(void) {
    _I_Buffer. FlushRingBuffer();
}

int16_t	sdrplayHandler::bitDepth(void) {
    return nrBits;
}

int32_t sdrplayHandler::getRate(void) {
    return inputRate;
}

bool sdrplayHandler::loadFunctions(void) {
    my_mir_sdr_StreamInit = (pfn_mir_sdr_StreamInit)
	GETPROCADDRESS(Handle, "mir_sdr_StreamInit");
    if (my_mir_sdr_StreamInit == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_StreamInit");
	return false;
    }

    my_mir_sdr_StreamUninit = (pfn_mir_sdr_StreamUninit)
	GETPROCADDRESS(Handle, "mir_sdr_StreamUninit");
    if (my_mir_sdr_StreamUninit == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_StreamUninit");
	return false;
    }

    my_mir_sdr_SetRf = (pfn_mir_sdr_SetRf)
	GETPROCADDRESS(Handle, "mir_sdr_SetRf");
    if (my_mir_sdr_SetRf == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetRf");
	return false;
    }

    my_mir_sdr_SetFs = (pfn_mir_sdr_SetFs)
	GETPROCADDRESS(Handle, "mir_sdr_SetFs");
    if (my_mir_sdr_SetFs == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetFs");
	return false;
    }

    my_mir_sdr_SetGr = (pfn_mir_sdr_SetGr)
	GETPROCADDRESS(Handle, "mir_sdr_SetGr");
    if (my_mir_sdr_SetGr == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetGr");
	return false;
    }

    my_mir_sdr_RSP_SetGr  = (pfn_mir_sdr_RSP_SetGr)
	GETPROCADDRESS(Handle, "mir_sdr_RSP_SetGr");
    if (my_mir_sdr_RSP_SetGr == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_RSP_SetGr");
	return false;
    }

    my_mir_sdr_SetGrParams = (pfn_mir_sdr_SetGrParams)
	GETPROCADDRESS(Handle, "mir_sdr_SetGrParams");
    if (my_mir_sdr_SetGrParams == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetGrParams");
	return false;
    }

    my_mir_sdr_SetDcMode = (pfn_mir_sdr_SetDcMode)
	GETPROCADDRESS(Handle, "mir_sdr_SetDcMode");
    if (my_mir_sdr_SetDcMode == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetDcMode");
	return false;
    }

    my_mir_sdr_SetDcTrackTime = (pfn_mir_sdr_SetDcTrackTime)
	GETPROCADDRESS(Handle, "mir_sdr_SetDcTrackTime");
    if (my_mir_sdr_SetDcTrackTime == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetDcTrackTime");
	return false;
    }

    my_mir_sdr_SetSyncUpdateSampleNum = (pfn_mir_sdr_SetSyncUpdateSampleNum)
	GETPROCADDRESS(Handle, "mir_sdr_SetSyncUpdateSampleNum");
    if (my_mir_sdr_SetSyncUpdateSampleNum == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetSyncUpdateSampleNum");
	return false;
    }

    my_mir_sdr_SetSyncUpdatePeriod = (pfn_mir_sdr_SetSyncUpdatePeriod)
	GETPROCADDRESS(Handle, "mir_sdr_SetSyncUpdatePeriod");
    if (my_mir_sdr_SetSyncUpdatePeriod == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetSyncUpdatePeriod");
	return false;
    }

    my_mir_sdr_ApiVersion = (pfn_mir_sdr_ApiVersion)
	GETPROCADDRESS(Handle, "mir_sdr_ApiVersion");
    if (my_mir_sdr_ApiVersion == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_ApiVersion");
	return false;
    }

    my_mir_sdr_AgcControl = (pfn_mir_sdr_AgcControl)
	GETPROCADDRESS(Handle, "mir_sdr_AgcControl");
    if (my_mir_sdr_AgcControl == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_AgcControl");
	return false;
    }

    my_mir_sdr_Reinit = (pfn_mir_sdr_Reinit)
	GETPROCADDRESS(Handle, "mir_sdr_Reinit");
    if (my_mir_sdr_Reinit == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_Reinit");
	return false;
    }

    my_mir_sdr_SetPpm = (pfn_mir_sdr_SetPpm)
	GETPROCADDRESS(Handle, "mir_sdr_SetPpm");
    if (my_mir_sdr_SetPpm == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetPpm");
	return false;
    }

    my_mir_sdr_DebugEnable = (pfn_mir_sdr_DebugEnable)
	GETPROCADDRESS(Handle, "mir_sdr_DebugEnable");
    if (my_mir_sdr_DebugEnable == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_DebugEnable");
	return false;
    }

    my_mir_sdr_rspDuo_TunerSel = (pfn_mir_sdr_rspDuo_TunerSel)
	GETPROCADDRESS(Handle, "mir_sdr_rspDuo_TunerSel");
    if (my_mir_sdr_rspDuo_TunerSel == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_rspDuo_TunerSel");
	return false;
    }

    my_mir_sdr_DCoffsetIQimbalanceControl = (pfn_mir_sdr_DCoffsetIQimbalanceControl)
	GETPROCADDRESS(Handle, "mir_sdr_DCoffsetIQimbalanceControl");
    if (my_mir_sdr_DCoffsetIQimbalanceControl == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_DCoffsetIQimbalanceControl");
	return false;
    }

    my_mir_sdr_ResetUpdateFlags	= (pfn_mir_sdr_ResetUpdateFlags)
	GETPROCADDRESS(Handle, "mir_sdr_ResetUpdateFlags");
    if (my_mir_sdr_ResetUpdateFlags == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_ResetUpdateFlags");
	return false;
    }

    my_mir_sdr_GetDevices = (pfn_mir_sdr_GetDevices)
	GETPROCADDRESS(Handle, "mir_sdr_GetDevices");
    if (my_mir_sdr_GetDevices == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_GetDevices");
	return false;
    }

    my_mir_sdr_GetCurrentGain = (pfn_mir_sdr_GetCurrentGain)
	GETPROCADDRESS(Handle, "mir_sdr_GetCurrentGain");
    if (my_mir_sdr_GetCurrentGain == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_GetCurrentGain");
	return false;
    }

    my_mir_sdr_GetHwVersion = (pfn_mir_sdr_GetHwVersion)
	GETPROCADDRESS(Handle, "mir_sdr_GetHwVersion");
    if (my_mir_sdr_GetHwVersion == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_GetHwVersion");
	return false;
    }

    my_mir_sdr_RSPII_AntennaControl = (pfn_mir_sdr_RSPII_AntennaControl)
	GETPROCADDRESS(Handle, "mir_sdr_RSPII_AntennaControl");
    if (my_mir_sdr_RSPII_AntennaControl == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_RSPII_AntennaControl");
	return false;
    }

    my_mir_sdr_SetDeviceIdx = (pfn_mir_sdr_SetDeviceIdx)
	GETPROCADDRESS(Handle, "mir_sdr_SetDeviceIdx");
    if (my_mir_sdr_SetDeviceIdx == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_SetDeviceIdx");
	return false;
    }

    my_mir_sdr_ReleaseDeviceIdx = (pfn_mir_sdr_ReleaseDeviceIdx)
	GETPROCADDRESS(Handle, "mir_sdr_ReleaseDeviceIdx");
    if (my_mir_sdr_ReleaseDeviceIdx == NULL) {
	log(DEV_PLAY, LOG_MIN, "Could not find mir_sdr_ReleaseDeviceIdx");
	return false;
    }

    return true;
}


QString	sdrplayHandler::errorCodes(mir_sdr_ErrT err) {
    switch (err) {
	case mir_sdr_Success:
	    return "success";
	case mir_sdr_Fail:
	    return "Fail";
	case mir_sdr_InvalidParam:
	    return "invalidParam";
	case mir_sdr_OutOfRange:
	    return "OutOfRange";
	case mir_sdr_GainUpdateError:
	    return "GainUpdateError";
	case mir_sdr_RfUpdateError:
	    return "RfUpdateError";
	case mir_sdr_FsUpdateError:
	    return "FsUpdateError";
	case mir_sdr_HwError:
	    return "HwError";
	case mir_sdr_AliasingError:
	    return "AliasingError";
	case mir_sdr_AlreadyInitialised:
	    return "AlreadyInitialised";
	case mir_sdr_NotInitialised:
	    return "NotInitialised";
	case mir_sdr_NotEnabled:
	    return "NotEnabled";
	case mir_sdr_HwVerError:
	    return "HwVerError";
	case mir_sdr_OutOfMemError:
	    return "OutOfMemError";
	case mir_sdr_HwRemoved:
	    return "HwRemoved";
	default:
	    return "unknown code";
    }
}

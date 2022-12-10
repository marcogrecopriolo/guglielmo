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
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include	"sdrplay-handler-v3.h"
#include	"sdrplay-commands.h"
#include	"constants.h"
#include	"logging.h"

#define DEV_PLAYV3 LOG_DEV

#define MIN_GAIN        20
#define MAX_GAIN        39

	sdrplayHandler_v3::sdrplayHandler_v3  ():
	                                          _I_Buffer (4 * 1024 * 1024) {
	nrBits			= 12;	// default

	denominator		= 2048;	// default

	vfoFrequency	= MHz (220);
	failFlag	= false;
	successFlag	= false;
	GRdB		= 30;
	lnaState	= 5;
	agcMode		= true;
	inputRate	= 2048000;
	start ();
	while (!failFlag && !successFlag)
	   usleep (1000);
	if (failFlag) {
	   while (isRunning ())
	      usleep (1000);
	   throw (21);
	}
	log (DEV_PLAYV3, LOG_MIN, "setup seems successfull");
}

	sdrplayHandler_v3::~sdrplayHandler_v3 () {
	threadRuns. store (false);
	while (isRunning ())
	   usleep (1000);
}

/////////////////////////////////////////////////////////////////////////
//	Implementing the interface
/////////////////////////////////////////////////////////////////////////

bool	sdrplayHandler_v3::restartReader	(int32_t newFreq) {
restartRequest r_1 (newFreq);
GRdBRequest r_2 (GRdB);
        if (receiverRuns. load ())
           return true;
        vfoFrequency    = newFreq;
        messageHandler (&r_1);
        return messageHandler (&r_2);
}

void	sdrplayHandler_v3::stopReader	() {
stopRequest r;
        if (!receiverRuns. load ())
           return;
        messageHandler (&r);
}
//
int32_t	sdrplayHandler_v3::getSamples (std::complex<float> *V, int32_t size) { 
_VLA(std::complex<int16_t>, temp, size);
int	i;

	int amount      = _I_Buffer. getDataFromBuffer (temp, size);
        for (i = 0; i < amount; i ++)
           V [i] = std::complex<float> (real (temp [i]) / (float) denominator,
                                        imag (temp [i]) / (float) denominator);
        return amount;
}

int32_t	sdrplayHandler_v3::Samples	() {
	return _I_Buffer. GetRingBufferReadAvailable();
}

void	sdrplayHandler_v3::resetBuffer	() {
	_I_Buffer. FlushRingBuffer();
}

int16_t	sdrplayHandler_v3::bitDepth	() {
	return nrBits;
}

int32_t sdrplayHandler_v3::getRate (void) {
        return inputRate;
}

///////////////////////////////////////////////////////////////////////////
//	Handling the GUI
//////////////////////////////////////////////////////////////////////

void	sdrplayHandler_v3::setIfGain (int newGRdB) {
	newGRdB = newGRdB * MAX_GAIN / 100 + MIN_GAIN;
	GRdBRequest r (newGRdB);
	if (!receiverRuns. load ())
           return;
        messageHandler (&r);
	GRdB = newGRdB;
	emit configurationChanged();
}

void	sdrplayHandler_v3::setLnaGain (int newLnaState) {
	newLnaState = newLnaState * lna_upperBound / 100;
	lnaRequest r (newLnaState);
	if (!receiverRuns. load ())
           return;
        messageHandler (&r);
	lnaState = newLnaState;
	emit configurationChanged();
}

void	sdrplayHandler_v3::setAgcControl (int dummy) {
	agcMode = (dummy > 0);
	agcRequest r (agcMode, 30);
	(void)dummy;
        messageHandler (&r);
	emit configurationChanged();
}

///////////////////////////////////////////////////////////////////////
//	the real controller starts here
///////////////////////////////////////////////////////////////////////

bool    sdrplayHandler_v3::messageHandler (generalCommand *r) {
        server_queue. push (r);
        serverjobs. release (1);
        while (!r -> waiter. tryAcquire (1, 1000))
           if (!threadRuns. load ())
              return false;
        return true;
}

static
void    StreamACallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples,
	                 unsigned int reset,
                         void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
_VLA(std::complex<int16_t>, localBuf, numSamples);

	(void)params;
	if (reset)
	   return;
	if (!p -> receiverRuns. load ())
	   return;

	for (int i = 0; i <  (int)numSamples; i ++) {
	   std::complex<int16_t> symb = std::complex<int16_t> (xi [i], xq [i]);
	   localBuf [i] = symb;
	}
	p -> _I_Buffer. putDataIntoBuffer (localBuf, numSamples);
}

static
void	StreamBCallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples, unsigned int reset,
                         void *cbContext) {
	(void)xi; (void)xq; (void)params; (void)cbContext;
        if (reset)
           printf ("sdrplay_api_StreamBCallback: numSamples=%d", numSamples);
}

static
void	EventCallback (sdrplay_api_EventT eventId,
                       sdrplay_api_TunerSelectT tuner,
                       sdrplay_api_EventParamsT *params,
                       void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
	(void)tuner;
	switch (eventId) {
	   case sdrplay_api_GainChange:
	      break;

	   case sdrplay_api_PowerOverloadChange:
	      p -> update_PowerOverload (params);
	      break;

	   default:
	      log (DEV_PLAYV3, LOG_MIN, "event %d", eventId);
	      break;
	}
}

void	sdrplayHandler_v3::
	         update_PowerOverload (sdrplay_api_EventParamsT *params) {
	sdrplay_api_Update (chosenDevice -> dev,
	                    chosenDevice -> tuner,
	                    sdrplay_api_Update_Ctrl_OverloadMsgAck,
	                    sdrplay_api_Update_Ext1_None);
	if (params -> powerOverloadParams.powerOverloadChangeType ==
	                                    sdrplay_api_Overload_Detected) {
//	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Overload_Detected");
	}
	else {
//	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Overload Corrected");
	}
}

void	sdrplayHandler_v3::run		() {
sdrplay_api_ErrT        err;
sdrplay_api_DeviceT     devs [6];
uint32_t                ndev;

        threadRuns. store (false);
	receiverRuns. store (false);

	chosenDevice		= nullptr;
	deviceParams		= nullptr;

	denominator		= 2048;		// default
	nrBits			= 12;		// default

	Handle			= fetchLibrary ();
	if (Handle == nullptr) {
	   failFlag	= true;
	   return;
	}

//	load the functions
	bool success	= loadFunctions ();
	if (!success) {
	   failFlag	= true;
	   releaseLibrary ();
	   return;
        }
	log (DEV_PLAYV3, LOG_MIN, "functions loaded");

//	try to open the API
	err	= sdrplay_api_Open ();
	if (err != sdrplay_api_Success) {
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Open failed %s",
	                          sdrplay_api_GetErrorString (err));
	   releaseLibrary ();
	   failFlag	= true;
	   return;
	}

	log (DEV_PLAYV3, LOG_MIN, "api opened");

//	Check API versions match
        err = sdrplay_api_ApiVersion (&apiVersion);
        if (err  != sdrplay_api_Success) {
           log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_ApiVersion failed %s",
                                     sdrplay_api_GetErrorString (err));
	   goto closeAPI;
        }

        if (apiVersion < (SDRPLAY_API_VERSION - 0.01)) {
           log (DEV_PLAYV3, LOG_MIN, "API versions don't match (local=%.2f dll=%.2f)",
                                              SDRPLAY_API_VERSION, apiVersion);
	   goto closeAPI;
	}
	
	log (DEV_PLAYV3, LOG_MIN, "api version %f detected", apiVersion);
//
//	lock API while device selection is performed
	sdrplay_api_LockDeviceApi ();
	{  int s	= sizeof (devs) / sizeof (sdrplay_api_DeviceT);
	   err	= sdrplay_api_GetDevices (devs, &ndev, s);
	   if (err != sdrplay_api_Success) {
	      log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDevices failed %s",
	                      sdrplay_api_GetErrorString (err));
	      goto unlockDevice_closeAPI;
	   }
	}

	if (ndev == 0) {
	   log (DEV_PLAYV3, LOG_MIN, "no valid device found");
	   goto unlockDevice_closeAPI;
	}

	log (DEV_PLAYV3, LOG_MIN, "%d devices detected", ndev);
	chosenDevice	= &devs [0];
	err	= sdrplay_api_SelectDevice (chosenDevice);
	if (err != sdrplay_api_Success) {
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_SelectDevice failed %s",
	                         sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
//	we have a device, unlock
	sdrplay_api_UnlockDeviceApi ();

	err	= sdrplay_api_DebugEnable (chosenDevice -> dev, 
	                                         (sdrplay_api_DbgLvl_t)1);
//	retrieve device parameters, so they can be changed if needed
	err	= sdrplay_api_GetDeviceParams (chosenDevice -> dev,
	                                                     &deviceParams);
	if (err != sdrplay_api_Success) {
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDeviceParams failed %s",
	                         sdrplay_api_GetErrorString (err));
	   goto closeAPI;
	}

	if (deviceParams == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_GetDeviceParams return null as par");
	   goto closeAPI;
	}
//
	vfoFrequency	= Khz (220000);		// default
//	set the parameter values
	chParams	= deviceParams -> rxChannelA;
	deviceParams	-> devParams -> fsFreq. fsHz	= inputRate;
	chParams	-> tunerParams. bwType = sdrplay_api_BW_1_536;
	chParams	-> tunerParams. ifType = sdrplay_api_IF_Zero;
//
//	these will change:
	chParams	-> tunerParams. rfFreq. rfHz    = 220000000.0;
	chParams	-> tunerParams. gain.gRdB	= GRdB;
	chParams	-> tunerParams. gain.LNAstate	= lnaState;
	chParams	-> ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
	if (agcMode) {
	   chParams    -> ctrlParams. agc. setPoint_dBfs = -30;
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_100HZ;
	}
	else
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;
//
//	assign callback functions
	cbFns. StreamACbFn	= StreamACallback;
	cbFns. StreamBCbFn	= StreamBCallback;
	cbFns. EventCbFn	= EventCallback;

	err	= sdrplay_api_Init (chosenDevice -> dev, &cbFns, this);
	if (err != sdrplay_api_Success) {
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Init failed %s",
                                       sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
	serial		= devs [0]. SerNo;
	hwVersion	= devs [0]. hwVer;
	switch (hwVersion) {
	   case 1:		// old RSP
	      lna_upperBound	= 3;
	      deviceModel	= "RSP-I";
	      denominator	= 2048;
	      nrBits		= 12;
	      break;
	   case 2:		// RSP II
	      lna_upperBound	= 8;
	      deviceModel 	= "RSP-II";
	      denominator	= 2048;
	      nrBits		= 14;
	      break;
	   case 3:		// RSP-DUO
	      lna_upperBound	= 9;
	      deviceModel	= "RSP-DUO";
	      denominator	= 2048;
	      nrBits		= 12;
	      break;
	   case 4:		// RSPDx
	      lna_upperBound	= 26;
	      deviceModel	= "RSP-Dx";
	      denominator	= 2048;
	      nrBits		= 14;
	      break;

	   default:
	   case 255:		// RSP-1A
	      lna_upperBound	= 9;
	      deviceModel	= "RSP-1A";
	      denominator	= 8192;
	      nrBits		= 14;
	      break;
	}

	threadRuns. store (true);	// it seems we can do some work

	successFlag	= true;
	while (threadRuns. load ()) {
	   while (!serverjobs. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      goto normal_exit;
//
//	here we could assert that the server_queue is not empty
	   switch (server_queue. front () -> cmd) {
	      case RESTART_REQUEST: {
	         restartRequest *p = (restartRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. rfFreq. rfHz =
	                                            (float)(p -> frequency);
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Frf,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    log (DEV_PLAYV3, LOG_MIN, "restart: error %s",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
                 }
	         receiverRuns. store (true);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case STOP_REQUEST: {
	         stopRequest *p =
	                  (stopRequest *)(server_queue. front ());
	         server_queue. pop ();
	         receiverRuns. store (false);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case AGC_REQUEST: {
	         agcRequest *p = 
	                    (agcRequest *)(server_queue. front ());
	         server_queue. pop ();
	         if (p -> agcMode) {
	            chParams    -> ctrlParams. agc. setPoint_dBfs =
	                                              - p -> setPoint;
                    chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_100HZ;
	         }
	         else
	            chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;
	         p -> result = true;
	         err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Ctrl_Agc,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    log (DEV_PLAYV3, LOG_MIN, "agc: error %s",
	                                   sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case GRDB_REQUEST: {
	         GRdBRequest *p =  (GRdBRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. gRdB = p -> GRdBValue;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    log (DEV_PLAYV3, LOG_MIN, "grdb: error %s",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case PPM_REQUEST: {
	         ppmRequest *p = (ppmRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> waiter. release (1);
	         break;
	      }

	      case LNA_REQUEST: {
	         lnaRequest *p = (lnaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. LNAstate =
	                                          p -> lnaState;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    log (DEV_PLAYV3, LOG_MIN, "grdb: error %s",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case ANTENNASELECT_REQUEST: {
	         antennaRequest *p = (antennaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> waiter. release (1);
	         break;
	      }
	
	      case GAINVALUE_REQUEST: {
	         gainvalueRequest *p = 
	                        (gainvalueRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         p -> waiter. release (1);
	         break;
	      }

	      default:		// cannot happen
	         break;
	   }
	}

normal_exit:
	err = sdrplay_api_Uninit	(chosenDevice -> dev);
	if (err != sdrplay_api_Success) 
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Uninit failed %s",
	                          sdrplay_api_GetErrorString (err));

	err = sdrplay_api_ReleaseDevice	(chosenDevice);
	if (err != sdrplay_api_Success) 
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_ReleaseDevice failed %s",
	                          sdrplay_api_GetErrorString (err));

//	sdrplay_api_UnlockDeviceApi	(); ??
        sdrplay_api_Close               ();
	if (err != sdrplay_api_Success) 
	   log (DEV_PLAYV3, LOG_MIN, "sdrplay_api_Close failed %s",
	                          sdrplay_api_GetErrorString (err));

	releaseLibrary			();
	log (DEV_PLAYV3, LOG_MIN, "library released, ready to stop thread");
	msleep (200);
	return;

unlockDevice_closeAPI:
	sdrplay_api_UnlockDeviceApi	();
closeAPI:	
	failFlag	= true;
	sdrplay_api_ReleaseDevice       (chosenDevice);
        sdrplay_api_Close               ();
	releaseLibrary	();
	log (DEV_PLAYV3, LOG_MIN, "stopping");
}

/////////////////////////////////////////////////////////////////////////////
//	handling the library
/////////////////////////////////////////////////////////////////////////////

HINSTANCE	sdrplayHandler_v3::fetchLibrary () {
HINSTANCE	Handle	= nullptr;
#if IS_WINDOWS
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

	wchar_t *libname = (wchar_t *)L"sdrplay_api.dll";
	Handle	= LoadLibrary (libname);
	if (Handle == nullptr) {
	   if (RegOpenKey (HKEY_LOCAL_MACHINE,
	                   TEXT("Software\\MiricsSDR\\API"),
	                   &APIkey) != ERROR_SUCCESS) {
              log (DEV_PLAYV3, LOG_MIN,
	           "failed to locate API registry entry, error = %d",
	           (int)GetLastError());
	      return nullptr;
	   }

	   RegQueryValueEx (APIkey,
	                    (wchar_t *)L"Install_Dir",
	                    nullptr,
	                    nullptr,
	                    (LPBYTE)&APIkeyValue,
	                    (LPDWORD)&APIkeyValue_length);
//	Ok, make explicit it is in the 32/64 bits section
	   wchar_t *x =
	        wcscat (APIkeyValue, (wchar_t *)L"\\x86\\sdrplay_api.dll");
//	        wcscat (APIkeyValue, (wchar_t *)L"\\x64\\sdrplay_api.dll");
	   RegCloseKey(APIkey);

	   Handle	= LoadLibraryW (x);
	   if (Handle == nullptr) {
	      log (DEV_PLAYV3, LOG_MIN, "Failed to open sdrplay_api.dll");
	      return nullptr;
	   }
	}
#else
	Handle		= dlopen ("libusb-1.0" LIBEXT, RTLD_NOW | RTLD_GLOBAL);
	Handle		= dlopen ("libsdrplay_api" LIBEXT, RTLD_NOW);
	if (Handle == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "error report %s", dlerror());
	   return nullptr;
	}
#endif
	return Handle;
}

void	sdrplayHandler_v3::releaseLibrary () {
#if IS_WINDOWS
        FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}

bool	sdrplayHandler_v3::loadFunctions () {
	sdrplay_api_Open	= (sdrplay_api_Open_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Open");
	if ((void *)sdrplay_api_Open == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Open");
	   return false;
	}

	sdrplay_api_Close	= (sdrplay_api_Close_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Close");
	if (sdrplay_api_Close == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Close");
	   return false;
	}

	sdrplay_api_ApiVersion	= (sdrplay_api_ApiVersion_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ApiVersion");
	if (sdrplay_api_ApiVersion == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_ApiVersion");
	   return false;
	}

	sdrplay_api_LockDeviceApi	= (sdrplay_api_LockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_LockDeviceApi");
	if (sdrplay_api_LockDeviceApi == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_LockdeviceApi");
	   return false;
	}

	sdrplay_api_UnlockDeviceApi	= (sdrplay_api_UnlockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_UnlockDeviceApi");
	if (sdrplay_api_UnlockDeviceApi == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_UnlockdeviceApi");
	   return false;
	}

	sdrplay_api_GetDevices		= (sdrplay_api_GetDevices_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDevices");
	if (sdrplay_api_GetDevices == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetDevices");
	   return false;
	}

	sdrplay_api_SelectDevice	= (sdrplay_api_SelectDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_SelectDevice");
	if (sdrplay_api_SelectDevice == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_SelectDevice");
	   return false;
	}

	sdrplay_api_ReleaseDevice	= (sdrplay_api_ReleaseDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ReleaseDevice");
	if (sdrplay_api_ReleaseDevice == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_ReleaseDevice");
	   return false;
	}

	sdrplay_api_GetErrorString	= (sdrplay_api_GetErrorString_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetErrorString");
	if (sdrplay_api_GetErrorString == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetErrorString");
	   return false;
	}

	sdrplay_api_GetLastError	= (sdrplay_api_GetLastError_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetLastError");
	if (sdrplay_api_GetLastError == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetLastError");
	   return false;
	}

	sdrplay_api_DebugEnable		= (sdrplay_api_DebugEnable_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_DebugEnable");
	if (sdrplay_api_DebugEnable == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_DebugEnable");
	   return false;
	}

	sdrplay_api_GetDeviceParams	= (sdrplay_api_GetDeviceParams_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDeviceParams");
	if (sdrplay_api_GetDeviceParams == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_GetDeviceParams");
	   return false;
	}

	sdrplay_api_Init		= (sdrplay_api_Init_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Init");
	if (sdrplay_api_Init == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Init");
	   return false;
	}

	sdrplay_api_Uninit		= (sdrplay_api_Uninit_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Uninit");
	if (sdrplay_api_Uninit == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Uninit");
	   return false;
	}

	sdrplay_api_Update		= (sdrplay_api_Update_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Update");
	if (sdrplay_api_Update == nullptr) {
	   log (DEV_PLAYV3, LOG_MIN, "Could not find sdrplay_api_Update");
	   return false;
	}

	return true;
}


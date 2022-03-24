#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-mini
 *
 *    dab-mini is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    dab-mini is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-mini if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"sdrplay-handler.h"

#define MIN_GAIN	20
#define MAX_GAIN	59

	sdrplayHandler::sdrplayHandler  ():
	                                   _I_Buffer (4 * 1024 * 1024) {
int	err;
float	ver;
mir_sdr_DeviceT devDesc [4];

	this	-> GRdB			= 50;
	this	-> lnaGain		= 50;
	this	-> agcMode		= false;

	this	-> inputRate		= Khz (2048);
	libraryLoaded			= false;

#if IS_WINDOWS
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

	wchar_t *libname = (wchar_t *)L"mir_sdr_api.dll";
        Handle  = LoadLibrary (libname);
        if (Handle == NULL) {
	   if (RegOpenKey (HKEY_LOCAL_MACHINE,
	                   TEXT("Software\\MiricsSDR\\API"),
	                   &APIkey) != ERROR_SUCCESS) {
	      fprintf (stderr,
	               "failed to locate API registry entry, error = %d\n",
	               (int)GetLastError());
	      throw (21);
	   }

	   RegQueryValueEx (APIkey,
	                    (wchar_t *)L"Install_Dir",
	                    NULL,
	                    NULL,
	                    (LPBYTE)&APIkeyValue,
	                    (LPDWORD)&APIkeyValue_length);
//	Ok, make 
	   wchar_t *x =
#ifdef	__BITS64__
	         wcscat (APIkeyValue, (wchar_t *)L"\\x64\\mir_sdr_api.dll");
#else
	         wcscat (APIkeyValue, (wchar_t *)L"\\x86\\mir_sdr_api.dll");
#endif
	   RegCloseKey(APIkey);

	   Handle	= LoadLibrary (x);
	   if (Handle == NULL) {
	     fprintf (stderr, "Failed to open mir_sdr_api.dll\n");
	     throw (22);
	   }
	}
#else
	Handle		= dlopen ("libusb-1.0.so", RTLD_NOW | RTLD_GLOBAL);
	Handle		= dlopen ("libmirsdrapi-rsp.so", RTLD_NOW);
	if (Handle == NULL) {
	   fprintf (stderr,  "%s",
	             "we could not load libmirsdrapi-rsp\nIf that is not the device you were expecting, do not worry\n");
	   throw (23);
	}
#endif
	libraryLoaded	= true;

	bool success = loadFunctions ();
	if (!success) {
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (23);
	}

	err		= my_mir_sdr_ApiVersion (&ver);
	if (ver < 2.13) {
	   fprintf (stderr, "please install mir_sdr library >= 2.13\n");
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (24);
	}

//
	my_mir_sdr_GetDevices (devDesc, &numofDevs, uint32_t (4));
	if (numofDevs == 0) {
	   fprintf (stderr, "Sorry, no device found\n");
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (25);
	}

	deviceIndex = 0;
	hwVersion = devDesc [deviceIndex]. hwVer;
	fprintf (stderr, "hwVer = %d\n", hwVersion);
	err = my_mir_sdr_SetDeviceIdx (deviceIndex);
	if (err != mir_sdr_Success) {
	   throw (33);
	}
//
//      we know we are only in the frequency range 175 .. 230 Mhz,
//      so we can rely on a single table for the lna reductions.
	switch (hwVersion) {
	   case 1:              // old RSP
	      lnaGainMax	= 3;
	      nrBits		= 12;
	      denominator	= 2048;
	      break;
	   case 2:
	      lnaGainMax	= 8;
	      nrBits		= 14;
	      denominator	= 8192;
	      break;
	   case 3:
	      lnaGainMax	= 9;
	      nrBits		= 14;
	      denominator	= 8192;
	      break;
	   default:			// RSP1A
	      lnaGainMax	= 9;
	      nrBits		= 14;
	      denominator	= 2048;
	      break;
	}

	if (hwVersion == 2) {
	   mir_sdr_ErrT err;
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	   if (err != mir_sdr_Success)
	      fprintf (stderr, "error %d in setting antenna\n", err);
	   
	}

	if (hwVersion == 3) {   // duo
	   err  = my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	   if (err != mir_sdr_Success)
	      fprintf (stderr, "error %d in setting of rspDuo\n", err);
	}

	running. store (false);
}

	sdrplayHandler::~sdrplayHandler	(void) {
	if (!libraryLoaded)	// should not happen
	   return;
	stopReader ();
	
#if IS_WINDOWS
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}
//
//
void	sdrplayHandler::setIfGain (int newGRdB) {
mir_sdr_ErrT    err;

	if (!running. load ())
	   return;

        err     =  my_mir_sdr_RSP_SetGr (newGRdB * MAX_GAIN / 100 + MIN_GAIN, lnaGain * lnaGainMax / 100, 1, 0);
        if (err != mir_sdr_Success) {
           fprintf (stderr, "Error at set_ifgain %s (%d %d)\n",
                             errorCodes (err). toLatin1 (). data (),
	                     newGRdB, lnaGain);
	   return;
	} else
		GRdB = newGRdB;
	emit configurationChanged();
}

void    sdrplayHandler::setLnaGain (int lnaState) {
mir_sdr_ErrT err;

	if (!running. load ())
	   return;

        err     = my_mir_sdr_AgcControl (agcMode ? mir_sdr_AGC_100HZ :
                                         mir_sdr_AGC_DISABLE,
                                         -30, 0, 0, 0, 0, lnaState * lnaGainMax / 100);
        if (err != mir_sdr_Success) {
           fprintf (stderr, "Error at set_lnagainReduction %s\n",
                               errorCodes (err). toLatin1 (). data ());
	   return;
	} else
		lnaGain = lnaState;
	emit configurationChanged();
}

void    sdrplayHandler::setAgcControl (int dummy) {
	agcMode = (dummy > 0);
        my_mir_sdr_AgcControl (agcMode ? mir_sdr_AGC_100HZ :
                                         mir_sdr_AGC_DISABLE,
                               -30,
                               0, 0, 0, 0, lnaGain * lnaGainMax / 100);
	emit configurationChanged();
}

static
void myStreamCallback (int16_t		*xi,
	               int16_t		*xq,
	               uint32_t		firstSampleNum, 
	               int32_t		grChanged,
	               int32_t		rfChanged,
	               int32_t		fsChanged,
	               uint32_t		numSamples,
	               uint32_t		reset,
	               uint32_t		hwRemoved,
	               void		*cbContext) {
int16_t	i;
sdrplayHandler	*p	= static_cast<sdrplayHandler *> (cbContext);
float	denominator	= p -> denominator;
std::complex<float> *localBuf =
	   (std::complex<float> *)alloca (numSamples * sizeof (std::complex<float>));

	if (reset || hwRemoved)
	   return;

	for (i = 0; i <  (int)numSamples; i ++)
	   localBuf [i] = std::complex<float> (float (xi [i]) / denominator,
	                                       float (xq [i]) / denominator);
	p ->  _I_Buffer. putDataIntoBuffer (localBuf, numSamples);
	(void)	firstSampleNum;
	(void)	grChanged;
	(void)	rfChanged;
	(void)	fsChanged;
	(void)	reset;
}

void	myGainChangeCallback (uint32_t	gRdB,
	                      uint32_t	lnaGRdB,
	                      void	*cbContext) {
//	fprintf (stderr, "GainChangeCallback gives %X\n", gRdB);
	(void)gRdB;
	(void)lnaGRdB;	
	(void)cbContext;
}

bool	sdrplayHandler::restartReader	(int32_t frequency) {
int	gRdBSystem;
int	samplesPerPacket;
mir_sdr_ErrT	err;

	if (running. load ())
	   return true;

	vfoFrequency	= frequency;
	err	= my_mir_sdr_StreamInit (&GRdB,
	                                 double (inputRate) / MHz (1),
	                                 double (frequency) / Mhz (1),
	                                 mir_sdr_BW_1_536,
	                                 mir_sdr_IF_Zero,
	                                 lnaGain * lnaGainMax / 100,
	                                 &gRdBSystem,
	                                 mir_sdr_USE_RSP_SET_GR,
	                                 &samplesPerPacket,
	                                 (mir_sdr_StreamCallback_t)myStreamCallback,
	                                 (mir_sdr_GainChangeCallback_t)myGainChangeCallback,
	                                 this);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error code = %d\n", err);
	   return false;
	}
	
        if (agcMode) {
           my_mir_sdr_AgcControl (mir_sdr_AGC_100HZ,
                                  -30,
                                  0, 0, 0, 0, lnaGain * lnaGainMax / 100);
        }
	else {
           my_mir_sdr_AgcControl (mir_sdr_AGC_DISABLE,
                                  -30,
                                  0, 0, 0, 0, lnaGain * lnaGainMax / 100);
	}

	err		= my_mir_sdr_SetDcMode (4, 1);
	err		= my_mir_sdr_SetDcTrackTime (63);
	running. store (true);
	return true;
}

void	sdrplayHandler::stopReader	(void) {
	if (!running. load ())
	   return;

	my_mir_sdr_StreamUninit	();
	running. store (false);
	_I_Buffer. FlushRingBuffer ();
}

//
//	The brave old getSamples. For the sdrplay, we get
//	size still in I/Q pairs
int32_t	sdrplayHandler::getSamples (std::complex<float> *V, int32_t size) { 
	return _I_Buffer. getDataFromBuffer (V, size);
}

int32_t	sdrplayHandler::Samples	(void) {
	return _I_Buffer. GetRingBufferReadAvailable ();
}

void	sdrplayHandler::resetBuffer	(void) {
	_I_Buffer. FlushRingBuffer ();
}

int16_t	sdrplayHandler::bitDepth	(void) {
	return nrBits;
}

int32_t sdrplayHandler::getRate (void) {
        return inputRate;
}

bool	sdrplayHandler::loadFunctions	(void) {
	my_mir_sdr_StreamInit	= (pfn_mir_sdr_StreamInit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamInit");
	if (my_mir_sdr_StreamInit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamInit\n");
	   return false;
	}

	my_mir_sdr_StreamUninit	= (pfn_mir_sdr_StreamUninit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamUninit");
	if (my_mir_sdr_StreamUninit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamUninit\n");
	   return false;
	}

	my_mir_sdr_SetRf	= (pfn_mir_sdr_SetRf)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetRf");
	if (my_mir_sdr_SetRf == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetRf\n");
	   return false;
	}

	my_mir_sdr_SetFs	= (pfn_mir_sdr_SetFs)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetFs");
	if (my_mir_sdr_SetFs == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetFs\n");
	   return false;
	}

	my_mir_sdr_SetGr	= (pfn_mir_sdr_SetGr)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGr");
	if (my_mir_sdr_SetGr == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGr\n");
	   return false;
	}

	my_mir_sdr_RSP_SetGr    = (pfn_mir_sdr_RSP_SetGr)
	                    GETPROCADDRESS (Handle, "mir_sdr_RSP_SetGr");
	if (my_mir_sdr_RSP_SetGr == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_RSP_SetGr\n");
	   return false;
	}

	my_mir_sdr_SetGrParams	= (pfn_mir_sdr_SetGrParams)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGrParams");
	if (my_mir_sdr_SetGrParams == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGrParams\n");
	   return false;
	}

	my_mir_sdr_SetDcMode	= (pfn_mir_sdr_SetDcMode)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcMode");
	if (my_mir_sdr_SetDcMode == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcMode\n");
	   return false;
	}

	my_mir_sdr_SetDcTrackTime	= (pfn_mir_sdr_SetDcTrackTime)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcTrackTime");
	if (my_mir_sdr_SetDcTrackTime == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcTrackTime\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdateSampleNum = (pfn_mir_sdr_SetSyncUpdateSampleNum)
	               GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdateSampleNum");
	if (my_mir_sdr_SetSyncUpdateSampleNum == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdateSampleNum\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdatePeriod	= (pfn_mir_sdr_SetSyncUpdatePeriod)
	                GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdatePeriod");
	if (my_mir_sdr_SetSyncUpdatePeriod == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdatePeriod\n");
	   return false;
	}

	my_mir_sdr_ApiVersion	= (pfn_mir_sdr_ApiVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_ApiVersion");
	if (my_mir_sdr_ApiVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ApiVersion\n");
	   return false;
	}

	my_mir_sdr_AgcControl	= (pfn_mir_sdr_AgcControl)
	                GETPROCADDRESS (Handle, "mir_sdr_AgcControl");
	if (my_mir_sdr_AgcControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_AgcControl\n");
	   return false;
	}

	my_mir_sdr_Reinit	= (pfn_mir_sdr_Reinit)
	                GETPROCADDRESS (Handle, "mir_sdr_Reinit");
	if (my_mir_sdr_Reinit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_Reinit\n");
	   return false;
	}

	my_mir_sdr_SetPpm	= (pfn_mir_sdr_SetPpm)
	                GETPROCADDRESS (Handle, "mir_sdr_SetPpm");
	if (my_mir_sdr_SetPpm == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetPpm\n");
	   return false;
	}

	my_mir_sdr_DebugEnable	= (pfn_mir_sdr_DebugEnable)
	                GETPROCADDRESS (Handle, "mir_sdr_DebugEnable");
	if (my_mir_sdr_DebugEnable == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DebugEnable\n");
	   return false;
	}

	my_mir_sdr_rspDuo_TunerSel = (pfn_mir_sdr_rspDuo_TunerSel)
	               GETPROCADDRESS (Handle, "mir_sdr_rspDuo_TunerSel");
	if (my_mir_sdr_rspDuo_TunerSel == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_rspDuo_TunerSel\n");
	   return false;
	}

	my_mir_sdr_DCoffsetIQimbalanceControl	=
	                     (pfn_mir_sdr_DCoffsetIQimbalanceControl)
	                GETPROCADDRESS (Handle, "mir_sdr_DCoffsetIQimbalanceControl");
	if (my_mir_sdr_DCoffsetIQimbalanceControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DCoffsetIQimbalanceControl\n");
	   return false;
	}


	my_mir_sdr_ResetUpdateFlags	= (pfn_mir_sdr_ResetUpdateFlags)
	                GETPROCADDRESS (Handle, "mir_sdr_ResetUpdateFlags");
	if (my_mir_sdr_ResetUpdateFlags == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ResetUpdateFlags\n");
	   return false;
	}

	my_mir_sdr_GetDevices		= (pfn_mir_sdr_GetDevices)
	                GETPROCADDRESS (Handle, "mir_sdr_GetDevices");
	if (my_mir_sdr_GetDevices == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetDevices");
	   return false;
	}

	my_mir_sdr_GetCurrentGain	= (pfn_mir_sdr_GetCurrentGain)
	                GETPROCADDRESS (Handle, "mir_sdr_GetCurrentGain");
	if (my_mir_sdr_GetCurrentGain == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetCurrentGain");
	   return false;
	}

	my_mir_sdr_GetHwVersion	= (pfn_mir_sdr_GetHwVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_GetHwVersion");
	if (my_mir_sdr_GetHwVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetHwVersion");
	   return false;
	}

	my_mir_sdr_RSPII_AntennaControl	= (pfn_mir_sdr_RSPII_AntennaControl)
	                GETPROCADDRESS (Handle, "mir_sdr_RSPII_AntennaControl");
	if (my_mir_sdr_RSPII_AntennaControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_RSPII_AntennaControl");
	   return false;
	}

	my_mir_sdr_SetDeviceIdx	= (pfn_mir_sdr_SetDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_SetDeviceIdx");
	if (my_mir_sdr_SetDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDeviceIdx");
	   return false;
	}

	my_mir_sdr_ReleaseDeviceIdx	= (pfn_mir_sdr_ReleaseDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_ReleaseDeviceIdx");
	if (my_mir_sdr_ReleaseDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ReleaseDeviceIdx");
	   return false;
	}

	return true;
}


QString	sdrplayHandler::errorCodes (mir_sdr_ErrT err) {
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
	      return "???";
	}
}

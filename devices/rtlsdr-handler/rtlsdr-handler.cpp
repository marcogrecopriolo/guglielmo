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
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 * 	This particular driver is a very simple wrapper around the
 * 	librtlsdr.  In order to keep things simple, we dynamically
 * 	load the dll (or .so). The librtlsdr is osmocom software and all rights
 * 	are greatly acknowledged
 */

#include	<QThread>
#include	"rtlsdr-handler.h"
#include	"rtl-sdr.h"
#include	"logging.h"

#define DEV_RTLSDR LOG_DEV
#if IS_WINDOWS
#define	GETPROCADDRESS	GetProcAddress
#else
#define	GETPROCADDRESS	dlsym
#endif

#define	READLEN_DEFAULT	8192
//
//	For the callback, we do need some environment which
//	is passed through the ctx parameter
//
//	This is the user-side call back function
//	ctx is the calling task
static
void	RTLSDRCallBack (uint8_t *buf, uint32_t len, void *ctx) {
rtlsdrHandler	*theStick	= (rtlsdrHandler *)ctx;

	if ((theStick == NULL) || (len != READLEN_DEFAULT))
	   return;

	(void)theStick -> _I_Buffer. putDataIntoBuffer (buf, len);
}
//
//	for handling the events in libusb, we need a controlthread
//	whose sole purpose is to process the rtlsdr_read_async function
//	from the lib.
class	dll_driver : public QThread {
private:
	rtlsdrHandler	*theStick;
public:

	dll_driver (rtlsdrHandler *d) {
	theStick	= d;
	start ();
	}

	~dll_driver (void) {
	}

private:
virtual void	run (void) {
	(theStick -> rtlsdr_read_async) (theStick -> device,
	                          (rtlsdr_read_async_cb_t)&RTLSDRCallBack,
	                          (void *)theStick,
	                          0,
	                          READLEN_DEFAULT);
	}
};
//
//	Our wrapper is a simple classs
	rtlsdrHandler::rtlsdrHandler ():
	                                 _I_Buffer (4 * 1024 * 1024) {
int16_t	deviceCount;
int32_t	r;
int	i;
char    *gainsString;

	agcControl		= false;
	ifGain			= 50;
	inputRate		= 2048000;
	libraryLoaded		= false;
	open			= false;
	workerHandle		= NULL;
	vfoFrequency		= KHz (22000);	// just a dummy
	gains			= NULL;

#if IS_WINDOWS
	const char *libraryString = "rtlsdr.dll";
	Handle		= LoadLibraryA (libraryString);
#else
	const char *libraryString = "librtlsdr" LIBEXT;
	Handle		= dlopen (libraryString, RTLD_NOW);
#endif

	if (Handle == NULL) {
#if IS_WINDOWS
	   log (DEV_RTLSDR, LOG_MIN, "failed to open %s Error = %li", libraryString, GetLastError());
#else
	   log (DEV_RTLSDR, LOG_MIN, "failed to open %s Error = %s", libraryString, dlerror());
#endif
	   throw (20);
	}

	libraryLoaded	= true;
	if (!load_rtlFunctions ()) {
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (21);
	}
//
//	Ok, from here we have the library functions accessible
	deviceCount 		= this -> rtlsdr_get_device_count ();
	if (deviceCount == 0) {
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (22);
	}

//	OK, now open the hardware
	r			= this -> rtlsdr_open (&device, 0);
	if (r < 0) {
	   log (DEV_RTLSDR, LOG_MIN, "opening device failed");
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (23);
	}

	open			= true;
	r			= this -> rtlsdr_set_sample_rate (device,
	                                                          inputRate);
	if (r < 0) {
	   log (DEV_RTLSDR, LOG_MIN, "setting samplerate failed");
	   rtlsdr_close (device);
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (24);
	}

	r			= this -> rtlsdr_get_sample_rate (device);
	log (DEV_RTLSDR, LOG_MIN, "samplerate set to %d", r);

	gainsCount   = rtlsdr_get_tuner_gains (device, NULL);
	gains	     = new int [gainsCount];
	gainsString  = new char[gainsCount *5 + 1];
	*gainsString = '\0';
	gainsCount   = rtlsdr_get_tuner_gains (device, gains);
	for (i = gainsCount; i > 0; i--) {
	   sprintf (gainsString + strlen(gainsString), "%.1f ", gains [i - 1] / 10.0);
	}
	log (DEV_RTLSDR, LOG_MIN, "supported gain values (%d): %s", gainsCount, gainsString);
	delete gainsString;

	if (agcControl)
	   rtlsdr_set_agc_mode (device, 1);
	else
	   rtlsdr_set_agc_mode (device, 0);
	rtlsdr_set_tuner_gain	(device, gains [(int)(ifGain * gainsCount / 100)]);
}

	rtlsdrHandler::~rtlsdrHandler	(void) {
	if (Handle == NULL) {	// nothing achieved earlier on
	   return;
	}
	
	if (!open) {	
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   return;
	}
//
//	library was open
	stopReader ();
	
	this -> rtlsdr_close (device);
#if IS_WINDOWS
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
	if (gains != NULL)
	   delete[] gains;

}
//
//
bool	rtlsdrHandler::restartReader	(int32_t frequency) {
int32_t	r;

	if (workerHandle != NULL)
	   return true;

	_I_Buffer. FlushRingBuffer ();
	r = this -> rtlsdr_reset_buffer (device);
	if (r < 0)
	   return false;

	vfoFrequency	= frequency;
//	update_gainSettings	(frequency / MHz (1));
	this -> rtlsdr_set_center_freq (device, frequency);
	workerHandle	= new dll_driver (this);
	rtlsdr_set_agc_mode (device, agcControl);
	rtlsdr_set_tuner_gain (device,
	                  gains [(int)(/* gainControl -> value () */ ifGain * gainsCount / 100)]);
	return true;
}

void	rtlsdrHandler::stopReader	(void) {
	if (workerHandle == NULL)
	   return;
	if (workerHandle != NULL) { // we are running
//	   record_gainSettings	(vfoFrequency / MHz (1));
	   this -> rtlsdr_cancel_async (device);
	   if (workerHandle != NULL) {
	      while (!workerHandle -> isFinished ()) 
	         usleep (100);
	      delete	workerHandle;
	   }
	}
	workerHandle = NULL;
}
//
//	when selecting  the gain from a table, use the table value
void	rtlsdrHandler::setIfGain (int gain) {
	log (DEV_RTLSDR, LOG_MIN, "gain will be set %d to %d",
	                     gain, gains [gain * gainsCount / 100]);
	ifGain = gain;
	rtlsdr_set_tuner_gain (device,
	                        gains [gain * gainsCount / 100]);
}
//
void	rtlsdrHandler::setAgcControl	(int v) {
	agcControl = (v != 0);
	rtlsdr_set_agc_mode (device, v);
	rtlsdr_set_tuner_gain (device,
	             gains [(int)(ifGain * gainsCount / 100)]);
}

//
//	we only have 8 bits, so rather than doing a float division to get
//	the float value we want, we precompute the possibilities
static 
float convTable [] = {
 -128 / 128.0 , -127 / 128.0 , -126 / 128.0 , -125 / 128.0 , -124 / 128.0 , -123 / 128.0 , -122 / 128.0 , -121 / 128.0 , -120 / 128.0 , -119 / 128.0 , -118 / 128.0 , -117 / 128.0 , -116 / 128.0 , -115 / 128.0 , -114 / 128.0 , -113 / 128.0 
, -112 / 128.0 , -111 / 128.0 , -110 / 128.0 , -109 / 128.0 , -108 / 128.0 , -107 / 128.0 , -106 / 128.0 , -105 / 128.0 , -104 / 128.0 , -103 / 128.0 , -102 / 128.0 , -101 / 128.0 , -100 / 128.0 , -99 / 128.0 , -98 / 128.0 , -97 / 128.0 
, -96 / 128.0 , -95 / 128.0 , -94 / 128.0 , -93 / 128.0 , -92 / 128.0 , -91 / 128.0 , -90 / 128.0 , -89 / 128.0 , -88 / 128.0 , -87 / 128.0 , -86 / 128.0 , -85 / 128.0 , -84 / 128.0 , -83 / 128.0 , -82 / 128.0 , -81 / 128.0 
, -80 / 128.0 , -79 / 128.0 , -78 / 128.0 , -77 / 128.0 , -76 / 128.0 , -75 / 128.0 , -74 / 128.0 , -73 / 128.0 , -72 / 128.0 , -71 / 128.0 , -70 / 128.0 , -69 / 128.0 , -68 / 128.0 , -67 / 128.0 , -66 / 128.0 , -65 / 128.0 
, -64 / 128.0 , -63 / 128.0 , -62 / 128.0 , -61 / 128.0 , -60 / 128.0 , -59 / 128.0 , -58 / 128.0 , -57 / 128.0 , -56 / 128.0 , -55 / 128.0 , -54 / 128.0 , -53 / 128.0 , -52 / 128.0 , -51 / 128.0 , -50 / 128.0 , -49 / 128.0 
, -48 / 128.0 , -47 / 128.0 , -46 / 128.0 , -45 / 128.0 , -44 / 128.0 , -43 / 128.0 , -42 / 128.0 , -41 / 128.0 , -40 / 128.0 , -39 / 128.0 , -38 / 128.0 , -37 / 128.0 , -36 / 128.0 , -35 / 128.0 , -34 / 128.0 , -33 / 128.0 
, -32 / 128.0 , -31 / 128.0 , -30 / 128.0 , -29 / 128.0 , -28 / 128.0 , -27 / 128.0 , -26 / 128.0 , -25 / 128.0 , -24 / 128.0 , -23 / 128.0 , -22 / 128.0 , -21 / 128.0 , -20 / 128.0 , -19 / 128.0 , -18 / 128.0 , -17 / 128.0 
, -16 / 128.0 , -15 / 128.0 , -14 / 128.0 , -13 / 128.0 , -12 / 128.0 , -11 / 128.0 , -10 / 128.0 , -9 / 128.0 , -8 / 128.0 , -7 / 128.0 , -6 / 128.0 , -5 / 128.0 , -4 / 128.0 , -3 / 128.0 , -2 / 128.0 , -1 / 128.0 
, 0 / 128.0 , 1 / 128.0 , 2 / 128.0 , 3 / 128.0 , 4 / 128.0 , 5 / 128.0 , 6 / 128.0 , 7 / 128.0 , 8 / 128.0 , 9 / 128.0 , 10 / 128.0 , 11 / 128.0 , 12 / 128.0 , 13 / 128.0 , 14 / 128.0 , 15 / 128.0 
, 16 / 128.0 , 17 / 128.0 , 18 / 128.0 , 19 / 128.0 , 20 / 128.0 , 21 / 128.0 , 22 / 128.0 , 23 / 128.0 , 24 / 128.0 , 25 / 128.0 , 26 / 128.0 , 27 / 128.0 , 28 / 128.0 , 29 / 128.0 , 30 / 128.0 , 31 / 128.0 
, 32 / 128.0 , 33 / 128.0 , 34 / 128.0 , 35 / 128.0 , 36 / 128.0 , 37 / 128.0 , 38 / 128.0 , 39 / 128.0 , 40 / 128.0 , 41 / 128.0 , 42 / 128.0 , 43 / 128.0 , 44 / 128.0 , 45 / 128.0 , 46 / 128.0 , 47 / 128.0 
, 48 / 128.0 , 49 / 128.0 , 50 / 128.0 , 51 / 128.0 , 52 / 128.0 , 53 / 128.0 , 54 / 128.0 , 55 / 128.0 , 56 / 128.0 , 57 / 128.0 , 58 / 128.0 , 59 / 128.0 , 60 / 128.0 , 61 / 128.0 , 62 / 128.0 , 63 / 128.0 
, 64 / 128.0 , 65 / 128.0 , 66 / 128.0 , 67 / 128.0 , 68 / 128.0 , 69 / 128.0 , 70 / 128.0 , 71 / 128.0 , 72 / 128.0 , 73 / 128.0 , 74 / 128.0 , 75 / 128.0 , 76 / 128.0 , 77 / 128.0 , 78 / 128.0 , 79 / 128.0 
, 80 / 128.0 , 81 / 128.0 , 82 / 128.0 , 83 / 128.0 , 84 / 128.0 , 85 / 128.0 , 86 / 128.0 , 87 / 128.0 , 88 / 128.0 , 89 / 128.0 , 90 / 128.0 , 91 / 128.0 , 92 / 128.0 , 93 / 128.0 , 94 / 128.0 , 95 / 128.0 
, 96 / 128.0 , 97 / 128.0 , 98 / 128.0 , 99 / 128.0 , 100 / 128.0 , 101 / 128.0 , 102 / 128.0 , 103 / 128.0 , 104 / 128.0 , 105 / 128.0 , 106 / 128.0 , 107 / 128.0 , 108 / 128.0 , 109 / 128.0 , 110 / 128.0 , 111 / 128.0 
, 112 / 128.0 , 113 / 128.0 , 114 / 128.0 , 115 / 128.0 , 116 / 128.0 , 117 / 128.0 , 118 / 128.0 , 119 / 128.0 , 120 / 128.0 , 121 / 128.0 , 122 / 128.0 , 123 / 128.0 , 124 / 128.0 , 125 / 128.0 , 126 / 128.0 , 127 / 128.0 };

//
//	The brave old getSamples. For the dab stick, we get
//	size samples: still in I/Q pairs, but we have to convert the data from
//	uint8_t to DSPCOMPLEX *
int32_t	rtlsdrHandler::getSamples (std::complex<float> *V, int32_t size) { 
int32_t	amount, i;
uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));
//
	amount = _I_Buffer. getDataFromBuffer (tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	    V [i] = std::complex<float>
	                    (convTable [tempBuffer [2 * i]],
	                     convTable [tempBuffer [2 * i + 1]]);;
	return amount / 2;
}

int32_t	rtlsdrHandler::Samples	(void) {
	return _I_Buffer. GetRingBufferReadAvailable () / 2;
}
//
bool	rtlsdrHandler::load_rtlFunctions (void) {
//
//	link the required procedures
	rtlsdr_open	= (pfnrtlsdr_open)
	                       GETPROCADDRESS (Handle, "rtlsdr_open");
	if (rtlsdr_open == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_open");
	   return false;
	}
	rtlsdr_close	= (pfnrtlsdr_close)
	                     GETPROCADDRESS (Handle, "rtlsdr_close");
	if (rtlsdr_close == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_close");
	   return false;
	}

	rtlsdr_set_sample_rate =
	    (pfnrtlsdr_set_sample_rate)GETPROCADDRESS (Handle, "rtlsdr_set_sample_rate");
	if (rtlsdr_set_sample_rate == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_sample_rate");
	   return false;
	}

	rtlsdr_get_sample_rate	=
	    (pfnrtlsdr_get_sample_rate)GETPROCADDRESS (Handle, "rtlsdr_get_sample_rate");
	if (rtlsdr_get_sample_rate == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_sample_rate");
	   return false;
	}

	rtlsdr_get_tuner_gains		= (pfnrtlsdr_get_tuner_gains)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_tuner_gains");
	if (rtlsdr_get_tuner_gains == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_tuner_gains");
	   return false;
	}


	rtlsdr_set_tuner_gain_mode	= (pfnrtlsdr_set_tuner_gain_mode)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_tuner_gain_mode");
	if (rtlsdr_set_tuner_gain_mode == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_tuner_gain_mode");
	   return false;
	}

	rtlsdr_set_agc_mode	= (pfnrtlsdr_set_agc_mode)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_agc_mode");
	if (rtlsdr_set_agc_mode == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_agc_mode");
	   return false;
	}

	rtlsdr_set_tuner_gain	= (pfnrtlsdr_set_tuner_gain)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_tuner_gain");
	if (rtlsdr_set_tuner_gain == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Cound not find rtlsdr_set_tuner_gain");
	   return false;
	}

	rtlsdr_get_tuner_gain	= (pfnrtlsdr_get_tuner_gain)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_tuner_gain");
	if (rtlsdr_get_tuner_gain == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_tuner_gain");
	   return false;
	}
	rtlsdr_set_center_freq	= (pfnrtlsdr_set_center_freq)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_center_freq");
	if (rtlsdr_set_center_freq == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_center_freq");
	   return false;
	}

	rtlsdr_get_center_freq	= (pfnrtlsdr_get_center_freq)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_center_freq");
	if (rtlsdr_get_center_freq == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_center_freq");
	   return false;
	}

	rtlsdr_reset_buffer	= (pfnrtlsdr_reset_buffer)
	                     GETPROCADDRESS (Handle, "rtlsdr_reset_buffer");
	if (rtlsdr_reset_buffer == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_reset_buffer");
	   return false;
	}

	rtlsdr_read_async	= (pfnrtlsdr_read_async)
	                     GETPROCADDRESS (Handle, "rtlsdr_read_async");
	if (rtlsdr_read_async == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Cound not find rtlsdr_read_async");
	   return false;
	}

	rtlsdr_get_device_count	= (pfnrtlsdr_get_device_count)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_device_count");
	if (rtlsdr_get_device_count == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_device_count");
	   return false;
	}

	rtlsdr_cancel_async	= (pfnrtlsdr_cancel_async)
	                     GETPROCADDRESS (Handle, "rtlsdr_cancel_async");
	if (rtlsdr_cancel_async == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_cancel_async");
	   return false;
	}

	rtlsdr_set_direct_sampling = (pfnrtlsdr_set_direct_sampling)
	                  GETPROCADDRESS (Handle, "rtlsdr_set_direct_sampling");
	if (rtlsdr_set_direct_sampling == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_direct_sampling");
	   return false;
	}

	rtlsdr_set_freq_correction = (pfnrtlsdr_set_freq_correction)
	                  GETPROCADDRESS (Handle, "rtlsdr_set_freq_correction");
	if (rtlsdr_set_freq_correction == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_freq_correction");
	   return false;
	}
	
	rtlsdr_get_device_name = (pfnrtlsdr_get_device_name)
	                  GETPROCADDRESS (Handle, "rtlsdr_get_device_name");
	if (rtlsdr_get_device_name == NULL) {
	   log (DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_device_name");
	   return false;
	}

	log (DEV_RTLSDR, LOG_MIN, "functions seem to be loaded");
	return true;
}

void	rtlsdrHandler::resetBuffer (void) {
	_I_Buffer. FlushRingBuffer ();
}

int16_t	rtlsdrHandler::bitDepth	(void) {
	return 8;
}

int32_t rtlsdrHandler::getRate  (void) {
        return inputRate;
}

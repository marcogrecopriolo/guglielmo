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
**
 *  IW0HDV Extio
 *
 *  Copyright 2015 by Andrea Montefusco IW0HDV
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 *	recoding, taking parts and extending for the airspyHandler interface
 *	for the Qt-DAB program
 *	jan van Katwijk
 *	Lazy Chair Computing
 */

#include	"airspy-handler.h"
#include	"airspyfilter.h"
#include 	"constants.h"
#include	"logging.h"

#define DEV_AIRSPY LOG_DEV

static
const	int	EXTIO_NS	=  8192;
static
const	int	EXTIO_BASE_TYPE_SIZE = sizeof (float);
static
const	int	MAX_GAIN	=  21;

	airspyHandler::airspyHandler () {
int	result, i;
int	distance	= 10000000;
std::vector <uint32_t> sampleRates;
uint32_t samplerateCount;
char    *samplerateString;

	agcControl		= false;
	ifGain			= 50;
	filter			= NULL;
	device			= 0;
	serialNumber		= 0;
	theBuffer		= NULL;
#if IS_WINDOWS
	const char *libraryString = "airspy.dll";
	Handle		= LoadLibraryA (libraryString);
#else
	const char *libraryString = "libairspy" LIBEXT;
	Handle		= dlopen (libraryString, RTLD_LAZY);
#endif

	if (Handle == NULL) {
#if IS_WINDOWS
	   log (DEV_AIRSPY, LOG_MIN, "failed to open %s - Error = %li", libraryString, GetLastError ());
#else
	   log (DEV_AIRSPY, LOG_MIN, "failed to open %s - Error = %s", libraryString, dlerror ());
#endif
	   throw (20);
	}

	libraryLoaded	= true;

	if (!load_airspyFunctions ()) {
	   log (DEV_AIRSPY, LOG_MIN, "problem in loading functions");
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
	
#endif
	}
//
	strcpy (serial,"");
	result = this -> my_airspy_init ();
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "my_airspy_init () failed: %s (%d)",
	             my_airspy_error_name((airspy_error)result), result);
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (21);
	}

	result = my_airspy_open (&device);
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "my_airpsy_open () failed: %s (%d)",
	             my_airspy_error_name ((airspy_error)result), result);
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (22);
	}

	(void) my_airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
	(void) my_airspy_get_samplerates (device, &samplerateCount, 0);
	sampleRates. resize (samplerateCount);
	my_airspy_get_samplerates (device,
	                            sampleRates. data (), samplerateCount);

	selectedRate	= 0;
	samplerateString  = new char[samplerateCount *10 + 1];
	*samplerateString = '\0';
	for (i = 0; i < (int)samplerateCount; i ++) {
	   sprintf (samplerateString + strlen(samplerateString), "%d ", sampleRates [i]);
	   if (abs ((int)sampleRates [i] - 2048000) < distance) {
	      distance	= abs ((int)sampleRates [i] - 2048000);
	      selectedRate = sampleRates [i];
	   }
	}
	log (DEV_AIRSPY, LOG_MIN, "%d samplerates are supported, %s", samplerateCount, samplerateString); 

	if (selectedRate == 0) {
	   log (DEV_AIRSPY, LOG_MIN, "no sample rate selected");
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (23);
	}
	else
	   log (DEV_AIRSPY, LOG_MIN, "selected samplerate = %d", selectedRate);

	result = my_airspy_set_samplerate (device, selectedRate);
	if (result != AIRSPY_SUCCESS) {
           log (DEV_AIRSPY, LOG_MIN, "airspy_set_samplerate() failed: %s (%d)",
	             my_airspy_error_name ((enum airspy_error)result), result);
#if IS_WINDOWS
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (24);
	}

	filtering 	= 0;
	int filterDegree = 9;
//
//	if we apply filtering it is using a symmetric lowpass filter
	filter		= new airspyFilter (filterDegree,
	                                        1024000, selectedRate);

//	The sizes of the mapTables follow from the input and output rate
//	(selectedRate / 1000) vs (2048000 / 1000)
//	so we end up with buffers with 1 msec content
	convBufferSize		= selectedRate / 1000;
	for (i = 0; i < 2048; i ++) {
	   float inVal	= float (selectedRate / 1000);
	   mapTable_int [i]	=  int (floor (i * (inVal / 2048.0)));
	   mapTable_float [i]	= i * (inVal / 2048.0) - mapTable_int [i];
	}
	convIndex	= 0;
	convBuffer. resize (convBufferSize + 1);

	theBuffer	= new RingBuffer<std::complex<float>>
	                                                    (256 * 1024);
	my_airspy_set_sensitivity_gain (device,
	                    ifGain * MAX_GAIN / GAIN_SCALE);

	my_airspy_set_mixer_agc (device, agcControl? 1 : 0);
}

	airspyHandler::~airspyHandler (void) {
	if (device != NULL) {
	   int result = my_airspy_stop_rx (device);
	   if (result != AIRSPY_SUCCESS) {
	      log (DEV_AIRSPY, LOG_MIN, "my_airspy_stop_rx () failed: %s (%d)",
	             my_airspy_error_name((airspy_error)result), result);
	   }

	   result = my_airspy_close (device);
	   if (result != AIRSPY_SUCCESS) {
	      log (DEV_AIRSPY, LOG_MIN, "airspy_close () failed: %s (%d)",
	             my_airspy_error_name((airspy_error)result), result);
	   }
	}


	if (filter	!= NULL)
	   delete filter;
	if (Handle == NULL) {
	   return;	// nothing achieved earlier
	}
	my_airspy_exit ();
#if IS_WINDOWS
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
err:
	if (theBuffer != NULL)
	   delete theBuffer;
}

bool	airspyHandler::restartReader	(int32_t frequency) {
int	result;
int32_t	bufSize	= EXTIO_NS * EXTIO_BASE_TYPE_SIZE * 2;
	if (running. load ())
	   return true;

	theBuffer	-> FlushRingBuffer ();

	vfoFrequency	= frequency;

	result = my_airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "my_airspy_set_sample_type () failed: %s (%d)",
	            my_airspy_error_name ((airspy_error)result), result);
	   return false;
	}


	my_airspy_set_freq (device, frequency);
	my_airspy_set_sensitivity_gain (device,
	                    ifGain * MAX_GAIN / GAIN_SCALE);
	result = my_airspy_set_mixer_agc (device, 
	                                  agcControl? 1 : 0);
	
	result = my_airspy_start_rx (device,
	            (airspy_sample_block_cb_fn)callback, this);
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "my_airspy_start_rx () failed: %s (%d)",
	         my_airspy_error_name((airspy_error)result), result);
	   return false;
	}
//
	running. store (true);
	return true;
}

void	airspyHandler::stopReader (void) {
int	result;

	if (!running. load ())
	   return;
	result = my_airspy_stop_rx (device);

	if (result != AIRSPY_SUCCESS ) 
	   log (DEV_AIRSPY, LOG_MIN, "my_airspy_stop_rx() failed: %s (%d)",
	          my_airspy_error_name ((airspy_error)result), result);

	running. store (false);
}
//
//	Directly copied from the airspy extio dll from Andrea Montefusco
int airspyHandler::callback (airspy_transfer* transfer) {
airspyHandler *p;

	if (!transfer)
	   return 0;		// should not happen
	p = static_cast<airspyHandler *> (transfer -> ctx);

// we read  AIRSPY_SAMPLE_INT16_IQ:
	int32_t bytes_to_write = transfer -> sample_count * sizeof (int16_t) * 2; 
	uint8_t *pt_rx_buffer   = (uint8_t *)transfer->samples;
	p -> data_available (pt_rx_buffer, bytes_to_write);
	return 0;
}

//	called from AIRSPY data callback
//	2*2 = 4 bytes for sample, as per AirSpy USB data stream format
//	we do the rate conversion here, read in groups of 2 * xxx samples
//	and transform them into groups of 2 * 512 samples
int 	airspyHandler::data_available (void *buf, int buf_size) {	
int16_t	*sbuf	= (int16_t *)buf;
int nSamples	= buf_size / (sizeof (int16_t) * 2);
std::complex<float> temp [2048];
int32_t  i, j;

	if (filtering) {
	   for (i = 0; i < nSamples; i ++) {
	      convBuffer [convIndex ++] = filter -> Pass (
	                                        sbuf [2 * i] / (float)2048,
	                                        sbuf [2 * i + 1] / (float)2048);
	      if (convIndex > convBufferSize) {
	         for (j = 0; j < 2048; j ++) {
	            int16_t  inpBase	= mapTable_int [j];
	            float    inpRatio	= mapTable_float [j];
	            temp [j]	= cmul (convBuffer [inpBase + 1], inpRatio) + 
	                             cmul (convBuffer [inpBase], 1 - inpRatio);
	         }

	         theBuffer	-> putDataIntoBuffer (temp, 2048);
//
//	shift the sample at the end to the beginning, it is needed
//	as the starting sample for the next time
	         convBuffer [0] = convBuffer [convBufferSize];
	         convIndex = 1;
	      }
	   }
	}
	else
	for (i = 0; i < nSamples; i ++) {
	   convBuffer [convIndex ++] = std::complex<float> (
	                                     sbuf [2 * i] / (float)2048,
	                                     sbuf [2 * i + 1] / (float)2048);
	   if (convIndex > convBufferSize) {
	      for (j = 0; j < 2048; j ++) {
	         int16_t  inpBase	= mapTable_int [j];
	         float    inpRatio	= mapTable_float [j];
	         temp [j]	= cmul (convBuffer [inpBase + 1], inpRatio) + 
	                          cmul (convBuffer [inpBase], 1 - inpRatio);
	      }

	      theBuffer	-> putDataIntoBuffer (temp, 2048);
//
//	shift the sample at the end to the beginning, it is needed
//	as the starting sample for the next time
	      convBuffer [0] = convBuffer [convBufferSize];
	      convIndex = 1;
	   }
	}
	return 0;
}
//
const char *airspyHandler::getSerial (void) {
airspy_read_partid_serialno_t read_partid_serialno;
int result = my_airspy_board_partid_serialno_read (device,
	                                          &read_partid_serialno);
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "failed: %s (%d)",
	         my_airspy_error_name ((airspy_error)result), result);
	   return "UNKNOWN";
	} else {
	   snprintf (serial, sizeof(serial), "%08X%08X", 
	             read_partid_serialno. serial_no [2],
	             read_partid_serialno. serial_no [3]);
	}
	return serial;
}
//
//	not used here
int	airspyHandler::open (void) {
int result = my_airspy_open (&device);

	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "airspy_open() failed: %s (%d)",
	          my_airspy_error_name((airspy_error)result), result);
	   return -1;
	} else {
	   return 0;
	}
}

void	airspyHandler::resetBuffer (void) {
	theBuffer	-> FlushRingBuffer ();
}

int16_t	airspyHandler::bitDepth (void) {
	return 12;
}

int32_t airspyHandler::getRate (void) {
        return inputRate;
}

int32_t	airspyHandler::getSamples (std::complex<float> *v, int32_t size, agcStats *stats) {

	return theBuffer	-> getDataFromBuffer (v, size);
}

int32_t	airspyHandler::Samples	(void) {
	return theBuffer	-> GetRingBufferReadAvailable ();
}
//
void	airspyHandler::setIfGain (int theGain) {
int	result = my_airspy_set_sensitivity_gain (device, theGain * MAX_GAIN / GAIN_SCALE);
	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "airspy_set_mixer_gain() failed: %s (%d)",
	            my_airspy_error_name ((airspy_error)result), result);
	   return;
	}
	ifGain = theGain;
}

//
/* Parameter value:
	0=Disable MIXER Automatic Gain Control
	1=Enable MIXER Automatic Gain Control
*/
void	airspyHandler::setAgcControl (int b) {
int result = my_airspy_set_mixer_agc (device, b);

	if (result != AIRSPY_SUCCESS) {
	   log (DEV_AIRSPY, LOG_MIN, "airspy_set_mixer_agc () failed: %s (%d)",
	            my_airspy_error_name ((airspy_error)result), result);
	   return;
	} else
		agcControl = (b != 0);
}


const char* airspyHandler::board_id_name (void) {
uint8_t bid;

	if (my_airspy_board_id_read (device, &bid) == AIRSPY_SUCCESS)
	   return my_airspy_board_id_name ((airspy_board_id)bid);
	else
	   return "UNKNOWN";
}
//
//
bool	airspyHandler::load_airspyFunctions (void) {
//
//	link the required procedures
	my_airspy_init	= (pfn_airspy_init)
	                       GETPROCADDRESS (Handle, "airspy_init");
	if (my_airspy_init == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_init");
	   return false;
	}

	my_airspy_exit	= (pfn_airspy_exit)
	                       GETPROCADDRESS (Handle, "airspy_exit");
	if (my_airspy_exit == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_exit");
	   return false;
	}

	my_airspy_open	= (pfn_airspy_open)
	                       GETPROCADDRESS (Handle, "airspy_open");
	if (my_airspy_open == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_open");
	   return false;
	}

	my_airspy_close	= (pfn_airspy_close)
	                       GETPROCADDRESS (Handle, "airspy_close");
	if (my_airspy_close == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_close");
	   return false;
	}

	my_airspy_get_samplerates	= (pfn_airspy_get_samplerates)
	                       GETPROCADDRESS (Handle, "airspy_get_samplerates");
	if (my_airspy_get_samplerates == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_get_samplerates");
	   return false;
	}

	my_airspy_set_samplerate	= (pfn_airspy_set_samplerate)
	                       GETPROCADDRESS (Handle, "airspy_set_samplerate");
	if (my_airspy_set_samplerate == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_samplerate");
	   return false;
	}

	my_airspy_start_rx	= (pfn_airspy_start_rx)
	                       GETPROCADDRESS (Handle, "airspy_start_rx");
	if (my_airspy_start_rx == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_start_rx");
	   return false;
	}

	my_airspy_stop_rx	= (pfn_airspy_stop_rx)
	                       GETPROCADDRESS (Handle, "airspy_stop_rx");
	if (my_airspy_stop_rx == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_stop_rx");
	   return false;
	}

	my_airspy_set_sample_type	= (pfn_airspy_set_sample_type)
	                       GETPROCADDRESS (Handle, "airspy_set_sample_type");
	if (my_airspy_set_sample_type == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_sample_type");
	   return false;
	}

	my_airspy_set_freq	= (pfn_airspy_set_freq)
	                       GETPROCADDRESS (Handle, "airspy_set_freq");
	if (my_airspy_set_freq == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_freq");
	   return false;
	}

	my_airspy_set_lna_gain	= (pfn_airspy_set_lna_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_lna_gain");
	if (my_airspy_set_lna_gain == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_lna_gain");
	   return false;
	}

	my_airspy_set_mixer_gain	= (pfn_airspy_set_mixer_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_mixer_gain");
	if (my_airspy_set_mixer_gain == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_mixer_gain");
	   return false;
	}

	my_airspy_set_vga_gain	= (pfn_airspy_set_vga_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_vga_gain");
	if (my_airspy_set_vga_gain == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_vga_gain");
	   return false;
	}
	
	my_airspy_set_linearity_gain = (pfn_airspy_set_linearity_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_linearity_gain");
	if (my_airspy_set_linearity_gain == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_linearity_gain");
	   return false;
	}

	my_airspy_set_sensitivity_gain = (pfn_airspy_set_sensitivity_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_sensitivity_gain");
	if (my_airspy_set_sensitivity_gain == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_sensitivity_gain");
	   return false;
	}

	my_airspy_set_lna_agc	= (pfn_airspy_set_lna_agc)
	                       GETPROCADDRESS (Handle, "airspy_set_lna_agc");
	if (my_airspy_set_lna_agc == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_lna_agc");
	   return false;
	}

	my_airspy_set_mixer_agc	= (pfn_airspy_set_mixer_agc)
	                       GETPROCADDRESS (Handle, "airspy_set_mixer_agc");
	if (my_airspy_set_mixer_agc == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_mixer_agc");
	   return false;
	}

	my_airspy_set_rf_bias	= (pfn_airspy_set_rf_bias)
	                       GETPROCADDRESS (Handle, "airspy_set_rf_bias");
	if (my_airspy_set_rf_bias == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_rf_bias");
	   return false;
	}

	my_airspy_error_name	= (pfn_airspy_error_name)
	                       GETPROCADDRESS (Handle, "airspy_error_name");
	if (my_airspy_error_name == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_error_name");
	   return false;
	}

	my_airspy_board_id_read	= (pfn_airspy_board_id_read)
	                       GETPROCADDRESS (Handle, "airspy_board_id_read");
	if (my_airspy_board_id_read == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_id_read");
	   return false;
	}

	my_airspy_board_id_name	= (pfn_airspy_board_id_name)
	                       GETPROCADDRESS (Handle, "airspy_board_id_name");
	if (my_airspy_board_id_name == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_id_name");
	   return false;
	}

	my_airspy_board_partid_serialno_read	=
	                (pfn_airspy_board_partid_serialno_read)
	                       GETPROCADDRESS (Handle, "airspy_board_partid_serialno_read");
	if (my_airspy_board_partid_serialno_read == NULL) {
	   log (DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_partid_serialno_read");
	   return false;
	}

	return true;
}

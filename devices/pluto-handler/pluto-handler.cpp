#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dabMini
 *
 *    dabMini is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    dabMini is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dabMini if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QThread>
#include	<QDebug>
#include	"pluto-handler.h"
#include	"ad9361.h"

#define MAX_GAIN	59

static	bool	debugFlag	= true;
// #define USE_NETWORK
#define HOST "127.0.0.1"

/* static scratch mem for strings */
static char tmpstr[64];

/* helper function generating channel names */
static
char*	get_ch_name (const char* type, int id) {
        snprintf (tmpstr, sizeof(tmpstr), "%s%d", type, id);
        return tmpstr;
}

static 
QString get_ch_name (QString type, int id) {
QString result = type;
	result. append (QString::number (id));
	return result;
}
//
//	Functions from the libad9361, needed here
int	ad9361_set_trx_fir_enable(struct iio_device *dev, int enable) {
int ret = iio_device_attr_write_bool (dev,
	                              "in_out_voltage_filter_fir_en",
	                              !!enable);
	if (ret < 0)
	   ret = iio_channel_attr_write_bool (
	                        iio_device_find_channel(dev, "out", false),
	                        "voltage_filter_fir_en", !!enable);
	return ret;
}

int	ad9361_get_trx_fir_enable (struct iio_device *dev, int *enable) {
bool value;

	int ret = iio_device_attr_read_bool (dev,
	                                     "in_out_voltage_filter_fir_en",
	                                     &value);

	if (ret < 0)
	   ret = iio_channel_attr_read_bool (
	                        iio_device_find_channel (dev, "out", false),
	                        "voltage_filter_fir_en", &value);
	if (!ret)
	   *enable	= value;

	return ret;
}
/* finds AD9361 streaming IIO channels */
bool	plutoHandler::
	      get_ad9361_stream_ch (struct iio_context *ctx,
	                            struct iio_device *dev,
	                            int chid, struct iio_channel **chn) {
	(void)ctx;
        *chn = iio_device_find_channel (dev, get_ch_name ("voltage", chid),
	                                                       false);
        if (*chn == nullptr)
	   *chn = iio_device_find_channel (dev,
	                                   get_ch_name ("altvoltage", chid),
	                                   false);
        return *chn != nullptr;
}

	plutoHandler::plutoHandler  ():
	                                  _I_Buffer (4 * 1024 * 1024) {
struct iio_channel *chn		= nullptr;

	phys_dev			= nullptr;
	this	-> gainControl		= 50;
	this	-> agcMode		= false;
	this	-> ctx			= nullptr;
	this	-> rxbuf		= nullptr;
	this	-> rx0_i		= nullptr;
	this	-> rx0_q		= nullptr;

	this	-> bw_hz		= PLUTO_RATE;
	this	-> fs_hz		= PLUTO_RATE;
	this	-> lo_hz		= 220000000;
	this	-> rfport		= "A_BALANCED";

	ctx	= iio_create_default_context ();
	if (ctx == nullptr) {
	   if (debugFlag)
	      fprintf (stderr, "default context failed\n");
	   ctx = iio_create_local_context ();
	}

#ifdef USE_NETWORK
	if (ctx == nullptr) {
	   if (debugFlag)
	      fprintf (stderr, "creating local context failed\n");
	   ctx = iio_create_network_context ("pluto.local");
	}

	if (ctx == nullptr) {
	   if (debugFlag)
	      fprintf (stderr, "creating network context with pluto.local failed\n");
	   ctx = iio_create_network_context (HOST);
	}
#endif

	if (ctx == nullptr) {
	   fprintf (stderr, "No pluto found, fatal\n");
	   throw (24);
	}

	if (debugFlag)
	   fprintf (stderr, "context name found %s\n",
	                            iio_context_get_name (ctx));

	if (iio_context_get_devices_count (ctx) <= 0) {
	   iio_context_destroy	(ctx);
	   throw (25);
	}

	rx = iio_context_find_device (ctx, "cf-ad9361-lpc");
	if (rx == nullptr) {
	   iio_context_destroy (ctx);
	   throw (26);
	}

// Configure phy and lo channels
	if (debugFlag)
	   fprintf (stderr, "* Acquiring AD9361 phy channel %d\n", 0);
	phys_dev = iio_context_find_device (ctx, "ad9361-phy");
	if (phys_dev == nullptr) {
	   if (debugFlag) 
	      fprintf (stderr, "no ad9361 found\n");
	   iio_context_destroy (ctx);
	   throw (27);
	}

	chn = iio_device_find_channel (phys_dev,
                                       get_ch_name (QString ("voltage"), 0).
	                                                  toLatin1 (). data (),
                                       false);
	if (chn == nullptr) {
	   if (debugFlag)
	      fprintf (stderr, "cannot acquire phy channel %d\n", 0);
	   iio_context_destroy (ctx);
	   throw (27);
	}

	int res = iio_channel_attr_write (chn, "rf_port_select",
	                                               this -> rfport);
	if (res < 0) {
	   if (debugFlag) {
	      char error [255];
	      iio_strerror (res, error, 255); 
	      fprintf (stderr, "error in port selection %s\n", error);
	   }
	   iio_context_destroy (ctx);
	   throw (28);
	}

	res = iio_channel_attr_write_longlong (chn,
	                                       "rf_bandwidth",
	                                       this -> bw_hz);
	if (res < 0) {
	   if (debugFlag) {
	      char errorText [255];
	      iio_strerror (res, errorText, 255); 
	      fprintf (stderr, "cannot select bandwidth %s\n", errorText);
	   }
	   iio_context_destroy (ctx);
	   throw (29);
	}

	res = iio_channel_attr_write_longlong (chn, "sampling_frequency",
	                                              this -> fs_hz);
	if (res < 0) {
	   if (debugFlag) {
	      char errorText [255];
	      iio_strerror (res, errorText, 255); 
	      fprintf (stderr, "cannot set sampling frequency %s\n", errorText);
	   }
	   iio_context_destroy (ctx);
	   throw (30);
	}

	this	-> gain_channel = chn;

// Configure LO channel
	if (debugFlag)
	   fprintf (stderr, "* Acquiring AD9361 %s lo channel\n", "RX");
	phys_dev = iio_context_find_device (ctx, "ad9361-phy");
//
	this -> lo_channel =
	             iio_device_find_channel (phys_dev,
                                              get_ch_name ("altvoltage", 0),
                                              true);
	if (this -> lo_channel == nullptr) {
	   if (debugFlag)
	      fprintf (stderr, "cannot find lo for channel\n");
	   iio_context_destroy (ctx);
	   throw (31);
	}

	res = iio_channel_attr_write_longlong (this -> lo_channel,
	                                               "frequency",
	                                               this -> lo_hz);
	if (res < 0 ) {
	   if (debugFlag) {
	      char error [255];
	      iio_strerror (res, error, 255); 
	      fprintf (stderr, "cannot set local oscillator frequency %s\n",
	                                                           error);
	   }
	   iio_context_destroy (ctx);
	   throw (32);
	}

        if (!get_ad9361_stream_ch (ctx, rx, 0, &rx0_i)) {
	   if (debugFlag)
	      fprintf (stderr, "Rx chan i not found\n");
	   iio_context_destroy (ctx);
	   throw (33);
	}

        if (!get_ad9361_stream_ch (ctx, rx, 1, &rx0_q)) {
	   if (debugFlag)
              fprintf (stderr, "Rx chan i not found\n");
           iio_context_destroy (ctx);
           throw (34);
	}

        iio_channel_enable (rx0_i);
        iio_channel_enable (rx0_q);

        rxbuf	= iio_device_create_buffer (rx, 1024*1024, false);
	if (rxbuf == nullptr) {
	   if (debugFlag) 
	      fprintf (stderr, "could not create RX buffer, fatal\n");
	   iio_context_destroy (ctx);
	   throw (35);
	}
//
	iio_buffer_set_blocking_mode (rxbuf, true);
	if (!agcMode) {
	   int ret = iio_channel_attr_write (this -> gain_channel,
	                                             "gain_control_mode",
	                                             "manual");
	   ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                                  "hardwaregain",
	                                                  gainControl * MAX_GAIN / 100);
	}
	else {
	   int ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                              "hardwaregain",
	                                              gainControl * MAX_GAIN / 100);
	   ret = iio_channel_attr_write (this -> gain_channel,
	                                             "gain_control_mode",
	                                             "slow_attack");
	}
	
//	set up for interpolator
	float	denominator	= float (DAB_RATE) / DIVIDER;
        float inVal		= float (PLUTO_RATE) / DIVIDER;
	for (int i = 0; i < DAB_RATE / DIVIDER; i ++) {
           mapTable_int [i]	= int (floor (i * (inVal / denominator)));
	   mapTable_float [i] =
	                     i * (inVal / denominator) - mapTable_int [i];
        }
        convIndex       = 0;

//      go for the filter
        int ret = ad9361_set_bb_rate_custom_filter_manual (phys_dev,
                                                           PLUTO_RATE,
                                                           1540000 / 2,
                                                           1.1 * 1540000 / 2,
                                                           1536000,
                                                           1536000);


	running. store (false);
	connected	= true;
}

	plutoHandler::~plutoHandler() {
	if (!connected)		// should not happen
	   return;
	stopReader();
	ad9361_set_trx_fir_enable (phys_dev, 0);
	iio_buffer_destroy (rxbuf);
	iio_context_destroy (ctx);
}
//

void	plutoHandler::setIfGain	(int newGain) {
int ret;

	gainControl = newGain;

	// changes to the gain only take effect when agc is turned off
	if (agcMode) {
	   return;
	}

	ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                       "hardwaregain",
	                                       newGain * MAX_GAIN / 100);
	if (ret < 0) {
	   if (debugFlag) 
	      fprintf (stderr,
	               "could not set hardware gain to %d\n", newGain);
	}
	emit configurationChanged();
}

void	plutoHandler::setAgcControl	(int m) {
int ret;

	agcMode = (m != 0);
	if (agcMode) {
	   ret = iio_channel_attr_write (this -> gain_channel,
	                                         "gain_control_mode",
	                                         "slow_attack");
	   if (ret < 0) {
	      if (debugFlag)
	         fprintf (stderr, "error in setting agc\n");
	      return;
	   }

	}
	else {	// switch agc off
	   ret = iio_channel_attr_write (this -> gain_channel,
	                                         "gain_control_mode",
	                                         "manual");
	   if (ret < 0) {
	      if (debugFlag)
	         fprintf (stderr, "error in gain setting\n");
	      return;
	   }

	   ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                          "hardwaregain", 
	                                          gainControl * MAX_GAIN / 100);
	   if (ret < 0) {
	      if (debugFlag)
	         fprintf (stderr,
	                  "could not set hardware gain to %d\n",
	                                          gainControl);
	   }
	}
	emit configurationChanged();
}

bool	plutoHandler::restartReader	(int32_t freq) {
int ret;
	if (debugFlag)
	   fprintf (stderr, "restart called with %d\n", freq);
	if (!connected)		// should not happen
	   return false;
	if (running. load())
	   return true;		// should not happen
	ret = iio_channel_attr_write (this -> gain_channel,
	                              "gain_control_mode",
	                              agcMode ?
	                                       "slow_attack" : "manual");
	if (ret < 0) {
	   if (debugFlag)
	      fprintf (stderr, "error in setting agc\n");
	}

	if (!agcMode) {
	   ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                          "hardwaregain",
	                                          gainControl * MAX_GAIN / 100);
	   if (ret < 0) {
	      if (debugFlag) 
	         fprintf (stderr,
	                  "could not set hardware gain to %d\n", 
	                                         gainControl);
	   }
	}

	this -> lo_hz = freq;
	ret = iio_channel_attr_write_longlong (this -> lo_channel,
	                                       "frequency",
	                                       this -> lo_hz);
	if (ret < 0) {
	   if (debugFlag)
	      fprintf (stderr, "cannot set local oscillator frequency\n");
	   return false;
	}
	else
	   start ();
	return true;
}

void	plutoHandler::stopReader() {
	if (!running. load())
	   return;
	running. store (false);
	while (isRunning())
	   usleep (500);
}

void	plutoHandler::run	() {
char	*p_end, *p_dat;
int	p_inc;
int	nbytes_rx;
std::complex<float> localBuf [DAB_RATE / DIVIDER];

	fprintf (stderr, "we are running\n");
	running. store (true);
	while (running. load ()) {
	   nbytes_rx	= iio_buffer_refill	(rxbuf);
	   p_inc	= iio_buffer_step	(rxbuf);
	   p_end	= (char *) iio_buffer_end  (rxbuf);

	   for (p_dat = (char *)iio_buffer_first (rxbuf, rx0_i);
	        p_dat < p_end; p_dat += p_inc) {
	      const int16_t i_p = ((int16_t *)p_dat) [0];
	      const int16_t q_p = ((int16_t *)p_dat) [1];
	      std::complex<float>sample = std::complex<float> (i_p / 2048.0,
	                                                       q_p / 2048.0);
	      convBuffer [convIndex ++] = sample;
	      if (convIndex > CONV_SIZE) {
	         for (int j = 0; j < DAB_RATE / DIVIDER; j ++) {
	            int16_t inpBase	= mapTable_int [j];
	            float   inpRatio	= mapTable_float [j];
	            localBuf [j]	= cmul (convBuffer [inpBase + 1],
	                                                          inpRatio) +
                                     cmul (convBuffer [inpBase], 1 - inpRatio);
                 }
	         _I_Buffer. putDataIntoBuffer (localBuf,
	                                        DAB_RATE / DIVIDER);
	         convBuffer [0] = convBuffer [CONV_SIZE];
	         convIndex = 1;
	      }
	   }
	}
	fprintf (stderr, "... and stopped\n");
}

int32_t	plutoHandler::getSamples (std::complex<float> *V, int32_t size) { 
	if (!isRunning ())
	   return 0;
	return _I_Buffer. getDataFromBuffer (V, size);
}

int32_t	plutoHandler::Samples () {
	return _I_Buffer. GetRingBufferReadAvailable();
}

void	plutoHandler::resetBuffer() {
	_I_Buffer. FlushRingBuffer();
}

int16_t	plutoHandler::bitDepth () {
	return 12;
}

int32_t plutoHandler::getRate   () {
        return PLUTO_RATE;
}

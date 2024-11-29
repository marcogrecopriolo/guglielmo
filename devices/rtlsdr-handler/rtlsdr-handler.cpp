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
 */

#include	<QThread>
#include	"rtlsdr-handler.h"
#include	"rtl-sdr.h"
#include	"logging.h"

#define STRBUFLEN 256
#define DEV_RTLSDR LOG_DEV

#define	READLEN_DEFAULT	8192

// for the callback, we do need some environment which
// is passed through the ctx parameter
//
// this is the user-side call back function
// ctx is the calling task
static
void RTLSDRCallBack(uint8_t *buf, uint32_t len, void *ctx) {
    rtlsdrHandler *theStick = (rtlsdrHandler *) ctx;

    if ((theStick == NULL) || (len != READLEN_DEFAULT))
	return;

    (void) theStick->_I_Buffer.putDataIntoBuffer (buf, len);
}

// for handling the events in libusb, we need a control thread
// whose sole purpose is to process the rtlsdr_read_async function
// from the lib
class dllDriver : public QThread {
private:
    rtlsdrHandler *theStick;

public:

    dllDriver(rtlsdrHandler *d) {
	theStick = d;
	start();
    }

    ~dllDriver(void) {
    }

private:
    virtual void run(void) {
	(theStick->rtlsdr_read_async)(theStick->device,
				      (rtlsdr_read_async_cb_t) &RTLSDRCallBack,
				      (void *) theStick, 0, READLEN_DEFAULT);
    }
};

rtlsdrHandler::rtlsdrHandler(): _I_Buffer (4 * 1024 * 1024) {
    int	i;

    currentId[0] = '\0';
    agcControl = false;
    ifGain = 50;
    libraryLoaded = false;
    open = false;
    workerHandle = NULL;
    gains = NULL;

#if IS_WINDOWS
    const char *libraryString = "rtlsdr.dll";
    Handle = LoadLibraryA(libraryString);
#else
    const char *libraryString = "librtlsdr" LIBEXT;
    Handle = dlopen(libraryString, RTLD_NOW);
#endif

    if (Handle == NULL) {
#if IS_WINDOWS
	log(DEV_RTLSDR, LOG_MIN, "failed to open %s Error = %li", libraryString, GetLastError());
#else
	log(DEV_RTLSDR, LOG_MIN, "failed to open %s Error = %s", libraryString, dlerror());
#endif
	throw (20);
    }

    libraryLoaded = true;
    if (!load_rtlFunctions()) {
	CLOSE_LIBRARY(Handle);
	throw(21);
    }
    if (this->rtlsdr_get_device_count() == 0) {
	CLOSE_LIBRARY(Handle);
	throw(22);
    }

    // rather than doing floating point divisions at all times
    // we just have a table of possible float values
    for (i = 0; i <= UCHAR_MAX; i++)
	convTable[i] = (CHAR_MIN+i)/float(-CHAR_MIN);

    if (!deviceOpen(0)) {
	log(DEV_RTLSDR, LOG_MIN, "opening device failed");
	CLOSE_LIBRARY(Handle);
	throw(23);
    }
}

rtlsdrHandler::~rtlsdrHandler(void) {
    if (Handle == NULL)
	return;
	
    if (open) {	
	stopReader();
	this->rtlsdr_close(device);
    }
    CLOSE_LIBRARY(Handle);
    if (gains != NULL)
	delete gains;
}


bool rtlsdrHandler::deviceOpen(int devNo) {
    char *devName;
    int err, i;

    log(DEV_RTLSDR, LOG_CHATTY, "opening device %i", devNo);
    err = this->rtlsdr_open(&device, devNo);
    if (err < 0) {
        log(DEV_RTLSDR, LOG_MIN, "device %i open failed: %i", devNo, err);
	return false;
    }
    err = this->rtlsdr_set_sample_rate(device, INPUT_RATE);
    if (err < 0) {
	log(DEV_RTLSDR, LOG_MIN, "setting samplerate failed: %i", err);
	rtlsdr_close(device);
	return false;
    }
    open = true;
    log(DEV_RTLSDR, LOG_CHATTY, "samplerate set to %d", INPUT_RATE);

    gainsCount = rtlsdr_get_tuner_gains(device, NULL);
    gains = new int[gainsCount];
    gainsCount = rtlsdr_get_tuner_gains(device, gains);
    for (i = 0; i < gainsCount; i++)
        log(DEV_RTLSDR, LOG_CHATTY, "found gain %i", gains[i]);
    devName = this->rtlsdr_get_device_name(i);
    sprintf((char *) currentId, "%s-%i", devName, 0);

    rtlsdr_set_agc_mode(device, agcControl != 0);
    rtlsdr_set_tuner_gain_mode(device, 1);
    rtlsdr_set_tuner_gain(device, gains[ifGain*gainsCount/GAIN_SCALE]);
    log(DEV_RTLSDR, LOG_CHATTY, "opened %s (%i)", currentId, devNo);
    return true;
}

int rtlsdrHandler::devices(deviceStrings *devs, int max) {
    char manu[STRBUFLEN], prod[STRBUFLEN], serNo[STRBUFLEN];

    int32_t count = this->rtlsdr_get_device_count();
    if (count <= 0)
	return 0;
    if (count > max)
	count = max;
    for (int i = 0; i < count; i++) {
	char * name;

	*devs[i].id = '\0';
	*devs[i].description = '\0';
	name = this->rtlsdr_get_device_name(i);
	strncpy((char *) &devs[i].name, name, DEV_SHORT);
	log(DEV_RTLSDR, LOG_CHATTY, "found device %i %s", i, (char *) &devs[i].name);

	// unluckily RTLSDR devices seem not to care about serial numbers, I have
	// two both wth S/N 00000001!
	sprintf((char *) &devs[i].id, "%s-%i", name, i);
	if (rtlsdr_get_device_usb_strings(i, manu, prod, serNo) == 0) {
	    int len = DEV_LONG;

	    strncat((char *) &devs[i].description, manu, len);
	    len -= strlen((char *) &devs[i].description);
	    strncat((char *) &devs[i].description, " ", len);
	    len -= strlen((char *) &devs[i].description);
	    strncat((char *) &devs[i].description, prod, len);
	    log(DEV_RTLSDR, LOG_CHATTY, "strings %s %s %s", (char *) manu, (char *) prod, (char *) serNo);
	}
    }
    log(DEV_RTLSDR, LOG_MIN, "found %i devices", count);
    return count;
}

bool rtlsdrHandler::setDevice(const char *id) {
    int devNo, count;
    char buf[DEV_SHORT];

    if (strcmp(id, currentId) == 0) {
	log(DEV_RTLSDR, LOG_MIN, "Skipping device switching - same device: %s", id);
	return true;
    }
    count = this->rtlsdr_get_device_count();
    for (devNo = 0; devNo < count; devNo++) {
	char *name = this->rtlsdr_get_device_name(devNo);
	log(DEV_RTLSDR, LOG_CHATTY, "found device %i %s", devNo, name);
	sprintf((char *) &buf, "%s-%i", name, devNo); 
	if (strcmp(id, (char *) buf) == 0)
	   break;
    }
    if (devNo >= count) {
	log(DEV_RTLSDR, LOG_MIN, "device not found %s", id);
	return false;
    }
    if (open) {
	log(DEV_RTLSDR, LOG_MIN, "stopping old device %s", currentId);
	stopReader();
	this->rtlsdr_close(device);
	if (gains != NULL)
	    delete gains;
	open = false;
	currentId[0] = '\0';
    }
    return deviceOpen(devNo);
}

bool rtlsdrHandler::restartReader(int32_t frequency) {
    if (workerHandle != NULL)
	return true;

    _I_Buffer. FlushRingBuffer();
    if (this->rtlsdr_reset_buffer(device) < 0)
	return false;

    this->rtlsdr_set_center_freq(device, frequency);
    workerHandle = new dllDriver(this);
    rtlsdr_set_agc_mode(device, agcControl);
    rtlsdr_set_tuner_gain(device, gains[ifGain*gainsCount/GAIN_SCALE]);
    return true;
}

void rtlsdrHandler::stopReader(void) {
    if (workerHandle == NULL)
	return;
    if (workerHandle != NULL) {
	this -> rtlsdr_cancel_async(device);
	if (workerHandle != NULL) {
	    while (!workerHandle->isFinished())
		usleep (100);
	    delete workerHandle;
	}
    }
    workerHandle = NULL;
}

void rtlsdrHandler::setIfGain(int gain) {
    log(DEV_RTLSDR, LOG_MIN, "IF gain will be set to %d%% to %d",
	gain, gains[gain*gainsCount/GAIN_SCALE]);
    ifGain = gain;
    rtlsdr_set_tuner_gain(device, gains[gain*gainsCount/GAIN_SCALE]);
}

void rtlsdrHandler::setAgcControl(int v) {
    agcControl = (v != 0);
    log(DEV_RTLSDR, LOG_MIN, "agc will be set to %d", v);
    rtlsdr_set_agc_mode(device, v);
    rtlsdr_set_tuner_gain(device, gains[ifGain*gainsCount/GAIN_SCALE]);
}

int32_t	rtlsdrHandler::getSamples(std::complex<float> *V, int32_t size, agcStats *stats) {
    int32_t amount, in, out;
    int32_t overflow = 0, minVal = UCHAR_MAX, maxVal = 0;
    _VLA(uint8_t, tempBuffer, 2*size*sizeof(uint8_t));

    amount = _I_Buffer.getDataFromBuffer(tempBuffer, 2*size);
    for (in = 0, out = 0; in < amount; out++) {
	int r = tempBuffer[in++], im = tempBuffer[in++];

	if (r == 0 || r == UCHAR_MAX || im == 0 || im == UCHAR_MAX)
	    overflow++;
	if (r < minVal)
	    minVal = r;
	if (r > maxVal)
	    maxVal = r;
	if (im < minVal)
	    minVal = im;
	if (im > maxVal)
	    maxVal = im;
	V[out] = std::complex<float>(convTable[r], convTable[im]);
    }
    stats->overflows = overflow;
    stats->min = minVal;
    stats->max = maxVal;
    return amount/2;
}

int32_t	rtlsdrHandler::Samples(void) {
    return _I_Buffer. GetRingBufferReadAvailable () / 2;
}

bool rtlsdrHandler::load_rtlFunctions(void) {
    rtlsdr_open = (pfnrtlsdr_open)
	GETPROCADDRESS(Handle, "rtlsdr_open");
    if (rtlsdr_open == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_open");
	return false;
    }
    rtlsdr_close = (pfnrtlsdr_close)
	GETPROCADDRESS(Handle, "rtlsdr_close");
    if (rtlsdr_close == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_close");
	return false;
    }
    rtlsdr_set_sample_rate = (pfnrtlsdr_set_sample_rate)
	GETPROCADDRESS(Handle, "rtlsdr_set_sample_rate");
    if (rtlsdr_set_sample_rate == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_sample_rate");
	return false;
    }
    rtlsdr_get_sample_rate = (pfnrtlsdr_get_sample_rate)
	GETPROCADDRESS(Handle, "rtlsdr_get_sample_rate");
    if (rtlsdr_get_sample_rate == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_sample_rate");
	return false;
    }
    rtlsdr_get_tuner_gains = (pfnrtlsdr_get_tuner_gains)
	GETPROCADDRESS(Handle, "rtlsdr_get_tuner_gains");
    if (rtlsdr_get_tuner_gains == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_tuner_gains");
	return false;
    }
    rtlsdr_set_tuner_gain_mode = (pfnrtlsdr_set_tuner_gain_mode)
	GETPROCADDRESS(Handle, "rtlsdr_set_tuner_gain_mode");
    if (rtlsdr_set_tuner_gain_mode == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_tuner_gain_mode");
	return false;
    }
    rtlsdr_set_agc_mode = (pfnrtlsdr_set_agc_mode)
	GETPROCADDRESS(Handle, "rtlsdr_set_agc_mode");
    if (rtlsdr_set_agc_mode == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_agc_mode");
	return false;
    }
    rtlsdr_set_tuner_gain = (pfnrtlsdr_set_tuner_gain)
	GETPROCADDRESS(Handle, "rtlsdr_set_tuner_gain");
    if (rtlsdr_set_tuner_gain == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Cound not find rtlsdr_set_tuner_gain");
	return false;
    }
    rtlsdr_get_tuner_gain = (pfnrtlsdr_get_tuner_gain)
	GETPROCADDRESS(Handle, "rtlsdr_get_tuner_gain");
    if (rtlsdr_get_tuner_gain == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_tuner_gain");
	return false;
    }
    rtlsdr_set_center_freq = (pfnrtlsdr_set_center_freq)
	GETPROCADDRESS(Handle, "rtlsdr_set_center_freq");
    if (rtlsdr_set_center_freq == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_center_freq");
	return false;
    }
    rtlsdr_get_center_freq = (pfnrtlsdr_get_center_freq)
	GETPROCADDRESS(Handle, "rtlsdr_get_center_freq");
    if (rtlsdr_get_center_freq == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_center_freq");
	return false;
    }
    rtlsdr_reset_buffer = (pfnrtlsdr_reset_buffer)
	GETPROCADDRESS(Handle, "rtlsdr_reset_buffer");
    if (rtlsdr_reset_buffer == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_reset_buffer");
	return false;
    }
    rtlsdr_read_async = (pfnrtlsdr_read_async)
	GETPROCADDRESS(Handle, "rtlsdr_read_async");
    if (rtlsdr_read_async == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Cound not find rtlsdr_read_async");
	return false;
    }
    rtlsdr_get_device_count = (pfnrtlsdr_get_device_count)
	GETPROCADDRESS(Handle, "rtlsdr_get_device_count");
    if (rtlsdr_get_device_count == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_device_count");
	return false;
    }
    rtlsdr_cancel_async = (pfnrtlsdr_cancel_async)
	GETPROCADDRESS(Handle, "rtlsdr_cancel_async");
    if (rtlsdr_cancel_async == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_cancel_async");
	return false;
    }
    rtlsdr_set_direct_sampling = (pfnrtlsdr_set_direct_sampling)
	GETPROCADDRESS(Handle, "rtlsdr_set_direct_sampling");
    if (rtlsdr_set_direct_sampling == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_direct_sampling");
	return false;
    }
    rtlsdr_set_freq_correction = (pfnrtlsdr_set_freq_correction)
	GETPROCADDRESS(Handle, "rtlsdr_set_freq_correction");
    if (rtlsdr_set_freq_correction == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_set_freq_correction");
	return false;
    }
    rtlsdr_get_device_name = (pfnrtlsdr_get_device_name)
	GETPROCADDRESS(Handle, "rtlsdr_get_device_name");
    if (rtlsdr_get_device_name == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_device_name");
	return false;
    }
    rtlsdr_get_device_usb_strings = (pfnrtlsdr_get_device_usb_strings)
	GETPROCADDRESS(Handle, "rtlsdr_get_device_usb_strings");
    if (rtlsdr_get_device_usb_strings == NULL) {
	log(DEV_RTLSDR, LOG_MIN, "Could not find rtlsdr_get_device_usb_strings");
	return false;
    }
    log(DEV_RTLSDR, LOG_MIN, "functions seem to be loaded");
    return true;
}

void rtlsdrHandler::resetBuffer(void) {
    _I_Buffer.FlushRingBuffer();
}

int16_t	rtlsdrHandler::bitDepth(void) {
    return CHAR_BIT;
}

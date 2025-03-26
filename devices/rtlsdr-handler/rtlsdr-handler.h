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

#ifndef RTLSDR_HANDLER_H
#define	RTLSDR_HANDLER_H

#include <climits>
#include <stdio.h>
#include "constants.h"
#include "device-handler.h"
#include "ringbuffer.h"

class dllDriver;

typedef	struct rtlsdr_dev rtlsdr_dev_t;
extern "C" {
    typedef void (*rtlsdr_read_async_cb_t)(uint8_t *buf, uint32_t len, void *ctx);
    typedef int (*pfnrtlsdr_open )(rtlsdr_dev_t **, uint32_t);
    typedef int (*pfnrtlsdr_close)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_set_center_freq)(rtlsdr_dev_t *, uint32_t);
    typedef uint32_t (*pfnrtlsdr_get_center_freq)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_get_tuner_gains)(rtlsdr_dev_t *, int *);
    typedef int (*pfnrtlsdr_set_tuner_gain_mode)(rtlsdr_dev_t *, int);
    typedef int (*pfnrtlsdr_set_agc_mode)(rtlsdr_dev_t *, int);
    typedef int (*pfnrtlsdr_set_sample_rate)(rtlsdr_dev_t *, uint32_t);
    typedef int (*pfnrtlsdr_get_sample_rate)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_set_tuner_gain)(rtlsdr_dev_t *, int);
    typedef int (*pfnrtlsdr_get_tuner_gain)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_reset_buffer)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_read_async)(rtlsdr_dev_t *, rtlsdr_read_async_cb_t,
				       void *, uint32_t, uint32_t);
    typedef int (*pfnrtlsdr_cancel_async)(rtlsdr_dev_t *);
    typedef int (*pfnrtlsdr_set_direct_sampling)(rtlsdr_dev_t *, int);
    typedef uint32_t (*pfnrtlsdr_get_device_count)(void);
    typedef int (*pfnrtlsdr_set_freq_correction)(rtlsdr_dev_t *, int);
    typedef char *(*pfnrtlsdr_get_device_name)(int);
    typedef char *(*pfnrtlsdr_get_device_usb_strings)(int, char *, char *, char *);
}

class rtlsdrHandler: public deviceHandler {
Q_OBJECT
public:
    rtlsdrHandler(void);
    ~rtlsdrHandler(void);

    int32_t devices(deviceStrings *, int);
    bool setDevice(const char *);
    bool restartReader(int32_t frequency);
    void stopReader(void);
    int32_t getSamples(std::complex<float> *,
		       int32_t, agcStats *stats);
    int32_t Samples(void);
    void resetBuffer(void);
    int16_t bitDepth(void);
    void setIfGain(int);
    void setAgcControl(int);
 
    // These need to be visible for the separate usb handling thread
    RingBuffer<uint8_t> _I_Buffer;
    pfnrtlsdr_read_async rtlsdr_read_async;
    struct rtlsdr_dev *device;

private:
    bool deviceOpen(int);
    bool load_rtlFunctions(void);

    char currentId[DEV_SHORT];
    bool agcControl;
    int	 ifGain;
    HINSTANCE Handle;
    dllDriver *workerHandle;
    bool libraryLoaded;
    bool open;
    int *gains;
    int16_t gainsCount;
    float convTable[UCHAR_MAX+1];

    pfnrtlsdr_open rtlsdr_open;
    pfnrtlsdr_close rtlsdr_close;
    pfnrtlsdr_set_center_freq rtlsdr_set_center_freq;
    pfnrtlsdr_get_center_freq rtlsdr_get_center_freq;
    pfnrtlsdr_get_tuner_gains rtlsdr_get_tuner_gains;
    pfnrtlsdr_set_tuner_gain_mode rtlsdr_set_tuner_gain_mode;
    pfnrtlsdr_set_agc_mode rtlsdr_set_agc_mode;
    pfnrtlsdr_set_sample_rate rtlsdr_set_sample_rate;
    pfnrtlsdr_get_sample_rate rtlsdr_get_sample_rate;
    pfnrtlsdr_set_tuner_gain rtlsdr_set_tuner_gain;
    pfnrtlsdr_get_tuner_gain rtlsdr_get_tuner_gain;
    pfnrtlsdr_reset_buffer rtlsdr_reset_buffer;
    pfnrtlsdr_cancel_async rtlsdr_cancel_async;
    pfnrtlsdr_set_direct_sampling rtlsdr_set_direct_sampling;
    pfnrtlsdr_get_device_count rtlsdr_get_device_count;
    pfnrtlsdr_set_freq_correction rtlsdr_set_freq_correction;
    pfnrtlsdr_get_device_name rtlsdr_get_device_name;
    pfnrtlsdr_get_device_usb_strings rtlsdr_get_device_usb_strings;
};
#endif

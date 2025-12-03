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

#include "constants.h"
#include "airspy-handler.h"

#include <climits>

#include "logging.h"
#include "math-helper.h"
#include <inttypes.h>

#define DEV_AIRSPY LOG_DEV

static const int MAX_GAIN = 21;

// we sample 2 signed 16 bits ints, but after averaging,
// we consider overflow anything above 15 bits
// also, empirically, neither the mini nor the R2 get anywhere
// near 16 bits
static const int SIGNAL_MAX = 16384;
static const float AMPLITUDE = SIGNAL_MAX;

airspyHandler::airspyHandler() {
    uint64_t numId;
    int result;

    agcControl = false;
    ifGain = MAX_GAIN / 2;
    device = NULL;
    theBuffer = NULL;
#if IS_WINDOWS
    const char* libraryString = "airspy.dll";
    Handle = LoadLibraryA(libraryString);
#else
    const char* libraryString = "libairspy" LIBEXT;
    Handle = dlopen(libraryString, RTLD_LAZY);
#endif

    if (Handle == NULL) {
#if IS_WINDOWS
        log(DEV_AIRSPY, LOG_MIN, "failed to open %s - Error = %li", libraryString, GetLastError());
#else
        log(DEV_AIRSPY, LOG_MIN, "failed to open %s - Error = %s", libraryString, dlerror());
#endif
        throw(20);
    }

    libraryLoaded = true;

    if (!load_airspyFunctions()) {
        log(DEV_AIRSPY, LOG_MIN, "problem in loading functions");
        CLOSE_LIBRARY(Handle);
        throw(21);
    }

    result = this->my_airspy_init();
    if (result != AIRSPY_SUCCESS) {
        log(DEV_AIRSPY, LOG_MIN, "my_airspy_init () failed: %s (%d)",
            my_airspy_error_name((airspy_error)result), result);
        CLOSE_LIBRARY(Handle);
        throw(21);
    }

    (void) my_airspy_list_devices(&numId, 1);
    if (!deviceOpen(numId)) {
        CLOSE_LIBRARY(Handle);
        throw(22);
    }

    theBuffer = new RingBuffer<std::complex<int32_t>>(4 * 1024 * 1024);
    running.store(false);
    log(DEV_AIRSPY, LOG_MIN, "airspy loaded");
}

airspyHandler::~airspyHandler(void) {
    if (device != NULL) {
	int result;

        stopReader();
        result = my_airspy_close(device);
        if (result != AIRSPY_SUCCESS) {
            log(DEV_AIRSPY, LOG_MIN, "airspy_close () failed: %s (%d)",
                my_airspy_error_name((airspy_error)result), result);
        }
        my_airspy_exit();
    }

    if (Handle != NULL)
        CLOSE_LIBRARY(Handle);

    if (theBuffer != NULL)
        delete theBuffer;
}

int airspyHandler::devices(deviceStrings *devs, int max) {
    uint64_t deviceList[max];

    int count = my_airspy_list_devices(deviceList, max);
    if (count <= 0) {
	log(DEV_AIRSPY, LOG_MIN, "airspy_list_devices returned %i", count);
        return 0;
    }
    for (int i = 0; i < count; i++) {
        *devs[i].description = '\0';
        sprintf((char *) &devs[i].name, "%" PRIx64, deviceList[i]);
        sprintf((char *) &devs[i].id, "%" PRIx64, deviceList[i]);

	// unlike librtlsdr, libairspy does not provide a device name or
	// usb strings
	// airspy usb strings do not distinguish between different products,
	// so we are just stuck with the serial number
        log(DEV_AIRSPY, LOG_CHATTY, "found device %i %s", i, (char *) &devs[i].id);

    }
    log(DEV_AIRSPY, LOG_MIN, "found %i devices", count);
    return count;
}

bool airspyHandler::setDevice(const char *id) {
    uint64_t numId;

    numId = std::strtoull(id, NULL, 16);
    if (currentId == numId) {
        log(DEV_AIRSPY, LOG_MIN, "Skipping device switching - same device: %s", id);
        return true;
    }
    if (open) {
        log(DEV_AIRSPY, LOG_MIN, "stopping old device %" PRIx64, currentId);
        stopReader();
        my_airspy_close(device);
        open = false;
        currentId = 0;
    }
    return deviceOpen(numId);
}

bool airspyHandler::deviceOpen(uint64_t numId) {
    int err, result;

    err = my_airspy_open_sn(&device, numId);
    if (err < 0) {
        log(DEV_AIRSPY, LOG_MIN, "%" PRIx64 "open failed: %i", numId, err);
        return false;
    }
    (void) my_airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_IQ);

    // Both the Airspy R2 and the mini support rates higher than 2048kbps.
    // Although, traditionally, people list sample rates and then choose
    // one of the advertised ones, both devices can set any sample rate.
    // Empirically, using one of the advertised rates has HW AGC working
    // much better, but at the extra cost of converting rates.
    // We choose simplicity, use 4096kbps and decimate by 2.
    result = my_airspy_set_samplerate(device, INPUT_RATE * 2);
    if (result != AIRSPY_SUCCESS) {
        log(DEV_AIRSPY, LOG_MIN, "airspy_set_samplerate() failed: %s (%d)",
            my_airspy_error_name((enum airspy_error) result), result);
        my_airspy_close(device);
	return false;
    }
    currentId = numId;
    open = true;
    log(DEV_AIRSPY, LOG_MIN, "opened device %" PRIx64, numId);
    return true;
}

bool airspyHandler::restartReader(int32_t frequency) {
    int result;

    if (running.load())
        return true;

    theBuffer->FlushRingBuffer();

    my_airspy_set_freq(device, frequency);
    my_airspy_set_sensitivity_gain(device, ifGain);
    result = my_airspy_set_mixer_agc(device,
        agcControl ? 1 : 0);

    result = my_airspy_start_rx(device,
        (airspy_sample_block_cb_fn)callback, this);
    if (result != AIRSPY_SUCCESS) {
        log(DEV_AIRSPY, LOG_MIN, "my_airspy_start_rx() failed: %s (%d)",
            my_airspy_error_name((airspy_error)result), result);
        return false;
    }

    running.store(true);
    log(DEV_AIRSPY, LOG_MIN, "reader stopped");
    return true;
}

void airspyHandler::stopReader(void) {
    int result;

    if (!running.load())
        return;
    running.store(false);
    result = my_airspy_stop_rx(device);

    if (result != AIRSPY_SUCCESS)
        log(DEV_AIRSPY, LOG_MIN, "my_airspy_stop_rx() failed: %s (%d)",
            my_airspy_error_name((airspy_error)result), result);

    theBuffer->FlushRingBuffer();
    log(DEV_AIRSPY, LOG_MIN, "reader stopped");
}

int airspyHandler::callback(airspy_transfer* transfer) {
    airspyHandler* p;

    if (!transfer)
        return 0;	// should not happen
    p = static_cast<airspyHandler*>(transfer->ctx);
    if (!p->running.load())
        return 0;

    p->data_available((int16_t *) transfer->samples, transfer->sample_count);
    return 0;
}

int airspyHandler::data_available(int16_t *sbuf, int nSamples) {
    int sampleCount = nSamples / 2;
    int32_t i;
    _VLA(std::complex<int32_t>, temp, sampleCount);

    for (i = 0; i < sampleCount; i++) {
        temp[i] = std::complex<int32_t>(
            sbuf[4 * i] + sbuf[4 * i + 2],
            sbuf[4 * i + 1] + sbuf[4 * i + 3]);
    }
    if (!running.load())
	return 0;
    theBuffer->putDataIntoBuffer(temp, sampleCount);
    return 0;
}

void airspyHandler::resetBuffer(void) {
    theBuffer->FlushRingBuffer();
}

int16_t airspyHandler::bitDepth(void) {
    return 12;
}

#define MAX_SIGNAL 16384

// the airspy requires a rather large acceptable signal window
void airspyHandler::getSwAGCRange(int32_t *min, int32_t *max) {
    *min = MAX_SIGNAL / 2;
    *max = MAX_SIGNAL;
}

void airspyHandler::getIfRange(int *min, int *max) {
    *min = 0;
    *max = MAX_GAIN;
}

int32_t airspyHandler::getSamples(std::complex<float>* v, int32_t size, agcStats* stats) {
    _VLA(std::complex<int32_t>,  temp, size);
    int32_t overflow = 0, minVal = INT_MAX, maxVal = INT_MIN;
    int i;

    int amount = theBuffer->getDataFromBuffer(temp, size);
    for (i = 0; i < amount; i++) {
        int r = real(temp[i]), im = imag(temp[i]);

        if (r <= -SIGNAL_MAX || r >= SIGNAL_MAX-1 || im <= -SIGNAL_MAX || im >= SIGNAL_MAX-1)
            overflow++;
        if (r < minVal)
            minVal = r;
        if (r > maxVal)
            maxVal = r;
        if (im < minVal)
            minVal = im;
        if (im > maxVal)
            maxVal = im;
        v[i] = std::complex<float>(r / AMPLITUDE, im / AMPLITUDE);
    }
    stats->overflows = overflow;
    stats->min = minVal;
    stats->max = maxVal;
    return amount;
}

int32_t airspyHandler::Samples(void) {
    return theBuffer->GetRingBufferReadAvailable();
}

void airspyHandler::setIfGain(int theGain) {
    int result = my_airspy_set_sensitivity_gain(device, theGain);
    if (result != AIRSPY_SUCCESS) {
        log(DEV_AIRSPY, LOG_MIN, "airspy_set_mixer_gain() failed: %s (%d)",
            my_airspy_error_name((airspy_error)result), result);
        return;
    }
    ifGain = theGain;
    log(DEV_AIRSPY, LOG_MIN, "IF gain will be set to %d", ifGain);
}

void airspyHandler::setAgcControl(int b) {
    int result = my_airspy_set_mixer_agc(device, b);

    if (result != AIRSPY_SUCCESS) {
        log(DEV_AIRSPY, LOG_MIN, "airspy_set_mixer_agc () failed: %s (%d)",
            my_airspy_error_name((airspy_error)result), result);
        return;
    } else {
        agcControl = (b != 0);
	log(DEV_AIRSPY, LOG_MIN, "agc will be set to %d", agcControl);
    }
}

bool airspyHandler::load_airspyFunctions(void) {
    my_airspy_init = (pfn_airspy_init)
        GETPROCADDRESS(Handle, "airspy_init");
    if (my_airspy_init == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_init");
        return false;
    }

    my_airspy_exit = (pfn_airspy_exit)
        GETPROCADDRESS(Handle, "airspy_exit");
    if (my_airspy_exit == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_exit");
        return false;
    }

    my_airspy_open = (pfn_airspy_open)
        GETPROCADDRESS(Handle, "airspy_open");
    if (my_airspy_open == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_open");
        return false;
    }

    my_airspy_open_sn = (pfn_airspy_open_sn)
        GETPROCADDRESS(Handle, "airspy_open_sn");
    if (my_airspy_open_sn == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_open_sn");
        return false;
    }

    my_airspy_close = (pfn_airspy_close)
        GETPROCADDRESS(Handle, "airspy_close");
    if (my_airspy_close == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_close");
        return false;
    }

    my_airspy_get_samplerates = (pfn_airspy_get_samplerates)
        GETPROCADDRESS(Handle, "airspy_get_samplerates");
    if (my_airspy_get_samplerates == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_get_samplerates");
        return false;
    }

    my_airspy_set_samplerate = (pfn_airspy_set_samplerate)
        GETPROCADDRESS(Handle, "airspy_set_samplerate");
    if (my_airspy_set_samplerate == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_samplerate");
        return false;
    }

    my_airspy_start_rx = (pfn_airspy_start_rx)
        GETPROCADDRESS(Handle, "airspy_start_rx");
    if (my_airspy_start_rx == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_start_rx");
        return false;
    }

    my_airspy_stop_rx = (pfn_airspy_stop_rx)
        GETPROCADDRESS(Handle, "airspy_stop_rx");
    if (my_airspy_stop_rx == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_stop_rx");
        return false;
    }

    my_airspy_set_sample_type = (pfn_airspy_set_sample_type)
        GETPROCADDRESS(Handle, "airspy_set_sample_type");
    if (my_airspy_set_sample_type == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_sample_type");
        return false;
    }

    my_airspy_set_freq = (pfn_airspy_set_freq)
        GETPROCADDRESS(Handle, "airspy_set_freq");
    if (my_airspy_set_freq == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_freq");
        return false;
    }

    my_airspy_set_lna_gain = (pfn_airspy_set_lna_gain)
        GETPROCADDRESS(Handle, "airspy_set_lna_gain");
    if (my_airspy_set_lna_gain == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_lna_gain");
        return false;
    }

    my_airspy_set_mixer_gain = (pfn_airspy_set_mixer_gain)
        GETPROCADDRESS(Handle, "airspy_set_mixer_gain");
    if (my_airspy_set_mixer_gain == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_mixer_gain");
        return false;
    }

    my_airspy_set_vga_gain = (pfn_airspy_set_vga_gain)
        GETPROCADDRESS(Handle, "airspy_set_vga_gain");
    if (my_airspy_set_vga_gain == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_vga_gain");
        return false;
    }

    my_airspy_set_linearity_gain = (pfn_airspy_set_linearity_gain)
        GETPROCADDRESS(Handle, "airspy_set_linearity_gain");
    if (my_airspy_set_linearity_gain == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_linearity_gain");
        return false;
    }

    my_airspy_set_sensitivity_gain = (pfn_airspy_set_sensitivity_gain)
        GETPROCADDRESS(Handle, "airspy_set_sensitivity_gain");
    if (my_airspy_set_sensitivity_gain == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_sensitivity_gain");
        return false;
    }

    my_airspy_set_lna_agc = (pfn_airspy_set_lna_agc)
        GETPROCADDRESS(Handle, "airspy_set_lna_agc");
    if (my_airspy_set_lna_agc == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_lna_agc");
        return false;
    }

    my_airspy_set_mixer_agc = (pfn_airspy_set_mixer_agc)
        GETPROCADDRESS(Handle, "airspy_set_mixer_agc");
    if (my_airspy_set_mixer_agc == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_mixer_agc");
        return false;
    }

    my_airspy_set_rf_bias = (pfn_airspy_set_rf_bias)
        GETPROCADDRESS(Handle, "airspy_set_rf_bias");
    if (my_airspy_set_rf_bias == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_set_rf_bias");
        return false;
    }

    my_airspy_error_name = (pfn_airspy_error_name)
        GETPROCADDRESS(Handle, "airspy_error_name");
    if (my_airspy_error_name == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_error_name");
        return false;
    }

    my_airspy_board_id_read = (pfn_airspy_board_id_read)
        GETPROCADDRESS(Handle, "airspy_board_id_read");
    if (my_airspy_board_id_read == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_id_read");
        return false;
    }

    my_airspy_board_id_name = (pfn_airspy_board_id_name)
        GETPROCADDRESS(Handle, "airspy_board_id_name");
    if (my_airspy_board_id_name == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_id_name");
        return false;
    }

    my_airspy_list_devices = (pfn_airspy_list_devices)
        GETPROCADDRESS(Handle, "airspy_list_devices");
    if (my_airspy_list_devices == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_list_devices");
        return false;
    }

    my_airspy_board_partid_serialno_read = (pfn_airspy_board_partid_serialno_read)
        GETPROCADDRESS(Handle, "airspy_board_partid_serialno_read");
    if (my_airspy_board_partid_serialno_read == NULL) {
        log(DEV_AIRSPY, LOG_MIN, "Could not find airspy_board_partid_serialno_read");
        return false;
    }

    return true;
}

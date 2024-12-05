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
 *	recoding and taking parts for the airspyRadio interface
 *	for the Qt-DAB program
 *	jan van Katwijk
 *	Lazy Chair Computing
 */
#ifndef AIRSPY_HANDLER_H
#define AIRSPY_HANDLER_H

#include "constants.h"
#include "device-handler.h"
#include "libairspy/airspy.h"
#include "ringbuffer.h"
#include <atomic>
#include <vector>
#define MAP_RATIO 1000

extern "C" {
typedef int (*pfn_airspy_init)(void);
typedef int (*pfn_airspy_exit)(void);
typedef int (*pfn_airspy_open)(struct airspy_device**);
typedef int (*pfn_airspy_open_sn)(struct airspy_device**, uint64_t);
typedef int (*pfn_airspy_close)(struct airspy_device*);
typedef int (*pfn_airspy_get_samplerates)(struct airspy_device* device,
    uint32_t* buffer,
    const uint32_t len);
typedef int (*pfn_airspy_set_samplerate)(struct airspy_device* device,
    uint32_t samplerate);
typedef int (*pfn_airspy_start_rx)(struct airspy_device* device,
    airspy_sample_block_cb_fn callback,
    void* rx_ctx);
typedef int (*pfn_airspy_stop_rx)(struct airspy_device* device);

typedef int (*pfn_airspy_set_sample_type)(struct airspy_device*,
    airspy_sample_type);
typedef int (*pfn_airspy_set_freq)(struct airspy_device* device,
    const uint32_t freq_hz);

typedef int (*pfn_airspy_set_lna_gain)(struct airspy_device* device,
    uint8_t value);

typedef int (*pfn_airspy_set_mixer_gain)(struct airspy_device* device,
    uint8_t value);

typedef int (*pfn_airspy_set_vga_gain)(struct airspy_device* device,
    uint8_t value);
typedef int (*pfn_airspy_set_lna_agc)(struct airspy_device* device,
    uint8_t value);
typedef int (*pfn_airspy_set_mixer_agc)(struct airspy_device* device,
    uint8_t value);

typedef int (*pfn_airspy_set_rf_bias)(struct airspy_device* dev,
    uint8_t value);

typedef const char* (*pfn_airspy_error_name)(enum airspy_error errcode);
typedef int (*pfn_airspy_board_id_read)(struct airspy_device*,
    uint8_t*);
typedef const char* (*pfn_airspy_board_id_name)(enum airspy_board_id board_id);
typedef int (*pfn_airspy_board_partid_serialno_read)(struct airspy_device* device, airspy_read_partid_serialno_t* read_partid_serialno);

typedef int (*pfn_airspy_set_linearity_gain)(struct airspy_device* device, uint8_t value);
typedef int (*pfn_airspy_set_sensitivity_gain)(struct airspy_device* device, uint8_t value);

typedef int (*pfn_airspy_list_devices)(uint64_t *, int);
}

class airspyHandler : public deviceHandler {
    Q_OBJECT
public:
    airspyHandler(void);
    ~airspyHandler(void);
    int32_t devices(deviceStrings *, int);
    bool setDevice(const char *);
    bool restartReader(int32_t);
    void stopReader(void);
    int32_t getSamples(std::complex<float> *v,
        int32_t size,
        agcStats* stats);
    int32_t Samples(void);
    void resetBuffer(void);
    int16_t bitDepth(void);
    int16_t currentTab;
    void getSwAGCRange(int *, int *);
    void getIfRange(int *, int *);
    void setIfGain(int);
    void setAgcControl(int);

private:
    int ifGain;
    bool agcControl;
    bool deviceOpen(uint64_t);
    bool load_airspyFunctions(void);

    pfn_airspy_init my_airspy_init;
    pfn_airspy_exit my_airspy_exit;
    pfn_airspy_error_name my_airspy_error_name;
    pfn_airspy_list_devices my_airspy_list_devices;
    pfn_airspy_open my_airspy_open;
    pfn_airspy_open_sn my_airspy_open_sn;
    pfn_airspy_close my_airspy_close;
    pfn_airspy_get_samplerates my_airspy_get_samplerates;
    pfn_airspy_set_samplerate my_airspy_set_samplerate;
    pfn_airspy_start_rx my_airspy_start_rx;
    pfn_airspy_stop_rx my_airspy_stop_rx;
    pfn_airspy_set_sample_type my_airspy_set_sample_type;
    pfn_airspy_set_freq my_airspy_set_freq;
    pfn_airspy_set_lna_gain my_airspy_set_lna_gain;
    pfn_airspy_set_mixer_gain my_airspy_set_mixer_gain;
    pfn_airspy_set_vga_gain my_airspy_set_vga_gain;
    pfn_airspy_set_linearity_gain my_airspy_set_linearity_gain;
    pfn_airspy_set_sensitivity_gain my_airspy_set_sensitivity_gain;
    pfn_airspy_set_lna_agc my_airspy_set_lna_agc;
    pfn_airspy_set_mixer_agc my_airspy_set_mixer_agc;
    pfn_airspy_set_rf_bias my_airspy_set_rf_bias;
    pfn_airspy_board_id_read my_airspy_board_id_read;
    pfn_airspy_board_id_name my_airspy_board_id_name;
    pfn_airspy_board_partid_serialno_read
        my_airspy_board_partid_serialno_read;

    HINSTANCE Handle_usb;
    HINSTANCE Handle;
    bool libraryLoaded;
    std::atomic<bool> running;
    bool open;
    uint64_t currentId;

    RingBuffer<std::complex<int32_t>> *theBuffer;
    struct airspy_device* device;
    static int callback(airspy_transfer_t*);
    int data_available(int16_t *, int);
};
#endif

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

#ifndef PLUTO_HANDLER_H
#define PLUTO_HANDLER_H

#include "constants.h"
#include "device-handler.h"
#include "ringbuffer.h"
#include <QtNetwork>
#include <atomic>
#include <iio.h>

#ifndef PLUTO_RATE
#define PLUTO_RATE 2100000
#define DAB_RATE 2048000
#define DIVIDER 1000
#define CONV_SIZE (PLUTO_RATE / DIVIDER)
#endif

class plutoHandler : public deviceHandler {
    Q_OBJECT

public:
    plutoHandler();
    ~plutoHandler();
    bool restartReader(int32_t);
    void stopReader();
    int32_t getSamples(std::complex<float>*,
        int32_t,
        agcStats* stats);
    int32_t Samples();
    void resetBuffer();
    int16_t bitDepth();
    void setIfGain(int);
    void setAgcControl(int);

private:
    RingBuffer<std::complex<float>> _I_Buffer;
    int gainControl;
    bool agcMode;

    void run();
    int32_t vfoFrequency;
    std::atomic<bool> running;

    // configuration items
    struct iio_device* phys_dev;
    int64_t bw_hz; // Analog banwidth in Hz
    int64_t fs_hz; // Baseband sample rate in Hz
    int64_t lo_hz; // Local oscillator frequency in Hz
    bool get_ad9361_stream_ch(struct iio_context* ctx,
        struct iio_device* dev,
        int chid,
        struct iio_channel**);

    const char* rfport; // Port name
    struct iio_channel* lo_channel;
    struct iio_channel* gain_channel;
    struct iio_device* rx;
    struct iio_context* ctx;
    struct iio_channel* rx0_i;
    struct iio_channel* rx0_q;
    struct iio_buffer* rxbuf;
    bool connected;
    std::complex<float> convBuffer[CONV_SIZE + 1];
    int convIndex;
    int16_t mapTable_int[DAB_RATE / DIVIDER];
    float mapTable_float[DAB_RATE / DIVIDER];
};
#endif

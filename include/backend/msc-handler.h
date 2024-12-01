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
 *    Taken from Qt-DAB, with bug fixes and enhancements.
 *
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef MSC_HANDLER_H
#define MSC_HANDLER_H

#ifdef __MSC_THREAD__
#include <QSemaphore>
#include <QThread>
#include <QWaitCondition>
#endif
#include "constants.h"
#include "dab-params.h"
#include "fft-handler.h"
#include "freq-interleaver.h"
#include "phasetable.h"
#include "ringbuffer.h"
#include "services.h"
#include <QMutex>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <vector>

class RadioInterface;
class Backend;

#ifdef __MSC_THREAD__
class mscHandler : public QThread {
#else
class mscHandler {
#endif
  public:
    mscHandler(RadioInterface *, uint8_t, RingBuffer<uint8_t> *);
    ~mscHandler();
    void processBlock_0(std::complex<float> *);
    void process_Msc(std::complex<float> *, int);
    bool set_Channel(serviceDescriptor *, RingBuffer<int16_t> *,
                     RingBuffer<uint8_t> *);
    void reset_Channel();
    void stopService(serviceDescriptor *);
    void reset_Buffers();

  private:
    void process_mscBlock(std::vector<int16_t>, int16_t);
    RadioInterface *myRadioInterface;
    RingBuffer<uint8_t> *dataBuffer;
    RingBuffer<uint8_t> *frameBuffer;
    dabParams params;
    fftHandler my_fftHandler;
    std::complex<float> *fft_buffer;
    std::vector<complex<float>> phaseReference;

    interLeaver myMapper;
    QMutex locker;
    bool audioService;
    std::vector<Backend *> theBackends;
    std::vector<int16_t> cifVector;
    int16_t cifCount;
    int16_t blkCount;
    std::atomic<bool> work_to_be_done;
    int16_t BitsperBlock;
    std::vector<int16_t> ibits;

    int16_t numberofblocksperCIF;
    int16_t blockCount;
    void processMsc(int32_t n);
    QMutex helper;
    int nrBlocks;
#ifdef __MSC_THREAD__
    void processBlock_0();
    std::vector<std::vector<std::complex<float>>> command;
    int16_t amount;
    void run();
    QSemaphore bufferSpace;
    QWaitCondition commandHandler;
    std::atomic<bool> running;
#endif
};
#endif

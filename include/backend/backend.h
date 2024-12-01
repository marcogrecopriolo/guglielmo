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
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef BACKEND_H
#define BACKEND_H

#include <QSemaphore>
#include <vector>
#ifdef __THREADED_BACKEND
#include <QThread>
#include <atomic>
#endif
#include "backend-deconvolver.h"
#include "backend-driver.h"
#include "ringbuffer.h"
#include <cstdio>

#define NUMBER_SLOTS 25
class RadioInterface;

#ifdef __THREADED_BACKEND
class Backend : public QThread {
#else
class Backend {
#endif
  public:
    Backend(RadioInterface *mr, serviceDescriptor *d, RingBuffer<int16_t> *,
            RingBuffer<uint8_t> *, RingBuffer<uint8_t> *);
    ~Backend();
    int32_t process(int16_t *, int16_t);
    void stopRunning();

    //	we need sometimes to access the key parameters for decoding
    int serviceId;
    int startAddr;
    int Length;
    bool shortForm;
    int protLevel;
    int16_t bitRate;
    int16_t subChId;
    QString serviceName;

  private:
    backendDeconvolver deconvolver;
    std::vector<uint8_t> outV;
    backendDriver driver;
#ifdef __THREADED_BACKEND
    void run();
    atomic<bool> running;
    QSemaphore freeSlots;
    QSemaphore usedSlots;
    std::vector<int16_t> theData[NUMBER_SLOTS];
    int16_t nextIn;
    int16_t nextOut;
#endif
    void processSegment(int16_t *Data);
    RadioInterface *radioInterface;

    int16_t fragmentSize;
    std::vector<std::vector<int16_t>> interleaveData;
    std::vector<int16_t> tempX;
    int16_t countforInterleaver;
    int16_t interleaverIndex;
    std::vector<uint8_t> disperseVector;
};
#endif

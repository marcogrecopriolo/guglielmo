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
#include "Qt-audiodevice.h"

// Create a "device"
Qt_AudioDevice::Qt_AudioDevice(RingBuffer<float>* Buffer,
    QObject* parent)
    : QIODevice(parent) {
    this->Buffer = Buffer;
}

Qt_AudioDevice::~Qt_AudioDevice(void) {
}

void Qt_AudioDevice::start(void) {
    open(QIODevice::ReadOnly);
}

void Qt_AudioDevice::stop(void) {
    Buffer->FlushRingBuffer();
    close();
}

// we always return "len" bytes
qint64 Qt_AudioDevice::readData(char* buffer, qint64 maxSize) {
    qint64 amount = 0;

    // "maxSize" is the requested size in bytes
    // "amount" is in floats
    amount = Buffer->getDataFromBuffer(buffer, maxSize / sizeof(float));

    if (amount >= 0 && sizeof(float) * amount < maxSize) {
        qint64 i;
        for (i = amount * sizeof(float); i < maxSize; i++)
            buffer[i] = 0;
    }

    return maxSize;
}

// usused here
qint64 Qt_AudioDevice::writeData(const char* data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

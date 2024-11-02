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
#ifndef QT_AUDIODEVICE_H
#define QT_AUDIODEVICE_H

#include "constants.h"
#include "ringbuffer.h"
#include <QIODevice>
#include <QObject>

class Qt_AudioDevice: public QIODevice {
    Q_OBJECT

public:
    Qt_AudioDevice(RingBuffer<float>*, QObject*);
    ~Qt_AudioDevice(void);

    void start(void);
    void stop(void);

    qint64 readData(char* data, qint64 maxlen);
    qint64 writeData(const char* data, qint64 len);

private:
    RingBuffer<float>* Buffer;
};
#endif

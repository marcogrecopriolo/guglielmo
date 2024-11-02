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
#ifndef QT_AUDIO_H
#define QT_AUDIO_H

#include <QAudioFormat>
#include <stdio.h>
#if QT_VERSION >= 0x060000
#include <QAudioSink>
#else
#include <QAudioOutput>
#endif
#include "Qt-audiodevice.h"
#include "audio-base.h"
#include "constants.h"
#include "ringbuffer.h"
#include <QTimer>

class Qt_Audio: public audioBase {
    Q_OBJECT

public:
    Qt_Audio(void);
    ~Qt_Audio(void);
    void stop(void);
    void restart(void);
    void setVolume(qreal);
    void audioOutput(float*, int32_t);

private:
    void setParams(int32_t);
    QAudioFormat AudioFormat;
#if QT_VERSION >= 0x060000
    QAudioSink* theAudioOutput;
#else
    QAudioOutput* theAudioOutput;
#endif
    Qt_AudioDevice* theAudioDevice;
    RingBuffer<float>* Buffer;
    QAudio::State currentState;
    int32_t outputRate;
    qreal volume;

private slots:
    void handleStateChanged(QAudio::State newState);
};
#endif

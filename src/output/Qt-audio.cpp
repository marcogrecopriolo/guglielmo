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

#include "Qt-audio.h"
#include "Qt-audiodevice.h"
#include <stdio.h>
#if QT_VERSION >= 0x060000
#include <QAudioDevice>
#include <QMediaDevices>
#endif
#include "logging.h"

Qt_Audio::Qt_Audio(void) {
    Buffer = new RingBuffer<float>(8 * 32768);
    outputRate = 48000; // default
    theAudioDevice = new Qt_AudioDevice(Buffer, this);
    theAudioOutput = nullptr;
    setParams(outputRate);
    volume = 0.5;
    if (theAudioOutput != nullptr) {
        theAudioOutput->setVolume(volume);
    }
}

Qt_Audio::~Qt_Audio(void) {
    if (theAudioOutput != nullptr)
        delete theAudioOutput;
    delete theAudioDevice;
    delete Buffer;
}

// Note that audioBase functions have - if needed - the rate
// converted.  This functions overrides the one in audioBase
void Qt_Audio::audioOutput(float* fragment, int32_t size) {
    if (theAudioDevice != nullptr) {
        Buffer->putDataIntoBuffer(fragment, 2 * size);
    }
}

void Qt_Audio::setParams(int outputRate) {
    if (theAudioOutput != nullptr) {
        delete theAudioOutput;
        theAudioOutput = nullptr;
    }

    AudioFormat.setSampleRate(outputRate);
    AudioFormat.setChannelCount(2);
#if QT_VERSION >= 0x060000
    AudioFormat.setSampleFormat(QAudioFormat::Float);
    QAudioDevice info(QMediaDevices::defaultAudioOutput());
#else
    AudioFormat.setSampleSize(sizeof(float) * 8);
    AudioFormat.setCodec("audio/pcm");
    AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    AudioFormat.setSampleType(QAudioFormat::Float);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
#endif

    if (!info.isFormatSupported(AudioFormat)) {
        log(LOG_SOUND, LOG_MIN, "format cannot be handled");
        return;
    }

#if QT_VERSION >= 0x060000
    theAudioOutput = new QAudioSink(AudioFormat, this);
#else
    theAudioOutput = new QAudioOutput(AudioFormat, this);
#endif
    connect(theAudioOutput, SIGNAL(stateChanged(QAudio::State)),
        this, SLOT(handleStateChanged(QAudio::State)));

    currentState = theAudioOutput->state();
}

void Qt_Audio::stop(void) {
    if (theAudioDevice != nullptr) {
        theAudioDevice->stop();
        if (theAudioOutput != nullptr)
            theAudioOutput->stop();
    }
}

void Qt_Audio::restart(void) {
    if (theAudioDevice != nullptr) {
        theAudioDevice->start();
        if (theAudioOutput != nullptr) {
            theAudioOutput->start(theAudioDevice);
            theAudioOutput->setVolume(volume);
        }
    }
}

void Qt_Audio::setVolume(qreal v) {
    volume = QAudio::convertVolume(v,
        QAudio::LogarithmicVolumeScale,
        QAudio::LinearVolumeScale);
    if (theAudioOutput != nullptr)
        theAudioOutput->setVolume(volume);
}

void Qt_Audio::handleStateChanged(QAudio::State newState) {
    currentState = newState;
    switch (currentState) {
    case QAudio::IdleState:
        if (theAudioOutput != nullptr)
            theAudioOutput->stop();
        break;

    default:
        break;
    }
}

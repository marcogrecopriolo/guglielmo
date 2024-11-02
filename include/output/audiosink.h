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
 *    Copyright (C)  2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef AUDIO_SINK_H
#define AUDIO_SINK_H
#include "audio-base.h"
#include "constants.h"
#include "ringbuffer.h"
#include <cstdio>
#include <portaudio.h>

struct channelList {
    const char* name;
    int16_t dev;
    PaTime latency;
};

class audioSink: public audioBase {
    Q_OBJECT

public:
    audioSink(int16_t);
    ~audioSink();
    bool setupChannels(void);
    void stop();
    void restart(void);
    void setVolume(qreal);
    bool selectDevice(int16_t);
    int16_t numberOfDevices();
    int16_t defaultDevice(void);
    const char* outputChannel(int16_t);

private:
    int32_t cardRate();

    bool OutputrateIsSupported(int16_t, int32_t);
    void audioOutput(float*, int32_t);
    float volume;
    int32_t CardRate;
    int16_t latency;
    int32_t size;
    bool portAudio;
    bool writerRunning;
    int16_t maxDevices;
    int16_t numDevices;
    int16_t defDevice;
    int paCallbackReturn;
    int16_t bufSize;
    PaStream* ostream;
    RingBuffer<float> _O_Buffer;
    PaStreamParameters outputParameters;

    channelList* outTable;

protected:
    static int paCallback_o(const void* input,
        void* output,
        unsigned long framesperBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);
};
#endif

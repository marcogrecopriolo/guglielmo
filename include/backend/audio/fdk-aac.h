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
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *	Use the fdk-aac library.
 */

#ifdef __WITH_FDK_AAC__
#ifndef FDK_AAC_H
#define FDK_AAC_H

#include "ringbuffer.h"
#include <QObject>
#include <aacdecoder_lib.h>
#include <stdint.h>

typedef struct {
    int rfa;
    int dacRate;
    int sbrFlag;
    int psFlag;
    int aacChannelMode;
    int mpegSurround;
    int CoreChConfig;
    int CoreSrIndex;
    int ExtensionSrIndex;
} stream_parms;

class RadioInterface;

/*
 *	fdkAAC is an interface to the fdk-aac library,
 *	using the LOAS protocol
 */
class fdkAAC : public QObject {
    Q_OBJECT
  public:
    fdkAAC(RadioInterface *mr, RingBuffer<int16_t> *buffer);
    ~fdkAAC();

    int16_t MP42PCM(stream_parms *sp, uint8_t packet[], int16_t packetLength);

  private:
    RingBuffer<int16_t> *audioBuffer;
    bool working;
    HANDLE_AACDECODER handle;
  signals:
    void newAudio(int, int);
};
#endif
#endif

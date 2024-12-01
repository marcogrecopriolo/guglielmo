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

#include "backend-driver.h"
#include "data-processor.h"
#include "mp2processor.h"
#include "mp4processor.h"

//	Driver program for the selected backend. Embodying that in a
//	separate class makes the "Backend" class simpler.

backendDriver::backendDriver(RadioInterface *mr, serviceDescriptor *d,
                             RingBuffer<int16_t> *audioBuffer,
                             RingBuffer<uint8_t> *dataBuffer,
                             RingBuffer<uint8_t> *frameBuffer) {
    if (d->type == AUDIO_SERVICE) {
        if (!((audiodata *)d)->isDABplus()) {
            theProcessor =
                new mp2Processor(mr, d->bitRate, audioBuffer, frameBuffer);
        } else {
            theProcessor = new mp4Processor(mr, d->bitRate, audioBuffer,
                                            frameBuffer, d->procMode);
        }
    } else if (d->type == PACKET_SERVICE)
        theProcessor = new dataProcessor(mr, (packetdata *)d, dataBuffer);
}

backendDriver::~backendDriver() { delete theProcessor; }

void backendDriver::addtoFrame(std::vector<uint8_t> theData) {
    theProcessor->addtoFrame(theData);
}

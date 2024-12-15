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
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017, 2018, 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */
#include "timesyncer.h"
#include "math-helper.h"
#include "sample-reader.h"

#define C_LEVEL_SIZE 50

timeSyncer::timeSyncer(sampleReader *mr) { myReader = mr; }

timeSyncer::~timeSyncer() {}

int timeSyncer::sync(int T_null, int T_F) {
    float cLevel = 0;
    int counter = 0;
    _VLA(float, envBuffer, syncBufferSize);
    const int syncBufferMask = syncBufferSize - 1;
    int i;

    syncBufferIndex = 0;
    for (i = 0; i < C_LEVEL_SIZE; i++) {
        std::complex<float> sample = myReader->getSample(0);
        envBuffer[syncBufferIndex] = fastMagnitude(sample);
        cLevel += envBuffer[syncBufferIndex];
        syncBufferIndex++;
    }
    // SyncOnNull:
    counter = 0;
    while (cLevel / C_LEVEL_SIZE > 0.55 * myReader->get_sLevel()) {
        std::complex<float> sample = myReader->getSample(0);
        //	         myReader. getSample (coarseOffset + fineCorrector);
        envBuffer[syncBufferIndex] = fastMagnitude(sample);
        //      update the levels
        cLevel += envBuffer[syncBufferIndex] -
                  envBuffer[(syncBufferIndex - C_LEVEL_SIZE) & syncBufferMask];
        syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
        counter++;
        if (counter > T_F) { // hopeless
            return NO_DIP_FOUND;
        }
    }
    /*
     *     It seemed we found a dip that started app 65/100 * 50 samples
     * earlier. We now start looking for the end of the null period.
     */
    counter = 0;
    // SyncOnEndNull:
    while (cLevel / C_LEVEL_SIZE < 0.75 * myReader->get_sLevel()) {
        std::complex<float> sample = myReader->getSample(0);
        envBuffer[syncBufferIndex] = fastMagnitude(sample);
        //      update the levels
        cLevel += envBuffer[syncBufferIndex] -
                  envBuffer[(syncBufferIndex - C_LEVEL_SIZE) & syncBufferMask];
        syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
        counter++;
        if (counter > T_null + 50) { // hopeless
            return NO_END_OF_DIP_FOUND;
        }
    }

    return TIMESYNC_ESTABLISHED;
}

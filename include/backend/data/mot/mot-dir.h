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
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *	MOT handling is a crime, here we have a single class responsible
 *	for handling a MOT directory
 */

#ifndef MOT_DIRECTORY_H
#define MOT_DIRECTORY_H

#include "mot-object.h"
#include <QHash>

class RadioInterface;

class motDirectory {
  public:
    motDirectory(RadioInterface *, uint16_t, int16_t, int32_t, int16_t,
                 uint8_t *, motCache *cache);
    ~motDirectory();
    void markObjectComplete();
    motObject *getHandle(uint16_t);
    void directorySegment(uint16_t transportId, uint8_t *segment,
                          int16_t segmentNumber, int32_t segmentSize,
                          bool lastSegment);
    uint16_t getTransportId();

  private:
    void analyseDirectory();
    uint16_t transportId;

    RadioInterface *myRadioInterface;
    uint8_t *dir_segments;
    bool marked[512];
    int16_t dir_segmentSize;
    int16_t num_dirSegments;
    int16_t dirSize;
    int16_t numObjects;
    int16_t doneObjects;
    motCache motComponents;
    motCache *cache;
};
#endif

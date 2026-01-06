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

#include "mot-dir.h"
#include "logging.h"

motDirectory::motDirectory(RadioInterface *mr, uint16_t transportId,
                           int16_t segmentSize, int32_t dirSize,
                           int16_t objects, uint8_t *segment,
			   motCache *cache) {
    int16_t i;

    log(LOG_MOT, LOG_CHATTY, "new directory transport %x size %i objects %i", transportId, dirSize, objects);
    this->myRadioInterface = mr;
    for (i = 0; i < 512; i++)
        marked[i] = false;
    num_dirSegments = -1;
    this->transportId = transportId;
    this->dirSize = dirSize;
    this->numObjects = objects;
    doneObjects = 0;
    this->dir_segmentSize = segmentSize;
    dir_segments = new uint8_t[dirSize];
    memcpy(&dir_segments[0], segment, segmentSize);
    marked[0] = true;
    this->cache = cache;
}

motDirectory::~motDirectory() {
    log(LOG_MOT, LOG_CHATTY, "discarding directory %x", transportId);
    delete[] dir_segments;
    for (auto it = motComponents.constBegin(); it != motComponents.constEnd(); ++it)
	delete it.value();
    motComponents.clear();
    if (cache != nullptr) {
	for (auto it = cache->constBegin(); it != cache->constEnd(); ++it)
	    delete it.value();
	cache->clear();
    }
}

motObject *motDirectory::getHandle(uint16_t transportId) {

    // returns nullptr as default value of a ptr
    return motComponents.value(transportId);
}

void motDirectory::markObjectComplete() {
    doneObjects++;
    if (doneObjects < numObjects) {
	log(LOG_MOT, LOG_VERBOSE, "mot directory %x completed %i / %i objects", transportId, doneObjects, numObjects);
    } else {
	log(LOG_MOT, LOG_CHATTY, "mot directory %x completed %i objects", transportId, numObjects);
    }
}

// unfortunately, directory segments do not need to come in in order
void motDirectory::directorySegment(uint16_t transportId, uint8_t *segment,
                                    int16_t segmentNumber, int32_t segmentSize,
                                    bool lastSegment) {
    int16_t i;

    if (this->transportId != transportId)
        return;

    if (this->marked[segmentNumber])
        return;
    if (lastSegment)
        this->num_dirSegments = segmentNumber + 1;

    log(LOG_MOT, LOG_VERBOSE, "directory %x new segment %i last %i", transportId, segmentNumber, lastSegment);
    this->marked[segmentNumber] = true;
    uint8_t *address = &dir_segments[segmentNumber * dir_segmentSize];
    memcpy(address, segment, segmentSize);

    // we are "complete" if we know the number of segments and
    // all segments are "in"
    if (this->num_dirSegments != -1) {
        for (i = 0; i < this->num_dirSegments; i++)
            if (!this->marked[i])
                return;
    }

    // yes we have all data to build up the directory
    analyseDirectory();
}

void motDirectory::analyseDirectory() {
    uint32_t currentBase = 11;		// in bytes
    uint8_t *data = dir_segments;
    uint16_t extensionLength =
        (dir_segments[currentBase] << 8) | data[currentBase + 1];

    int16_t i;

    currentBase += 2 + extensionLength;
    for (i = 0; i < numObjects; i++) {
        uint16_t transportId = (data[currentBase] << 8) | data[currentBase + 1];
        if (transportId == 0)		// just a dummy
            break;
        uint8_t *segment = &data[currentBase + 2];
        motObject *handle = new motObject(myRadioInterface, motObject::Directory, transportId,
                                          segment, -1, false);
	if (cache != nullptr) {
	    motObject *cached = cache->take(transportId);
	    if (cached != nullptr) {
		if (handle->mergeObject(cached))
		    markObjectComplete();
		delete cached;
	    }
	}
        currentBase += 2 + handle->getHeaderSize();
	motComponents.insert(transportId, handle);
    }

    // we no longer need objects received prior to the directory
    if (cache != nullptr) {
	for (auto it = cache->constBegin(); it != cache->constEnd(); ++it)
	    delete it.value();
	cache->clear();
	cache = nullptr;
    }
}

uint16_t motDirectory::getTransportId() { return transportId; }

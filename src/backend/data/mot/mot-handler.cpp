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
 */

//	Interface between msc packages and real MOT handling

#include "mot-handler.h"
#include "bits-helper.h"
#include "mot-dir.h"
#include "mot-object.h"
#include "radio.h"

motHandler::motHandler(RadioInterface *mr) {
    myRadioInterface = mr;
    orderNumber = 0;

    currentDirectory = nullptr;
    currentObject = nullptr;
    cache = new motCache();
}

motHandler::~motHandler() {
    if (currentObject != nullptr)
        delete currentObject;
    if (currentDirectory != nullptr)
        delete currentDirectory;
    if (cache != nullptr) {
	for (auto it = cache->constBegin(); it != cache->constEnd(); ++it)
	    delete it.value();
	cache->clear();
    }
}

void motHandler::add_mscDatagroup(std::vector<uint8_t> msc) {
    uint8_t *data = (uint8_t *)(msc.data());
    bool extensionFlag = getBits_1(data, 0) != 0;
    bool crcFlag = getBits_1(data, 1) != 0;
    bool segmentFlag = getBits_1(data, 2) != 0;
    bool userAccessFlag = getBits_1(data, 3) != 0;
    uint8_t groupType = getBits_4(data, 4);
    uint8_t CI = getBits_4(data, 8);
    int32_t next = 16; // bits
    bool lastFlag = false;
    uint16_t segmentNumber = 0;
    bool transportIdFlag = false;
    uint16_t transportId = 0;
    uint8_t lengthInd;
    int32_t i;
    motObject *h;

    (void)CI;
    if (msc.size() <= 0) {
        return;
    }

    if (crcFlag && !check_CRC_bits(data, msc.size()))
        return;

    if (extensionFlag)
        next += 16;

    if (segmentFlag) {
        lastFlag = getBits_1(data, next) != 0;
        segmentNumber = getBits(data, next + 1, 15);
        next += 16;
    }

    if (userAccessFlag) {
        transportIdFlag = getBits_1(data, next + 3);
        lengthInd = getBits_4(data, next + 4);
        next += 8;
        if (transportIdFlag) {
            transportId = getBits(data, next, 16);
        }
        next += lengthInd * 8;
    }

    int32_t sizeinBits = msc.size() - next - (crcFlag != 0 ? 16 : 0);

    if (!transportIdFlag)
        return;

    std::vector<uint8_t> motVector;
    motVector.resize(sizeinBits / 8);
    for (i = 0; i < sizeinBits / 8; i++)
        motVector[i] = getBits_8(data, next + 8 * i);

    uint32_t segmentSize = ((motVector[0] & 0x1F) << 8) | motVector[1];

    // This is all described in ETSI EN301234 section 5.1
    switch (groupType) {

    // MOT object header, never in directory mode, only ever one at any one time (discard any prior data).
    case 3:

	// we only handle non segemented headers
	if (segmentNumber != 0)
	    break;

	if  (currentObject != nullptr || transportId != currentObject->getTransportId())
	    delete currentObject;
        currentObject = new motObject(myRadioInterface,
                              motObject::Header,
                              transportId, &motVector[2], segmentSize,
                              lastFlag);
        break;

    // MOT object body, this could be in header mode or directory mode, plus, we may not
    // even have the directory yet
    case 4:
	if (currentObject != nullptr && currentObject->getTransportId() == transportId) {
	     (void) currentObject->addBodySegment(&motVector[2], segmentNumber, segmentSize, lastFlag);
	     break;
	}
	if (currentDirectory != nullptr) {
	    h = currentDirectory->getHandle(transportId);
	    if (h != nullptr && h->addBodySegment(&motVector[2], segmentNumber, segmentSize, lastFlag))
		currentDirectory->markObjectComplete();
	} else if (cache != nullptr) {

	    // returns null on missing entry as defaul;t value of a pointer
	    motObject *h = cache->value(transportId);
	    if ((h == nullptr) && (segmentNumber == 0)) {
		h = new motObject(myRadioInterface,
                         motObject::Cache,
                         transportId, &motVector[2], segmentSize,
                         lastFlag);
		cache->insert(transportId, h);
	    }
	    if (h != nullptr)
		(void) h->addBodySegment(&motVector[2], segmentNumber, segmentSize, lastFlag);
        }
	break;

    // MOT directory
    case 6:
        if (segmentNumber == 0) {
            if (currentDirectory != nullptr) {

		// TODO - should we not refresh the current one?
                if (currentDirectory->getTransportId() == transportId)
                    break;

		// a different one, start fromm scratch
                delete currentDirectory;
	    }

            int32_t segmentSize = ((motVector[0] & 0x1F) << 8) | motVector[1];
            uint8_t *segment = &motVector[2];
            int dirSize = ((segment[0] & 0x3F) << 24) | ((segment[1]) << 16) |
                          ((segment[2]) << 8) | segment[3];
            uint16_t numObjects = (segment[4] << 8) | segment[5];
            //	         int32_t period = (segment [6] << 16) |
            //	                          (segment [7] <<  8) | segment [8];
            //	         int32_t segSize
            //	                        = ((segment [9] & 0x1F) << 8) | segment [10];
            currentDirectory =
                new motDirectory(myRadioInterface, transportId, segmentSize,
                                 dirSize, numObjects, segment, cache);
	        cache = nullptr;
        } else {
            if ((currentDirectory == nullptr) ||
                (currentDirectory->getTransportId() != transportId))
                break;
            currentDirectory->directorySegment(transportId, &motVector[2],
                                           segmentNumber, segmentSize,
                                           lastFlag);
        }
        break;

    default:
        return;
    }
}

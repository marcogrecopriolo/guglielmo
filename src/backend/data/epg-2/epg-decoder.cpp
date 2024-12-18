/*
 *  *    Copyright (C) 2021
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
 *    Copyright (C) 2013 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "epg-decoder.h"
#include "logging.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define EPG_TAG 0X02

epgDecoder::epgDecoder() {}

epgDecoder::~epgDecoder() {}

static uint8_t bitTable[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

int epgDecoder::getBit(uint8_t *v, int bitnr) {
    int bytenr = bitnr / 8;

    bitnr = bitnr % 8;
    return (v[bytenr] & bitTable[bitnr]) != 0 ? 1 : 0;
}

uint32_t epgDecoder::getBits(uint8_t *v, int bitnr, int length) {
    uint16_t res = 0;
    for (int i = 0; i < length; i++) {
        res <<= 1;
        res |= getBit(v, bitnr + i);
    }
    return res;
}

int epgDecoder::process_epg(uint8_t *v, int e_length, uint32_t SId) {
    int ind = 0;
    uint8_t tag = v[0];
    int length = v[1];
    int index = 0;

    this->SId = SId;
    if (v[0] != EPG_TAG)
        return length;

    if (length == 0xFE) {
        length = (v[2] << 8) | v[3];
        index = 4;
    } else if (length == 0xFF) {
        length = (v[2] << 16) | (v[3] << 8) | v[4];
        index = 5;
    } else
        index = 2;

    int endPoint = index + length;

    while (index < endPoint)
        index = process_epgElement(v, index);
    return endPoint;
}

int32_t epgDecoder::process_epgElement(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    int endPoint = index + length;

    switch (tag) {
    case 0x04: // token table element
        while (index < endPoint)
            index = process_token(v, index);
        return endPoint;

    case 0x05: // obsolete
        return endPoint;

    case 0x06: // obsolete default language
        return endPoint;

    case 0x20: // programmeGroups
        return endPoint;

    case 0x21: // schedule
        while (index < endPoint)
            index = schedule_element(v, index);
        return endPoint;

    case 0x22: // alternate source
        return endPoint;

    default:
        log(LOG_EPG, LOG_CHATTY, "Missed tag %d in epg", tag);
        return endPoint;
    }
}

int epgDecoder::schedule_element(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];
    progDesc theElement;

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    int endPoint = index + length;

    log(LOG_EPG, LOG_CHATTY, "schedule element %x (length %d)", tag, length);
    switch (tag) {
    case 0x1C: // programme
        theElement.clean();
        while (index < endPoint) {
            index = programme_element(v, index, &theElement);
        }
        record(&theElement);
        return endPoint;

    case 0x24: // scope
        log(LOG_EPG, LOG_CHATTY, "we have a service scope");
        return endPoint;

    //	the attributes
    case 0x80: // version
        process_483(v, index, length);
        return endPoint;

    case 0x81: { // creation time
        int time = process_474(v, index, length);
        log(LOG_EPG, LOG_CHATTY, "schedule created  at %.2d:%.2d", time / 60,
            time % 60);
        return endPoint;
    }

    case 0x82: // originator
        return endPoint;

    default:
        log(LOG_EPG, LOG_CHATTY, "missed tag $d in schedule", tag);
        return index + length;
    }
}

int epgDecoder::programme_element(uint8_t *v, int index, progDesc *p) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    int endPoint = index + length;
    log(LOG_EPG, LOG_CHATTY, "programmeElement, tag %x, length %d", tag,
        length);
    switch (tag) {
    case 0x80: // Id
        p->ident = process_471(v, index, length);
        return endPoint;

    case 0x81:
        //      process_472 (v, index, length);
        return endPoint;

    case 0x82:
    case 0x83:
        return endPoint;

    case 0x84: // broadcast
        return endPoint;

    case 0x86: // xml:lan
        return endPoint;

    case 0x10: // shortName
        p->shortName = process_481(v, index, length);
        return endPoint;

    case 0x11: // mediumName
        p->mediumName = process_481(v, index, length);
        return endPoint;

    case 0x12: // longName
        p->longName = process_481(v, index, length);
        return endPoint;

    case 0x13: // media description
        process_mediaDescription(v, index);
        return endPoint;

    case 0x14:
        while (index < endPoint)
            index = genre_element(v, index);
        return endPoint;

    case 0x19: // location
        process_location(v, index, p);
        return endPoint;

    default:
        log(LOG_EPG, LOG_CHATTY, "missed tag with programme %x", tag);
        return endPoint;
    }
}

int epgDecoder::process_mediaDescription(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    log(LOG_EPG, LOG_CHATTY, "mediaDescription, tag %x (%d)", tag, length);
    int endPoint = index + length;
    while (index < endPoint)
        index = multimedia(v, index);

    return endPoint;
}

int epgDecoder::process_location(uint8_t *v, int index, progDesc *p) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    int endPoint = index + length;
    while (index < endPoint)
        index = location_element(v, index, p);

    return endPoint;
}

int epgDecoder::location_element(uint8_t *v, int index, progDesc *p) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    int endPoint = index + length;

    switch (tag) {
    case 0x2C: // time
        while (index < endPoint)
            index = time_element(v, index);
        return endPoint;

    case 0x2D: // bearer
        while (index < endPoint)
            index = bearer_element(v, index);
        return endPoint;

    case 0x2F: // relative time
        return index + length;

    case 0x80: { // starttime
        p->startTime = process_474(v, index, length);
        return index + length;
    }
    case 0x81: // duration
        process_475(v, index, length);
        return index + length;
    default:
        return index + length;
    }
    return index + length;
}

int epgDecoder::multimedia(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    return index + length;
}

int epgDecoder::time_element(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    switch (tag) {
    case 0x80: // time
        //      process_474(v, index, length);
        return index + length;

    case 0x81: // duration
        process_475(v, index, length);
        return index + length;

    case 0x82: // actual time
    {
        int xx = process_474(v, index, length);
        log(LOG_EPG, LOG_CHATTY, "actual time %d %d\n", xx / 60, xx % 60);
    }
        return index + length;

    default:
        return index + length;
    }
}

int epgDecoder::genre_element(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    switch (tag) {
    case 0x80:
    case 0x81:
        return index + length;

    default: // cannot happen
        return index + length;
    }
}

int epgDecoder::bearer_element(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    switch (tag) {
    case 0x80: // time
        process_476(v, index, length);
        return index + length;

    default:
        return index + length;
    }
}

int epgDecoder::process_token(uint8_t *v, int index) {
    uint8_t tag = v[index];
    int length = v[index + 1];
    char text[length + 1];

    index += 2;
    for (int i = 0; i < length; i++)
        text[i] = v[index + i];
    text[length] = 0;
    stringTable[tag] = QString::fromUtf8(text);
    return index + length;
}

void epgDecoder::process_45(uint8_t *v, int index, int length) {}

void epgDecoder::process_46(uint8_t *v, int index, int length) {}

QString epgDecoder::process_471(uint8_t *v, int index, int length) {
    QString result = "";
    for (int i = 0; i < length; i++)
        result.append(v[index + i]);
    return result;
}

void epgDecoder::process_472(uint8_t *v, int index, int length) {
    uint32_t shortCRID = (((v[index] << 8) | v[index + 1]) << 8) | v[index + 2];

    log(LOG_EPG, LOG_MIN, "short CRID %X", shortCRID);
}

void epgDecoder::process_473(uint8_t *v, int index, int length) {}

int epgDecoder::process_474(uint8_t *v, int index, int length) {
    uint8_t ltoFlag = getBit(v, 8 * index + 19);
    uint8_t utcFlag = getBit(v, 8 * index + 20);
    int hours;
    int minutes;
    int ltoBase;

    log(LOG_EPG, LOG_CHATTY, "Handling time, utcFlag %d, ltoFlag %d", utcFlag,
        ltoFlag);
    hours = getBits(v, 8 * index + 21, 5);
    minutes = getBits(v, 8 * index + 26, 6);

    log(LOG_EPG, LOG_CHATTY, "hours = %d, minutes = %d", hours, minutes);
    if (utcFlag)
        ltoBase = 48;
    else
        ltoBase = 32;

    if (ltoFlag) {
        uint16_t halfHours = getBits(v, 8 * index + ltoBase, 8);
           log (LOG_EPG, LOG_CHATTY, "half hpurs sign %d, value %d",
//	                               halfHours & 0x20, halfHours & 0x1F);
	   if (halfHours & 0x20)
	      halfHours = - halfHours & 0x1F;
	   else
	      halfHours = halfHours & 0x1F;
	   minutes += halfHours * 30;
    }

    return hours * 60 + minutes;
}

void epgDecoder::process_475(uint8_t *v, int index, int length) {
    uint16_t duration = (v[index] << 8) | v[index + 1];

    duration /= 60;
    log(LOG_EPG, LOG_CHATTY, "duration is %d hours %d minutes", duration / 60,
        duration % 60);
}

void epgDecoder::process_476(uint8_t *v, int index, int length) {
    uint8_t ensFlag = getBit(v, 8 * index + 1);
    uint8_t sidFlag = getBit(v, 8 * index + 3);

    if (ensFlag == 1)
        if (sidFlag == 0)
            log(LOG_EPG, LOG_MIN, "SId = %X", getBits(v, 8 * index + 32, 16));

    if (ensFlag == 0)
        if (sidFlag == 0)
            log(LOG_EPG, LOG_MIN, "SId = %X", getBits(v, 8 * index + 8, 16));
}

QString epgDecoder::process_481(uint8_t *v, int index, int elength) {
    uint8_t tag = v[index];
    int length = v[index + 1];
    QString result = "";

    if (length == 0XFE) {
        length = (v[index + 2] << 8) | v[index + 3];
        index += 4;
    } else if (length == 0XFF) {
        length = (v[index + 2] << 16) | (v[index + 3] << 8) | v[index + 4];
        index += 5;
    } else
        index += 2;

    switch (tag) {
    case 0x01:
        if (length == 1)
            return stringTable[v[index]];
        else {
            char temp[length + 1];
            for (int i = 0; i < length; i++)
                temp[i] = v[index + i];
            temp[length] = 0;
            result = QString::fromUtf8(temp);
        }
        break;
    default:
        break;
    }
    return result;
}

void epgDecoder::process_483(uint8_t *v, int index, int elength) {
    log(LOG_EPG, LOG_MIN, "Version %d", (v[index + 0] << 8) | v[index + 1]);
}

void epgDecoder::record(progDesc *theElement) {
    if (theElement->startTime == -1)
        return;

    if ((theElement->mediumName == QString("")) &&
        (theElement->longName == QString("")))
        return;

    set_epgData(this->SId, theElement->startTime,
                theElement->longName != QString("") ? theElement->longName
                                                    : theElement->mediumName);
}

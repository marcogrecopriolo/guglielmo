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
 *    Taken from qt-dab, with bug fixes and enhancements.
 *
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef SERVICES_H
#define SERVICES_H

#include "dab-tables.h"

class serviceDescriptor {
public:
    uint8_t type;
    bool defined;
    uint8_t procMode;
    QString serviceName;
    int32_t SId;
    int SCIds;
    int16_t subchId;
    int16_t startAddr;
    bool shortForm;
    int16_t protLevel;
    int16_t length;
    int16_t bitRate;
    QString channel; // just for presets
public:
    serviceDescriptor() {
        defined = false;
        serviceName = "";
        procMode = __BOTH;
    }
    virtual ~serviceDescriptor() {}
};

class packetdata : public serviceDescriptor {
public:
    int16_t DSCTy;
    int16_t FEC_scheme;
    int16_t DGflag;
    int16_t appType;
    int16_t compnr;
    int16_t packetAddress;
    packetdata() {
        type = PACKET_SERVICE;
    }
};

class audiodata : public serviceDescriptor {
public:
    int16_t ASCTy;
    int16_t language;
    int16_t programType;
    int16_t compnr;
    int32_t fmFrequency;

    audiodata() {
        type = AUDIO_SERVICE;
    }

    bool isDABplus(void) {
        return ASCTy == 077;
    }

    void audioInfo(char* buf, int len) {
        snprintf(buf, len, "%s %i kBits/sec", (isDABplus() ? "DAB+" : "DAB"), bitRate);
    }

    // TODO we should really check for country and return US program types if required
    void serviceInfo(char* buf, int len) {
	if (language != 0 && programType != 0)
            snprintf(buf, len, "%s - %s", getProgramType_Not_NorthAmerica(programType), getLanguage(language));
	else if (language != 0)
            snprintf(buf, len, "%s", getLanguage(language));
	else if (programType != 0)
            snprintf(buf, len, "%s", getProgramType_Not_NorthAmerica(programType));
	else
            *buf = '\0';
    }
};
#endif

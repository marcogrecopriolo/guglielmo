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

#ifndef	DAB_TABLES_H
#define	DAB_TABLES_H
#include <cinttypes>

// https://www.etsi.org/deliver/etsi_en/300400_300499/300401/02.01.01_20/en_300401v020101a.pdf

// page 46
enum transportMechanismIdentifier {
    TMStreamAudio = 0,
    TMStreamData = 1,
    TMPacketData = 3
};

// https://www.https://www.etsi.org/deliver/etsi_ts/101700_101799/101756/02.04.01_60/ts_101756v020401p.pdf

// table 2a and 2b
enum serviceComponentType {
    SCTDabAudio = 0,
    SCTTDC = 5,
    SCTMPEG2TS = 24,
    SCTMOT = 60,
    SCTProprietary = 61,
    SCTDabPlusAudio = 63,
    SCTUndef = (int16_t) -1
};

// table 16
enum userApplicationType {
    UATSlideShow = 0x2,
    UATTPEG = 0x4,
    UATSPI = 0x7,
    UATDMB = 0x9,
    UATFilecasting = 0xD,
    UATFIS = 0xE,
    UATJournaline = 0x44A,
    UATUndef = (int16_t) -1
};


// table 17
enum MOTContentBaseType {
    MOTBaseTypeGeneralData = 0x00,
    MOTBaseTypeText  = 0x01,
    MOTBaseTypeImage = 0x02,
    MOTBaseTypeAudio = 0x03,
    MOTBaseTypeVideo = 0x04,
    MOTBaseTypeTransport = 0x05,
    MOTBaseTypeSystem = 0x06,
    MOTBaseTypeApplication = 0x07,
    MOTBaseTypeProprietary = 0x3f
};

enum MOTContentType {

    // Masks
    MOTCTBaseTypeMask = 0x3f00,
    MOTCTSubTypeMask = 0x00ff,

    // General Data: 0x00xx
    MOTCTGeneralDataObjectTransfer = 0x0000,
    MOTCTGeneralDataMIMEHTTP = 0x0001,

    // Text formats: 0x01xx
    MOTCTTextASCII = 0x0100,
    MOTCTTextLatin1 = 0x0101,
    MOTCTTextHTML = 0x0102,
    MOTCTTextPDF = 0x0103,

    // Image formats: 0x02xx
    MOTCTImageGIF = 0x0200,
    MOTCTImageJFIF = 0x0201,
    MOTCTImageBMP = 0x0202,
    MOTCTImagePNG = 0x0203,
    MOTCTAudioMPEG1Layer1 = 0x0300,
    MOTCTAudioMPEG1Layer2 = 0x0301,
    MOTCTAudioMPEG1Layer3 = 0x0302,
    MOTCTAudioMPEG2Layer1 = 0x0303,
    MOTCTAudioMPEG2Layer2 = 0x0304,
    MOTCTAudioMPEG2Layer3 = 0x0305,
    MOTCTAudioPCM = 0x0306,
    MOTCTAudioAIFF = 0x0307,
    MOTCTAudioATRAC = 0x0308,
    MOTCTAudioUndefined = 0x0309,
    MOTCTAudioMPEG4 = 0x030a,

    // Video formats: 0x04xx
    MOTCTVideoMPEG1 = 0x0400,
    MOTCTVideoMPEG2 = 0x0401,
    MOTCTVideoMPEG4 = 0x0402,
    MOTCTVideoH263 = 0x0403,

    // MOT transport: 0x05xx
    MOTCTTransportHeaderUpdate = 0x0500,
    MOTCTTransportHeaderOnly = 0x0501,

    // System: 0x06xx
    MOTCTSystemMHEG = 0x0600,
    MOTCTSystemJava = 0x0601,

    // Application Specific: 0x07xx
    MOTCTApplication = 0x0700,

    // Proprietary: 0x3fxx
    MOTCTProprietary = 0x3f00
};

// Return the base type from the MOTContentType
inline MOTContentBaseType getContentBaseType (MOTContentType ct) {
    return static_cast<MOTContentBaseType>((ct & MOTCTBaseTypeMask) >> 8);
}

// Return the sub type from the MOTContentType
inline uint8_t getContentSubType(MOTContentType ct) {
    return static_cast<uint8_t>((ct & MOTCTSubTypeMask));
}

const char* getASCTy(int16_t ASCTy);
const char* getDSCTy(int16_t DSCTy);
const char* getLanguage(int16_t language);
const char* getCountry(uint8_t ecc, uint8_t countryId);
const char* getProgramType_Not_NorthAmerica(int16_t programType);
const char* getProgramType_For_NorthAmerica(int16_t programType);
const char* getProgramType(bool, uint8_t interTabId, int16_t programType);
const char* getUserApplicationType(int16_t appType);
const char* getFECscheme(int16_t FEC_scheme);
const char* getProtectionLevel(bool shortForm, int16_t protLevel);
const char* getCodeRate(bool shortForm, int16_t protLevel);
#endif

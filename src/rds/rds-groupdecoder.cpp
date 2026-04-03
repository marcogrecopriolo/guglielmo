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
 *    Taken from sdr-j-fm, with bug fixes and enhancements.
 *
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This part of the FM demodulation software is largely
 *    a rewrite and local adaptation of FMSTACK software
 *    Technical University of Munich, Institute for Integrated Systems (LIS)
 *    FMSTACK Copyright (C) 2010 Michael Feilen
 * 
 *    Author(s)       : Michael Feilen (michael.feilen@tum.de)
 *    Initial release : 01.09.2009
 *    Last changed    : 09.03.2010
 */

#include "rds-groupdecoder.h"
#include "codetables.h"
#include "logging.h"
#include "radio.h"
#include <cstring>
#include <stdio.h>

// The bitmaps
#define ALL_NAME_SEGMENTS ((uint32_t)(1 << NUMBER_OF_NAME_SEGMENTS) - 1)
#define ALL_TEXT_SEGMENTS ((uint32_t)(1 << NUMBER_OF_TEXT_SEGMENTS) - 1)

rdsGroupDecoder::rdsGroupDecoder(RadioInterface* radioInterface, bool p) {
    this->radioInterface = radioInterface;
    connect(this, SIGNAL(setStationLabel(const QString&)),
	radioInterface, SLOT(showLabel(const QString&)));
    connect(this, SIGNAL(setRadioText(const QString&)),
	radioInterface, SLOT(showText(const QString&)));
    partialText = p;
    reset();
}

rdsGroupDecoder::~rdsGroupDecoder(void) {
    disconnect(this, SIGNAL(setStationLabel(const QString&)),
	radioInterface, SLOT(showLabel(const QString&)));
    disconnect(this, SIGNAL(setRadioText(const QString&)),
	radioInterface, SLOT(showText(const QString&)));
}

void rdsGroupDecoder::setPartialText(bool p) {
    partialText = p;
}

void rdsGroupDecoder::reset(void) {
    piCode = 0;
    alphabet = tabG0;

    // Initialize Group 1 members
    memset(stationLabel, ' ', STATION_LABEL_LENGTH);
    grp1DiCode = 0;
    stationNameSegmentRegister = 0;

    // Initialize Group 2 members
    memset(textBuffer, ' ', RADIOTEXT_LENGTH);
    textBuffer[RADIOTEXT_LENGTH] = '\0';
    textABflag = -1; // Not defined
    textSegmentRegister = 0;
    initialLen = 0;
    setRadioText("");
    setStationLabel("");
}

bool rdsGroupDecoder::decode(RDSGroup* grp) {
    log(LOG_RDS, LOG_VERBOSE, "Got group %d", grp->getGroupType());

    // PI-code has changed -> new station received
    // Reset the decoder
    if (grp->getPiCode() != piCode) {
	reset();
	piCode = grp->getPiCode();
    }

    // Cannot decode B type groups
    if (grp->isTypeBGroup())
	return false;

    // Decide by group type code
    switch (grp->getGroupType()) {
    case RDSGroup::BASIC_TUNING_AND_SWITCHING:
	handleBasicTuning(grp);
	break;

    case RDSGroup::RADIO_TEXT:
	handleRadioText(grp);
	break;

    case RDSGroup::CLOCKTIME_AND_DATE:
	handleTimeAndDate(grp);
	break;

    // Group 8: Open data application. Not implemented yet
    case RDSGroup::TMC_DATA: {
	const uint16_t blk_B = grp->getBlock(RDSGroup::BLOCK_B);
	const uint16_t blk_C = grp->getBlock(RDSGroup::BLOCK_C);
	const uint32_t location = grp->getBlock(RDSGroup::BLOCK_D);
	const uint32_t event = blk_C & 0x3FF;
	// const uint32_t extend = (blk_C >> 11) & 0x7;
	// const uint32_t direction = (blk_C >> 14) & 0x1;
	// const uint32_t diversionAdvice = (blk_C >> 15) & 0x1;
	// const uint32_t duration = (blk_B & 0x7);
	const uint32_t singleGroupMsg = (blk_B >> 3) & 0x1;
	const uint32_t tuningInfo = (blk_B >> 4) & 0x1;

	if (singleGroupMsg == 1 && tuningInfo == 0) {
	    if (location > 51321 || event > 10000) {
		// ERROR!
	    } else {
		// m_pSink -> addTMCLocationAndEvent (location, event);
	    }
	}
	break;
    }
    default:; // just ignore for now
    }

    return true;
}

void rdsGroupDecoder::handleBasicTuning(RDSGroup* grp) {
    uint32_t segIndex = grp->getBlockB() & 0x3;
    uint32_t charsforStationName = grp->getBlockD() & 0xFFFF;

    addToStationLabel(segIndex, charsforStationName, grp->getProgrammeType());

    // Fill DI code
    grp1DiCode |= ((grp->getBlockB() >> 2) & 1) << segIndex;
}

void rdsGroupDecoder::addToStationLabel(uint32_t index,
    uint32_t name, int16_t pty) {
    uint32_t i = index * 2;

    stationLabel[i++] = (name >> 8);
    stationLabel[i] = (name & 0xFF);
    stationNameSegmentRegister |= 1 << index;
    log(LOG_RDS, LOG_VERBOSE, "station label %i (%.2s) %i %s", index,
	(char *) &stationLabel[i-1], stationNameSegmentRegister,
	qPrintable(prepareText(stationLabel, STATION_LABEL_LENGTH)));
    if (stationNameSegmentRegister == ALL_NAME_SEGMENTS) {
	setStationLabel(prepareText(stationLabel, STATION_LABEL_LENGTH));
	stationNameSegmentRegister = 0;
	(void) pty;
	memset(stationLabel, ' ', STATION_LABEL_LENGTH);
    }
}

void rdsGroupDecoder::handleRadioText(RDSGroup* grp) {
    const uint16_t new_txtABflag = (grp->getBlockB() >> 4) & 1;
    const uint32_t currentSegment = (grp->getBlockB()) & 0xF;
    int start = CHARS_PER_RTXT_SEGMENT * currentSegment;
    uint32_t segmentRegister;
    uint16_t textPart1, textPart2;
    uint16_t i;

    if (textABflag != new_txtABflag) {
	textABflag = new_txtABflag;

	// Reset the segment buffer
	// The text will be redisplayed when the new one arrives
	textSegmentRegister = 0;
	initialLen = 0;
	memset(textBuffer, ' ', RADIOTEXT_LENGTH);
    }

    textPart1 = grp->getBlockC();
    textPart2 = grp->getBlockD();

    // Store the received data
    i = start;
    textBuffer[i++] = (char)(textPart1 >> 8);
    textBuffer[i++] = (char)(textPart1 & 0xFF);
    textBuffer[i++] = (char)(textPart2 >> 8);
    textBuffer[i++] = (char)(textPart2 & 0xFF);

    // current segment is received (set bit in segment register to 1)
    segmentRegister = 1 << currentSegment;
    textSegmentRegister |= segmentRegister;
    if (initialLen == start)
	while (textSegmentRegister & segmentRegister) {
	    initialLen += CHARS_PER_RTXT_SEGMENT;
	    segmentRegister = segmentRegister << 1;
	}

    // check for end of message
    bool end = false;
    for (i = start; i < start + CHARS_PER_RTXT_SEGMENT; i++)
	if (textBuffer[i] == END_OF_RADIO_TEXT && initialLen > i) {
	    end = true;
	    break;
	}

    log(LOG_RDS, LOG_VERBOSE, "radio text (%.4s) %i %i %x %s", (char *) &textBuffer[start], end, initialLen,
		    textSegmentRegister, (char *) &textBuffer);

    if ((partialText && initialLen > 0) || end || textSegmentRegister == ALL_TEXT_SEGMENTS)
	setRadioText(prepareText(textBuffer, initialLen));
    if (end || textSegmentRegister == ALL_TEXT_SEGMENTS) {
	textSegmentRegister = 0;
	initialLen = 0;
	memset(textBuffer, ' ', RADIOTEXT_LENGTH);
    }
}

void rdsGroupDecoder::handleTimeAndDate(RDSGroup* grp) {
    // uint16_t Hours	= (grp -> getBlockD () >> 12) & 0xF;
    // uint16_t Minutes	= (grp -> getBlockD () >> 6) & 0x3F;
    // uint16_t Days	= grp -> getBlockC ();
    // uint16_t offset	= (grp -> getBlockD ()) & 0x4F;

    (void)grp;
}

// handle the text, taking into account different alphabets
QString rdsGroupDecoder::prepareText(char* v, int16_t length) {
    int16_t i;
    uint8_t previousChar = v[0];
    QString outString = QString("");

    for (i = 1; i < length; i++) {
	uint8_t currentChar = v[i];
	if (alphabetSwitcher(previousChar, currentChar)) {
	    alphabet = setAlphabetTo(previousChar, currentChar);
	    previousChar = v[i];
	    i++;
	} else {
	    outString.append(alphabet[previousChar]);
	    previousChar = currentChar;
	}
    }
    outString.append(alphabet[previousChar]);
    return outString;
}

bool rdsGroupDecoder::alphabetSwitcher(uint8_t c1, uint8_t c2) {
    if ((c1 == 0x0F) && (c2 == 0x0F))
	return true;
    if ((c1 == 0x0E) && (c2 == 0x0E))
	return true;
    if ((c1 == 0x1B) && (c2 == 0x6E))
	return true;
    return false;
}

// Note that if we ensure that the function is only
// called with alphabetSwitcher == true, we only have to
// look at the first character
uint16_t* rdsGroupDecoder::setAlphabetTo(uint8_t c1, uint8_t c2) {
    log(LOG_RDS, LOG_MIN, "setting alphabet to %x %x", c1, c2);
    switch (c1) {
    default: // should not happen
    case 0x0F:
	return tabG0;
    case 0x0E:
	return tabG1;
    case 0x1B:
	return tabG2;
    }
}

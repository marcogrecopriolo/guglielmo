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

#include	"rds-groupdecoder.h"
#include	<cstring>
#include	<stdio.h>
#include	"radio.h"


//	The alfabets
#define	G0	0
#define	G1	1
#define	G2	2

//	The bitmaps
#define	ALL_NAME_SEGMENTS	((uint32_t) (1 << NUMBER_OF_NAME_SEGMENTS) - 1)
#define	ALL_TEXT_SEGMENTS	((uint32_t) (1 << NUMBER_OF_TEXT_SEGMENTS) - 1)

#include	"ebu-codetables.h"

	rdsGroupDecoder::rdsGroupDecoder (RadioInterface *RI) {
	MyRadioInterface	= RI;
	connect (this, SIGNAL (setStationLabel (const QString &)),
	         MyRadioInterface, SLOT (showLabel (const QString &)));
	connect (this, SIGNAL (setRadioText (const QString &)),
	         MyRadioInterface, SLOT (showText (const QString &)));
	reset ();
}

	rdsGroupDecoder::~rdsGroupDecoder(void) {
}

void	rdsGroupDecoder::reset (void) {
	m_piCode = 0;

// Initialize Group 1 members
	memset (stationLabel, ' ', STATION_LABEL_LENGTH);
	m_grp1_diCode		= 0;
	stationNameSegmentRegister	= 0;

// Initialize Group 2 members
	memset (textBuffer, ' ', RADIOTEXT_LENGTH);
	textABflag		= -1; // Not defined
	textSegmentRegister	= 0;
	setRadioText		("");
	setStationLabel		("");
	clearMusicSpeechFlag	();
	setPTYCode		(0);
	setPiCode		(0);
	setAFDisplay		(0);
}

bool rdsGroupDecoder::decode (RDSGroup *grp) {
//	fprintf (stderr, "Got group %d\n", grp -> getGroupType ());
	setGroup	(grp -> getGroupType ());
	setPTYCode	(grp -> getProgrammeType ());
	setPiCode	(grp -> getPiCode ());

//	PI-code has changed -> new station received
//	Reset the decoder
	if (grp -> getPiCode() != m_piCode) {
	   reset ();
	   m_piCode = grp -> getPiCode();
	}

//	Cannot decode B type groups
	if (grp -> isTypeBGroup()) return false;

//	Decide by group type code
	switch (grp -> getGroupType()) {
	   case RDSGroup::BASIC_TUNING_AND_SWITCHING:
	      Handle_Basic_Tuning_and_Switching (grp);
	      break;
//
	   case RDSGroup::RADIO_TEXT:
	      Handle_RadioText (grp);
	      break;

	   case RDSGroup::CLOCKTIME_AND_DATE:
	      Handle_Time_and_Date (grp);
	      break;

//
// Group 8: Open data application. Not implemented yet
//
	   case RDSGroup::TMC_DATA:
	      {  const uint16_t blk_B = grp -> getBlock (RDSGroup::BLOCK_B);
	         const uint16_t blk_C = grp -> getBlock (RDSGroup::BLOCK_C);
	         const uint32_t location = grp -> getBlock(RDSGroup::BLOCK_D);
	         const uint32_t event = blk_C & 0x3FF;
//	         const uint32_t extend = (blk_C >> 11) & 0x7;
//	         const uint32_t direction = (blk_C >> 14) & 0x1;
//	         const uint32_t diversionAdvice = (blk_C >> 15) & 0x1;
//	         const uint32_t duration = (blk_B & 0x7);
	         const uint32_t singleGroupMsg = (blk_B >> 3) & 0x1;
	         const uint32_t tuningInfo = (blk_B >> 4) & 0x1;

	         if (singleGroupMsg == 1 && tuningInfo == 0) {
	            if (location > 51321 || event > 10000) {
		// ERROR!
	            }
	            else {
//	               m_pSink -> addTMCLocationAndEvent (location, event);
	               ;
	            }
	         }
	         break;
	      }
	   default:
	      ;		// just ignore for now
	}

	return true;
}

void	rdsGroupDecoder::Handle_Basic_Tuning_and_Switching (RDSGroup *grp) {
uint32_t segIndex		= grp -> getBlock_B () & 0x3;
uint32_t charsforStationName	= grp -> getBlock_D () & 0xFFFF;

	addtoStationLabel (segIndex, charsforStationName);
	additionalFrequencies (grp -> getBlock_C ());

//	Set Music/Speech flag
	setMusicSpeechFlag ((grp -> getBlock_B () >> 3) & 1);

//	Fill DI code
	m_grp1_diCode |= ((grp -> getBlock_B () >> 2) & 1) << segIndex;
}

void	rdsGroupDecoder::addtoStationLabel (uint32_t index,
	                                    uint32_t name) {
	uint32_t i = index * 2;

	stationLabel [i++] = (name >> 8);
	stationLabel [i] =  (name & 0xFF);
	stationNameSegmentRegister |= 1 << index;
	if (stationNameSegmentRegister == ALL_NAME_SEGMENTS)
	   setStationLabel (prepareText(stationLabel, STATION_LABEL_LENGTH));
}

void	rdsGroupDecoder::additionalFrequencies (uint16_t blockContents) {
uint8_t af1 = blockContents >> 8;
uint8_t af2 = blockContents & 0xFF;

	if ((af1 > 1) && (af1 < 205)) {
	   setAFDisplay (af1 * 100 + 87500);
	}

//	Check for range and add only VHF frequencies
	if ((af1 != 250) && (af2 > 1) && (af1 < 205))  {
	   setAFDisplay (af2 * 100 + 87500);
	}
}

void	rdsGroupDecoder::Handle_RadioText (RDSGroup *grp) {
const uint16_t	new_txtABflag = (grp -> getBlock_B () >> 4) & 1;
const uint16_t	currentSegment = (grp -> getBlock_B ()) & 0xF;
int		start = 4 * currentSegment;
uint16_t	textPart1, textPart2;
uint16_t	i;

	if (textABflag != new_txtABflag) {
	   textABflag = new_txtABflag;

//	Reset the segment buffer
//	The text will be redisplayed when the new one arrives
	   textSegmentRegister = 0;
	   memset (textBuffer, ' ', RADIOTEXT_LENGTH);
	}

	textPart1	= grp -> getBlock_C ();
	textPart2	= grp -> getBlock_D ();

	// Store the received data
	i = start;
	textBuffer [i++] = (char)(textPart1 >> 8);
	textBuffer [i++] = (char)(textPart1 & 0xFF);
	textBuffer [i++] = (char)(textPart2 >> 8);
	textBuffer [i++] = (char)(textPart2 & 0xFF);

//	current segment is received (set bit in segment register to 1)
	textSegmentRegister |= 1 << currentSegment;

//	check for end of message
	for (i = start; i < start + 4; i ++)

	   // dump text if we've seen beginning and end
	   if (textBuffer [i] == END_OF_RADIO_TEXT && (textSegmentRegister & 1)) {
	      setRadioText (prepareText (textBuffer, i));
	      textSegmentRegister = 0;
	      memset (textBuffer, ' ', RADIOTEXT_LENGTH);
	      return;
	   }

// Check if all fragments are in or we had an end of message
	if (textSegmentRegister == ALL_TEXT_SEGMENTS) {
	   setRadioText (prepareText (textBuffer, RADIOTEXT_LENGTH));
	   textSegmentRegister = 0;
	   memset (textBuffer, ' ', RADIOTEXT_LENGTH);
	}
}

void	rdsGroupDecoder::Handle_Time_and_Date (RDSGroup *grp) {
// uint16_t Hours	= (grp -> getBlock_D () >> 12) & 0xF;
// uint16_t Minutes	= (grp -> getBlock_D () >> 6) & 0x3F;
// uint16_t Days	= grp -> getBlock_C ();
// uint16_t offset	= (grp -> getBlock_D ()) & 0x4F;

	(void) grp;
}
//
//	handle the text, taking into account different alfabets
QString	rdsGroupDecoder::prepareText (char *v, int16_t length) {
int16_t	i;
uint8_t	previousChar	= v [0];
QString outString	= QString ("");

	for (i = 1; i < length; i ++) {
	   uint8_t currentChar = v [i];
	   if (alfabetSwitcher (previousChar, currentChar)) {
	      theAlfabet = setAlfabetTo (previousChar, currentChar);
	      previousChar = v [i];
	      i++;
	   }
	   else {
	      outString. append (mapEBUtoUnicode (theAlfabet, previousChar));
	      previousChar = currentChar;
	   }
	}
	outString. append (mapEBUtoUnicode (theAlfabet, previousChar));
	return outString;
}

bool	rdsGroupDecoder::alfabetSwitcher (uint8_t c1, uint8_t c2) {
	if ((c1 == 0x0F) && (c2 == 0x0F))	return true;
	if ((c1 == 0x0E) && (c2 == 0x0E))	return true;
	if ((c1 == 0x1B) && (c2 == 0x6E))	return true;
	return false;
}
//
//	Note that iff we ensure that the function is only
//	called with alfabetSwitcher == true, we only have to
//	look at the first character
uint8_t	rdsGroupDecoder::setAlfabetTo (uint8_t c1, uint8_t c2) {
	fprintf (stderr, "setting alfabet to %x %x\n", c1, c2);
	switch (c1) {
	   default:		// should not happen
	   case 0x0F:
	      return G0;
	   case 0x0E:
	      return G1;
	   case 0x1B:
	      return G2;
	}
}

uint8_t	rdsGroupDecoder::applyAlfabet (uint8_t alfabet, uint8_t c) {
	if (alfabet == G0)
	   return c;
	else
	   return c;
}

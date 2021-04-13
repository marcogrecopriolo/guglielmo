/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopiolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

// Groups

// there's a group per device, named like the device

#define	GROUP_UI	"ui"
#define	GROUP_SOUND	"sound"
#define	GROUP_DAB	"dab"
#define	GROUP_FM	"fm"
#define	GROUP_PRESETS	"presets"	// this is an array, but still

// keys

// general
#define GEN_VOLUME		"volume"
#define GEN_SQUELCH		"squelch"
#define GEN_TUNER_MODE		"tunerMode"
#define GEN_CHANNEL		"channel"
#define GEN_SERVICE_NAME	"serviceName"
#define GEN_FM_FREQUENCY	"FMfrequency"

#define GEN_DEF_VOLUME		"0"
#define GEN_DEF_SQUELCH		"0"
#define GEN_DEF_TUNER_MODE	"FM"
#define GEN_DEF_CHANNEL		"12C"

#define GEN_FM			"FM"
#define GEN_DAB			"DAB"

// ui
#define UI_THEME		"theme"
#define UI_DEF_THEME		""

// dab
#define DAB_THRESHOLD		"threshold"
#define DAB_DIFF_LENGTH		"diff_length"
#define DAB_TII_DELAY		"tii_delay"
#define DAB_TII_DEPTH		"tii_depth"
#define DAB_ECHO_DEPTH		"echo_depth"
#define DAB_SERVICE_ORDER	"service_order"

#define DAB_DEF_THRESHOLD	3
// #define DAB_DEF_DIFF_LENGTH	
#define DAB_MIN_TII_DELAY	5
#define DAB_DEF_TII_DELAY	DAB_MIN_TII_DELAY
#define DAB_DEF_TII_DEPTH	1
#define DAB_DEF_ECHO_DEPTH	1
#define DAB_DEF_SERVICE_ORDER	0

// fm
#define FM_BUFFERS_SIZE		"buffersSize"
#define FM_WORKING_RATE		"workingRate"
#define FM_AUDIO_RATE		"audioRate"
#define FM_AVERAGE_COUNT	"averageCount"
#define FM_REPEAT_RATE		"repeatRate"
#define FM_FILTER_DEPTH		"filterDepth"
#define FM_THRESHOLD		"threshold"

#define FM_FILTER		"filter"
#define FM_DEGREE		"degree"
#define FM_DECODER		"decoder"
#define FM_DEEMPHASIS		"deemphasis"
#define FM_LOW_PASS_FILTER	"threshold"

#define FM_DEF_BUFFERS_SIZE	"512"
#define FM_DEF_WORKING_RATE	"48000"
#define FM_DEF_AUDIO_RATE	"48000"
#define FM_DEF_AVERAGE_COUNT	"5"
#define FM_DEF_REPEAT_RATE	"5"
#define FM_DEF_FILTER_DEPTH	"5"
#define FM_DEF_THRESHOLD	"20"

#define FM_DEF_FILTER		"0"
#define FM_DEF_DEGREE		"15"
#define FM_DEF_DECODER		"3"
#define FM_DEF_DEEMPHASIS	"50"
#define FM_DEF_LOW_PASS_FILTER	"-1"

// sound
#define SOUND_MODE		"soundMode"
#define SOUND_LATENCY		"latency"
#define SOUND_CHANNEL		"soundChannel"

#define SOUND_DEF_MODE		"Qt"
#define SOUND_DEF_LATENCY	"5"
#define SOUND_DEF_CHANNEL	"0"

#define SOUND_QT		"Qt"
#define SOUND_PORTAUDIO		"Portaudio"

// presets
#define PRESETS_NAME		"preset"

// devices
#define DEV_AGC			"agc"
#define DEV_IF_GAIN		"ifGain"
#define DEV_LNA_GAIN		"lnaGain"

#define DEV_DEF_AGC		"0"
#define DEV_DEF_IF_GAIN		"50"
#define DEV_DEF_LNA_GAIN	"50"
#endif		// __SETTINGS_H__

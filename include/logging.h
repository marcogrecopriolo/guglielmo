/*
 *    Copyright (C) 2022
 *    Marco Greco <marcogrecopriolo@gmail.com>
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

#ifndef __LOGGING_H__
#define __LOGGING_H__

#define LOG_MIN		0
#define LOG_CHATTY	1
#define LOG_VERBOSE	2

#define LOG_UI		0x0001
#define LOG_EVENT	0x0002
#define LOG_MPRIS	0x0004
#define LOG_DAB		0x0008
#define LOG_FM		0x0010
#define LOG_RDS		0x0020
#define LOG_DEV		0x0040
#define LOG_AUDIO	0x0080
#define LOG_DATA	0x0100
#define LOG_VITDEC	0x0200
#define LOG_SOUND	0x0400
#define LOG_EPG		0x0800
#define LOG_AGC		0x1000

extern qint64 logMask;
extern int logVerbosity;

#ifdef __GNUC__
#define log(comp, level, fmt, ...) if ((logVerbosity >= (level)) && (logMask & (comp)) != 0) fprintf(stderr, "%-7s " fmt "\n", (#comp ":")+4, ##__VA_ARGS__)
#else
#define log(comp, level, fmt, ...) if ((logVerbosity >= (level)) && (logMask & (comp)) != 0) fprintf(stderr, "%-7s " fmt "\n", (#comp ":")+4, __VA_ARGS__)
#endif

#endif	// __LOGGING_H__

#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
//	Common definitions and includes

#ifndef	__CONSTANTS__
#define	__CONSTANTS__
#include	<QString>
#include	<stdint.h>
#include	<limits>
#include	<stdlib.h>
#include	<unistd.h>
#include	<cmath>
#include	<cstdint>
#include	<cstdlib>
#include	<cstdio>
#include	<complex>
#include	<cstring>

// the mingw compiler complains that nested use of the defined() macro might not be portable
// hence we define IS_WINDOWS explicitly
// currently we only cover mingw32
#if (defined(__MINGW32__))
#define IS_WINDOWS 1
#endif

#ifndef	__FREEBSD__
//#include	<malloc.h>
#endif

#if IS_WINDOWS
//#include	"iostream.h"
#include	"windows.h"
#else
#ifndef	__FREEBSD__
//#include	"alloca.h"
#endif
#include	"dlfcn.h"
typedef	void	*HINSTANCE;
#endif


// DAB

#define		DAB		0100
#define		DAB_PLUS	0101

#define		AUDIO_SERVICE	0101
#define		PACKET_SERVICE	0102
#define		UNKNOWN_SERVICE	0100

#define		INPUT_RATE	2048000
#define		BANDWIDTH	1536000


typedef struct {
        int     theTime;
        QString theText;
} epgElement;

class serviceId {
public:
	QString		name;
	uint32_t	SId;
	uint16_t	subChId;
};

//	order by id order by name
#define ID_BASED        1
#define ALPHA_BASED     0
#define	SUBCH_BASED	2

#define		BAND_III	0100
#define		L_BAND		0101
#define		A_BAND		0102

#define	__BOTH		0
#define	__ONLY_SOUND	1
#define	__ONLY_DATA	2

class	descriptorType {
public:
	uint8_t	type;
	bool	defined;
	uint8_t	procMode;
	QString	serviceName;
	int32_t	SId;
	int	SCIds;
	int16_t subchId;
	int16_t	startAddr;
	bool	shortForm;
	int16_t	protLevel;
	int16_t	length;
	int16_t	bitRate;
	QString	channel;	// just for presets
public:
		descriptorType() {
	defined		= false;
	serviceName	= "";
	procMode	= __BOTH;
	}
virtual		~descriptorType() {}
};

//	for service handling we define
class packetdata: public descriptorType {
public:
	int16_t DSCTy;
	int16_t	FEC_scheme;
	int16_t	DGflag;
	int16_t	appType;
	int16_t	compnr;
	int16_t	packetAddress;
	packetdata() {
	   type	= PACKET_SERVICE;
	}
};

class audiodata: public descriptorType {
public:
	int16_t	ASCTy;
	int16_t	language;
	int16_t	programType;
	int16_t	compnr;
	int32_t	fmFrequency;
	audiodata() {
	   type	= AUDIO_SERVICE;
	}
};

static inline
bool	check_CRC_bits (uint8_t *in, int32_t size) {
static
const uint8_t crcPolynome [] =
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};	// MSB .. LSB
int32_t	i, f;
uint8_t	b [16];
int16_t	Sum	= 0;

	memset (b, 1, 16);

	for (i = size - 16; i < size; i ++)
	   in [i] ^= 1;

	for (i = 0; i < size; i++) {
	   if ((b [0] ^ in [i]) == 1) {
	      for (f = 0; f < 15; f++) 
	         b [f] = crcPolynome [f] ^ b[f + 1];
	      b [15] = 1;
	   }
	   else {
	      memmove (&b [0], &b[1], sizeof (uint8_t ) * 15); // Shift
	      b [15] = 0;
	   }
	}

	for (i = 0; i < 16; i++)
	   Sum += b [i];

	return Sum == 0;
}

static inline
bool	check_crc_bytes (uint8_t *msg, int32_t len) {
int i, j;
uint16_t	accumulator	= 0xFFFF;
uint16_t	crc;
uint16_t	genpoly		= 0x1021;

	for (i = 0; i < len; i ++) {
	   int16_t data = msg [i] << 8;
	   for (j = 8; j > 0; j--) {
	      if ((data ^ accumulator) & 0x8000)
	         accumulator = ((accumulator << 1) ^ genpoly) & 0xFFFF;
	      else
	         accumulator = (accumulator << 1) & 0xFFFF;
	      data = (data << 1) & 0xFFFF;
	   }
	}
//
//	ok, now check with the crc that is contained
//	in the au
	crc	= ~((msg [len] << 8) | msg [len + 1]) & 0xFFFF;
	return (crc ^ accumulator) == 0;
}


// FM

#define	QT_STATIC_CONST

#define	MIN_FM		87.0
#define	MAX_FM		108.9
#define	DEF_FM		88.0

typedef	float	DSPFLOAT;
typedef	std::complex<DSPFLOAT>	DSPCOMPLEX;

using namespace std;

#define	MINIMUM(x, y)	((x) < (y) ? x : y)
#define	MAXIMUM(x, y)	((x) > (y) ? x : y)

#define	PILOT_FILTER_SIZE	31
#define	RDS_LOWPASS_SIZE	89
#define	HILBERT_SIZE		13
#define AUDIO_FILTER_SIZE	11
#define	RDS_BAND_FILTER_SIZE	49
#define	FFT_SIZE		256
#define	LEVEL_SIZE		512
#define	LEVEL_FREQ		3

// Common

#ifndef	M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif
using namespace std;
//
#define	Hz(x)		(x)
#define	Khz(x)		(x * 1000)
#define	KHz(x)		(x * 1000)
#define	kHz(x)		(x * 1000)
#define	Mhz(x)		(Khz (x) * 1000)
#define	MHz(x)		(KHz (x) * 1000)
#define	mHz(x)		(kHz (x) * 1000)

//
//	40 up shows good results
#define		DIFF_LENGTH	60
static inline
bool	isIndeterminate (float x) {
	return x != x;
}

static inline
bool	isInfinite (float x) {
	return x == numeric_limits<float>::infinity();
}

static inline
std::complex<float> cmul (std::complex<float> x, float y) {
	return std::complex<float> (real (x) * y, imag (x) * y);
}

static inline
std::complex<float> cdiv (std::complex<float> x, float y) {
	return std::complex<float> (real (x) / y, imag (x) / y);
}

static inline
float	get_db (float x) {
	return 20 * log10 ((x + 0.005) / (float)(256));
}

static inline
float   get_db (DSPFLOAT x, int32_t y) {
        return 20 * log10 ((x + 1) / (float)(y));
}

#define	MINIMUM(x, y)	((x) < (y) ? x : y)
#define	MAXIMUM(x, y)	((x) > (y) ? x : y)

static inline
float	jan_abs (std::complex<float> z) {
float	re	= real (z);
float	im	= imag (z);
	return (re < 0 ? -re : re) +
	       (im < 0 ? -im : im);
}

//	just some locals
//
//	generic, up to 16 bits
static inline
uint16_t	getBits (uint8_t *d, int32_t offset, int16_t size) {
int16_t	i;
uint16_t	res	= 0;

	for (i = 0; i < size; i ++) {
	   res <<= 1;
	   res |= (d [offset + i]) & 01;
	}
	return res;
}

static inline
uint16_t	getBits_1 (uint8_t *d, int32_t offset) {
	return (d [offset] & 0x01);
}

static inline
uint16_t	getBits_2 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	return res;
}

static inline
uint16_t	getBits_3 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	return res;
}

static inline
uint16_t	getBits_4 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	return res;
}

static inline
uint16_t	getBits_5 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	return res;
}

static inline
uint16_t	getBits_6 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	return res;
}

static inline
uint16_t	getBits_7 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	res	<<= 1;
	res	|= d [offset + 6];
	return res;
}

static inline
uint16_t	getBits_8 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	res	<<= 1;
	res	|= d [offset + 6];
	res	<<= 1;
	res	|= d [offset + 7];
	return res;
}


static inline
uint32_t	getLBits	(uint8_t *d,
	                         int32_t offset, int16_t amount) {
uint32_t	res	= 0;
int16_t		i;

	for (i = 0; i < amount; i ++) {
	   res <<= 1;
	   res |= (d [offset + i] & 01);
	}
	return res;
}

#endif

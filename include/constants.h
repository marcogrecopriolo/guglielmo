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

// Common definitions and includes

#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QString>
#include <complex>

// the mingw compiler complains that nested use of the defined() macro might not be portable
// hence we define IS_WINDOWS explicitly
#if (defined(__MINGW32__) || defined(_MSC_VER))
#define IS_WINDOWS 1
#endif

#ifndef _MSC_VER
#include <unistd.h>
#define _ALWAYS_INLINE __attribute__((always_inline))
#define _ALIGN(S, F) F __attribute__((aligned(S)))
#define _VLA(T, N, S) T N[(S)]
#else
#include "getopt.h"
#define _ALWAYS_INLINE __attribute__ inline __forceinline
#define _ALIGN(S, F) __declspec(align(S)) F

// MSVC does not support VLAs
#define _VLA(T, N, S) T* N = (T*)alloca((S) * sizeof(T))

// MSVC has conflicting definitions for std::min and std::max
#define NOMINMAX
#endif

#include <QThread>
#define usleep(NS) QThread::usleep(NS)

#if IS_WINDOWS
// #include "iostream.h"
#include "windows.h"
#define KEY_MODIFIER Qt::ALT
#else
#define KEY_MODIFIER Qt::CTRL
#if !defined(__FREEBSD__) && !defined(__APPLE_CC__)
#include <malloc.h>
#endif
#include "dlfcn.h"
typedef void* HINSTANCE;
#ifdef __APPLE_CC__
#define LIBEXT ".dylib"
#else
#define LIBEXT ".so"
#endif
#endif

// Volume, AGC, gains, etc

#define AUDIO_SCALE 100
#define GAIN_SCALE 100

// DAB

#define AUDIO_SERVICE 0101
#define PACKET_SERVICE 0102
#define UNKNOWN_SERVICE 0100

#define INPUT_RATE 2048000

// 40 and above shows good results
#define DIFF_LENGTH 60

typedef struct {
    int theTime;
    QString theText;
} epgElement;

class serviceId {
public:
    QString name;
    uint32_t SId;
    uint16_t subChId;
};

// order by id  / name
#define ID_BASED 1
#define ALPHA_BASED 0
#define SUBCH_BASED 2

#define BAND_III 0100
#define L_BAND 0101
#define A_BAND 0102

#define __BOTH 0
#define __ONLY_SOUND 1
#define __ONLY_DATA 2

// FM

#define QT_STATIC_CONST

#define MIN_FM 87.0
#define MAX_FM 108.9
#define DEF_FM 88.0

using namespace std;

typedef float DSPFLOAT;
typedef std::complex<DSPFLOAT> DSPCOMPLEX;


#define PILOT_FILTER_SIZE 31
#define RDS_LOWPASS_SIZE 89
#define HILBERT_SIZE 13
#define AUDIO_FILTER_SIZE 11
#define RDS_BAND_FILTER_SIZE 49
#define FFT_SIZE 256
#define LEVEL_SIZE 512
#define LEVEL_FREQ 3

// Common

#define MIN_AGC_AMPLITUDE 60
#define MAX_AGC_AMPLITUDE 90

#define Hz(x) (x)
#define Khz(x) (x * 1000)
#define KHz(x) (x * 1000)
#define kHz(x) (x * 1000)
#define Mhz(x) (Khz(x) * 1000)
#define MHz(x) (KHz(x) * 1000)
#define mHz(x) (kHz(x) * 1000)

#endif

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

#ifndef MATH_HELPER_H
#define MATH_HELPER_H
#include <cmath>
#include "constants.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

#define MINIMUM(x, y) ((x) < (y) ? x : y)
#define MAXIMUM(x, y) ((x) > (y) ? x : y)

static inline std::complex<float> cmul(std::complex<float> x, float y) {
    return std::complex<float>(real(x) * y, imag(x) * y);
}

static inline std::complex<float> cdiv(std::complex<float> x, float y) {
    return std::complex<float>(real(x) / y, imag(x) / y);
}

static inline float getDb(DSPFLOAT x, int32_t y) {
    return 20 * log10((x + 1) / (float)(y));
}

static inline float fastMagnitude(std::complex<float> z) {
    float re = real(z);
    float im = imag(z);
    return (re < 0 ? -re : re) + (im < 0 ? -im : im);
}
#endif

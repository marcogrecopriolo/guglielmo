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
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */
#ifndef NEW_CONVERTER_H
#define NEW_CONVERTER_H

#include "constants.h"
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <samplerate.h>
#include <vector>

class newConverter {

public:
    newConverter(int32_t inRate, int32_t outRate,
        int32_t inSize);

    ~newConverter();

    bool convert(std::complex<float> v,
        std::complex<float>* out, int32_t* amount);

    int32_t getOutputsize();
    void reset(void);

private:
    int32_t inRate;
    int32_t outRate;
    double ratio;
    int32_t outputLimit;
    int32_t inputLimit;
    SRC_STATE* converter;
    SRC_DATA src_data;
    std::vector<float> inBuffer;
    std::vector<float> outBuffer;
    int32_t inp;
};
#endif

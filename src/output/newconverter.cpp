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
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */

#include "newconverter.h"
#include "logging.h"
#include <cstdio>

newConverter::newConverter(int32_t inRate, int32_t outRate,
    int32_t inSize) {
    int err;
    this->inRate = inRate;
    this->outRate = outRate;
    inputLimit = inSize;
    ratio = double(outRate) / inRate;
    outputLimit = inSize * ratio;

    // converter = src_new (SRC_SINC_BEST_QUALITY, 2, &err);
    // converter = src_new (SRC_SINC_MEDIUM_QUALITY, 2, &err);

    converter = src_new(SRC_LINEAR, 2, &err);
    inBuffer.resize(2 * inputLimit + 20);
    outBuffer.resize(2 * outputLimit + 20);
    src_data.data_in = inBuffer.data();
    src_data.data_out = outBuffer.data();
    src_data.src_ratio = ratio;
    src_data.end_of_input = 0;
    inp = 0;
    log(LOG_SOUND, LOG_CHATTY, "converter created in %i out %i size %i", inRate, outRate, inSize);
}

newConverter::~newConverter() {
    log(LOG_SOUND, LOG_CHATTY, "converter destroyed in %i out %i size %i", inRate, outRate, inputLimit);
    src_delete(converter);
}

bool newConverter::convert(std::complex<float> v,
    std::complex<float>* out, int32_t* amount) {
    int32_t i;
    int32_t framesOut;
    int res;

    inBuffer[2 * inp] = real(v);
    inBuffer[2 * inp + 1] = imag(v);
    inp++;
    if (inp < inputLimit)
        return false;

    src_data.input_frames = inp;
    src_data.output_frames = outputLimit + 10;
    res = src_process(converter, &src_data);
    if (res != 0) {
        log(LOG_SOUND, LOG_MIN, "converter error %s", src_strerror(res));
        return false;
    }
    inp = 0;
    framesOut = src_data.output_frames_gen;
    for (i = 0; i < framesOut; i++)
        out[i] = std::complex<float>(outBuffer[2 * i],
            outBuffer[2 * i + 1]);
    *amount = framesOut;
    return true;
}

int32_t newConverter::getOutputsize() {
    return outputLimit;
}
void newConverter::reset() {
    inp = 0;
}

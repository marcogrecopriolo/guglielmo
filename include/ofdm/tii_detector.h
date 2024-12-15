/*
 *    Copyright (C) 2022
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
 *    Taken from the Qt-DAB program with bug fixes and enhancements.
 *
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef TII_DETECTOR_H
#define TII_DETECTOR_H

#include "dab-params.h"
#include "fft-handler.h"
#include <cstdint>
#include <vector>

class TII_Detector {
  public:
    TII_Detector(uint8_t dabMode, int16_t);
    ~TII_Detector();
    void reset();
    void addBuffer(std::vector<std::complex<float>>);
    uint16_t processNULL();

  private:
    void collapse(std::complex<float> *, float *);
    int16_t depth;
    uint8_t invTable[256];
    dabParams params;
    fftHandler my_fftHandler;
    int16_t T_u;
    int16_t carriers;
    std::complex<float> *fft_buffer;
    std::vector<complex<float>> theBuffer;
    std::vector<float> window;
};
#endif

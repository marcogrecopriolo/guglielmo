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
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef PHASEREFERENCE_H
#define PHASEREFERENCE_H
#include "constants.h"
#include "fft-handler.h"
#include "phasetable.h"
#include "process-params.h"
#include "ringbuffer.h"
#include <QObject>
#include <cstdint>
#include <cstdio>
#include <vector>

class phaseReference : public QObject, public phaseTable {
    Q_OBJECT
  public:
    phaseReference(processParams *, dabParams *);
    ~phaseReference();
    int32_t findIndex(std::vector<std::complex<float>>, int);
    int16_t estimate_CarrierOffset(std::vector<std::complex<float>>);

    float phase(std::vector<std::complex<float>>, int);

    //	This one is used in the ofdm decoder
    std::vector<std::complex<float>> refTable;

  private:
    fftHandler my_fftHandler;
    RingBuffer<float> *response;
    std::vector<float> phaseDifferences;
    int16_t diff_length;
    int16_t depth;
    int32_t T_u;
    int32_t T_g;
    int16_t carriers;

    std::complex<float> *fft_buffer;
    int32_t fft_counter;
    int32_t framesperSecond;
    int32_t displayCounter;
  signals:
    void showCorrelation(int, int);
};
#endif

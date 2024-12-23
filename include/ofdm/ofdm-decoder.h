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
#ifndef OFDM_DECODER_H
#define OFDM_DECODER_H

#include "constants.h"
#include "dab-params.h"
#include "fft-handler.h"
#include "freq-interleaver.h"
#include "phasetable.h"
#include "ringbuffer.h"
#include <QObject>
#include <cstdint>
#include <vector>

class RadioInterface;

class ofdmDecoder : public QObject {
    Q_OBJECT
  public:
    ofdmDecoder(RadioInterface *, uint8_t,
                RingBuffer<std::complex<float>> *iqBuffer = nullptr);
    ~ofdmDecoder();
    void processBlock_0(std::vector<std::complex<float>>);
    void decode(std::vector<std::complex<float>>, int32_t n, int16_t *);
    void stop();
    void reset();

  private:
    RadioInterface *myRadioInterface;
    dabParams params;
    fftHandler my_fftHandler;
    interLeaver myMapper;

    RingBuffer<std::complex<float>> *iqBuffer;
    float computeQuality(std::complex<float> *);
    void compute_timeOffset(std::complex<float> *, std::complex<float> *);
    void compute_clockOffset(std::complex<float> *, std::complex<float> *);
    void compute_frequencyOffset(std::complex<float> *, std::complex<float> *);
    int32_t T_s;
    int32_t T_u;
    int32_t T_g;
    int32_t nrBlocks;
    int32_t carriers;
    int16_t getMiddle();
    std::vector<complex<float>> phaseReference;
    std::vector<int16_t> ibits;
    std::complex<float> *fft_buffer;
    phaseTable *phasetable;

  signals:
    void showIQ(int);
    void showQuality(float);
};
#endif

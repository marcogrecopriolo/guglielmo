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
 *    Taken from sdr-j-fm, with bug fixes and enhancements.
 *
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */

#ifndef FFT_FILTER_H
#define FFT_FILTER_H

#include "constants.h"
#include "fft.h"

class fftFilter {

public:
    fftFilter(int32_t, int16_t);
    ~fftFilter(void);
    void setBand(int32_t, int32_t, int32_t);
    void setSimple(int32_t, int32_t, int32_t);
    void setLowPass(int32_t, int32_t);
    DSPCOMPLEX Pass(DSPCOMPLEX);
    DSPFLOAT Pass(DSPFLOAT);

private:
    int32_t fftSize;
    int16_t filterDegree;
    int16_t OverlapSize;
    int16_t NumofSamples;
    common_fft* MyFFT;
    DSPCOMPLEX* FFT_A;
    common_ifft* MyIFFT;
    DSPCOMPLEX* FFT_C;
    common_fft* FilterFFT;
    DSPCOMPLEX* filterVector;
    DSPFLOAT* RfilterVector;
    DSPCOMPLEX* Overloop;
    int32_t inp;
};
#endif

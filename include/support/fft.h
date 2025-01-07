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

#ifndef COMMON_FFT_H
#define COMMON_FFT_H

#include "constants.h"

#define FFTW_MALLOC fftwf_malloc
#define FFTW_PLAN_DFT_1D fftwf_plan_dft_1d
#define FFTW_DESTROY_PLAN fftwf_destroy_plan
#define FFTW_FREE fftwf_free
#define FFTW_PLAN fftwf_plan
#define FFTW_EXECUTE fftwf_execute
#include <fftw3.h>

class common_fft {

public:
    common_fft(int32_t);
    ~common_fft(void);
    DSPCOMPLEX* getVector(void);
    void do_FFT(void);
    void do_IFFT(void);
    void do_Shift(void);

private:
    int32_t fft_size;
    DSPCOMPLEX* vector;
    FFTW_PLAN plan;
    void Scale(DSPCOMPLEX*);
};

class common_ifft {
public:
    common_ifft(int32_t);
    ~common_ifft(void);
    DSPCOMPLEX* getVector(void);
    void do_IFFT(void);

private:
    int32_t fft_size;
    DSPCOMPLEX* vector;
    FFTW_PLAN plan;
    void Scale(DSPCOMPLEX*);
};
#endif

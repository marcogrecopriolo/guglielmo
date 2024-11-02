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
 *    Copyright (C) 2009 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef FFT_HANDLER_H
#define FFT_HANDLER_H

#include "constants.h"
#include "dab-params.h"

#define FFTW_MALLOC fftwf_malloc
#define FFTW_PLAN_DFT_1D fftwf_plan_dft_1d
#define FFTW_DESTROY_PLAN fftwf_destroy_plan
#define FFTW_FREE fftwf_free
#define FFTW_PLAN fftwf_plan
#define FFTW_EXECUTE fftwf_execute
#include <fftw3.h>

class fftHandler {

public:
    fftHandler(uint8_t);
    ~fftHandler();
    std::complex<float>* getVector();
    void do_FFT();
    void do_IFFT();

private:
    dabParams p;
    int32_t fftSize;
    std::complex<float>* vector;
    FFTW_PLAN plan;
};
#endif

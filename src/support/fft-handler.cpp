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
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "fft-handler.h"
#include <cstring>

// The basic idea was to have a single instance of the
// fftHandler, for all DFT's. Makes sense, since they are all
// of size T_u.
// However, in the concurrent version this does not work,
// it seems some locking there is inevitable
fftHandler::fftHandler(uint8_t mode): p(mode) {
    this->fftSize = p.get_T_u();
    vector = (std::complex<float>*)
        FFTW_MALLOC(sizeof(std::complex<float>) * fftSize);
    plan = FFTW_PLAN_DFT_1D(fftSize,
        reinterpret_cast<fftwf_complex*>(vector),
        reinterpret_cast<fftwf_complex*>(vector),
        FFTW_FORWARD, FFTW_ESTIMATE);
}

fftHandler::~fftHandler() {
    FFTW_DESTROY_PLAN(plan);
    FFTW_FREE(vector);
}

std::complex<float>* fftHandler::getVector() {
    return vector;
}

void fftHandler::do_FFT() {
    FFTW_EXECUTE(plan);
}

// note that we do not scale here, not needed
// for the purpose we are using it for
void fftHandler::do_IFFT() {
    int16_t i;

    for (i = 0; i < fftSize; i++)
        vector[i] = conj(vector[i]);
    FFTW_EXECUTE(plan);
    for (i = 0; i < fftSize; i++)
        vector[i] = conj(vector[i]);
}

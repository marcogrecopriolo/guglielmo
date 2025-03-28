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
#include "phasereference.h"
#include "radio.h"
#include <cstring>
#include <vector>
/*
 *	\class phaseReference
 *	Implements the correlation that is used to identify
 *	the "first" element (following the cyclic prefix) of
 *	the first non-null block of a frame
 *	The class inherits from the phaseTable.
 */

phaseReference::phaseReference(processParams *p)
    : phaseTable(p->dabMode), params(p->dabMode), my_fftHandler(p->dabMode) {
    int32_t i;
    float Phi_k;

    this->response = p->responseBuffer;
    this->diff_length = p->diff_length;
    this->diff_length = 128;
    this->depth = p->echo_depth;
    this->T_u = params.get_T_u();
    this->T_g = params.get_T_g();
    this->carriers = params.get_carriers();

    refTable.resize(T_u);
    phaseDifferences.resize(diff_length);
    fft_buffer = my_fftHandler.getVector();

    framesperSecond = 2048000 / params.get_T_F();
    displayCounter = 0;

    for (i = 0; i < T_u; i++)
        refTable[i] = std::complex<float>(0, 0);

    for (i = 1; i <= params.get_carriers() / 2; i++) {
        Phi_k = get_Phi(i);
        refTable[i] = std::complex<float>(cos(Phi_k), sin(Phi_k));
        Phi_k = get_Phi(-i);
        refTable[T_u - i] = std::complex<float>(cos(Phi_k), sin(Phi_k));
    }

    //      prepare a table for the coarse frequency synchronization
    //      can be a static one, actually, we are only interested in
    //      the ones with a null
    for (i = 1; i <= diff_length; i++)
        phaseDifferences[i - 1] = abs(arg(refTable[(T_u + i) % T_u] *
                                          conj(refTable[(T_u + i + 1) % T_u])));
}

phaseReference::~phaseReference() {}

/*
 *	\brief findIndex
 *	the vector v contains "T_u" samples that are believed to
 *	belong to the first non-null block of a DAB frame.
 *	We correlate the data in this vector with the predefined
 *	data, and if the maximum exceeds a threshold value,
 *	we believe that that indicates the first sample we were
 *	looking for.
 */
int32_t phaseReference::findIndex(std::vector<std::complex<float>> v,
                                  int threshold) {
    int32_t i;
    int32_t maxIndex = -1;
    float sum = 0;
    float Max = -1000;
    _VLA(float, lbuf, T_u / 2);
    _VLA(float, mbuf, T_u / 2);
    std::vector<int> resultVector;

    memcpy(fft_buffer, v.data(), T_u * sizeof(std::complex<float>));
    my_fftHandler.do_FFT();

    //	into the frequency domain, now correlate
    for (i = 0; i < T_u; i++)
        fft_buffer[i] *= conj(refTable[i]);
    //	and, again, back into the time domain
    my_fftHandler.do_IFFT();

    //	We compute the average and the max signal values
    for (i = 0; i < T_u / 2; i++) {
        lbuf[i] = fastMagnitude(fft_buffer[i]);
        mbuf[i] = lbuf[i];
        sum += lbuf[i];
    }

    sum /= T_u / 2;

    for (i = 0; i < 200; i++) {
        if (lbuf[T_g - 80 + i] > Max) {
            maxIndex = T_g - 80 + i;
            Max = lbuf[T_g - 80 + i];
        }
    }

    if (Max / sum < threshold) {
        return (-abs(Max / sum) - 1);
    }

    if (response != nullptr) {
        if (++displayCounter > framesperSecond / 2) {
            response->putDataIntoBuffer(mbuf, T_u / 2);
            showCorrelation(T_u / 2, T_g);
            displayCounter = 0;
        }
    }
    return maxIndex;
}

//	an approach that works fine is to correlate the phasedifferences
//	between subsequent carriers
#define SEARCH_RANGE (2 * 35)
int16_t
phaseReference::estimate_CarrierOffset(std::vector<std::complex<float>> v) {
    int16_t i, j, index_1 = 100, index_2 = 100;
    _VLA(float, computedDiffs, SEARCH_RANGE + diff_length + 1);

    memcpy(fft_buffer, v.data(), T_u * sizeof(std::complex<float>));
    my_fftHandler.do_FFT();

    for (i = T_u - SEARCH_RANGE / 2; i < T_u + SEARCH_RANGE / 2 + diff_length;
         i++) {
        computedDiffs[i - (T_u - SEARCH_RANGE / 2)] =
            abs(arg(fft_buffer[i % T_u] * conj(fft_buffer[(i + 1) % T_u])));
    }

    float Mmin = 1000;
    float Mmax = 0;
    for (i = T_u - SEARCH_RANGE / 2; i < T_u + SEARCH_RANGE / 2; i++) {
        float sum = 0;
        float sum2 = 0;

        for (j = 1; j < diff_length; j++) {
            if (phaseDifferences[j - 1] < 0.1) {
                sum += computedDiffs[i - (T_u - SEARCH_RANGE / 2) + j];
            }
            if (phaseDifferences[j - 1] > M_PI - 0.1) {
                sum2 += computedDiffs[i - (T_u - SEARCH_RANGE / 2) + j];
            }
        }
        if (sum < Mmin) {
            Mmin = sum;
            index_1 = i;
        }
        if (sum2 > Mmax) {
            Mmax = sum2;
            index_2 = i;
        }
    }

    if (index_1 != index_2)
        return 100;
    return index_1 - T_u;
}

float phaseReference::phase(std::vector<complex<float>> v, int Ts) {
    std::complex<float> sum = std::complex<float>(0, 0);

    for (int i = 0; i < Ts; i++)
        sum += v[i];

    return arg(sum);
}

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
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "freq-interleaver.h"
#include <cstdint>
#include <cstdio>

/**
 *	\brief createMapper
 *	create the mapping table  for the (de-)interleaver
 *	formulas according to section 14.6 (Frequency interleaving)
 *	of the DAB standard
 */

void interLeaver::createMapper(int16_t T_u, int16_t V1, int16_t lwb,
                               int16_t upb, int16_t *v) {
    _VLA(int16_t, tmp, T_u);
    int16_t index = 0;
    int16_t i;

    tmp[0] = 0;
    for (i = 1; i < T_u; i++)
        tmp[i] = (13 * tmp[i - 1] + V1) % T_u;
    for (i = 0; i < T_u; i++) {
        if (tmp[i] == T_u / 2)
            continue;
        if ((tmp[i] < lwb) || (tmp[i] > upb))
            continue;
        // we now have a table with values from lwb .. upb
        v[index++] = tmp[i] - T_u / 2;
        // we now have a table with values from lwb - T_u / 2 .. lwb + T_u / 2
    }
}

interLeaver::interLeaver(dabParams *params) {

    int32_t T_u = params->get_T_u();
    int32_t carriers = params->get_carriers();
    permTable.resize(T_u);

    switch (params->get_dabMode()) {
    case 1:
    default: // shouldn't happen
        createMapper(T_u, 511, 256, 256 + carriers, permTable.data());
        break;

    case 2:
        createMapper(T_u, 127, 64, 64 + carriers, permTable.data());
        break;

    case 3:
        createMapper(T_u, 63, 32, 32 + carriers, permTable.data());
        break;

    case 4:
        createMapper(T_u, 255, 128, 128 + carriers, permTable.data());
        break;
    }
}

interLeaver::~interLeaver() {}

//	according to the standard, the map is a function from
//	0 .. 1535 -> -768 .. 768 (with exclusion of {0})
int16_t interLeaver::mapIn(int16_t n) { return permTable[n]; }

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

#ifndef PHASE_TABLE_H
#define PHASE_TABLE_H

#include "constants.h"
#include "math-helper.h"
#include <cstdint>
#include <cstdio>

struct phasetableElement {
    int32_t kmin, kmax;
    int32_t i;
    int32_t n;
};

class phaseTable {
  public:
    phaseTable(int16_t);
    ~phaseTable();
    float get_Phi(int32_t);

  private:
    struct phasetableElement *currentTable;
    int16_t Mode;
    int32_t h_table(int32_t i, int32_t j);
};
#endif

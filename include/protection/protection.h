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
 *
 *	Simple base class for combining uep and eep deconvolvers
 */
#ifndef PROTECTION_H
#define PROTECTION_H

#include "viterbi-spiral.h"
#include <cstdint>
#include <vector>

extern uint8_t PI_X[];

class protection : public viterbiSpiral {
  public:
    protection(int16_t, int16_t);
    virtual ~protection() {};
    virtual bool deconvolve(int16_t *, int32_t, uint8_t *) { return false; };

  protected:
    int16_t bitRate;
    int32_t outSize;
    std::vector<uint8_t> indexTable;
    std::vector<int16_t> viterbiBlock;
};
#endif

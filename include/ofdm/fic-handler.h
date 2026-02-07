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
*
 * 	FIC data
 */
#ifndef FIC_HANDLER_H
#define FIC_HANDLER_H

#include "dab-params.h"
#include "fib-decoder.h"
#include "viterbi-spiral.h"
#include <QObject>
#include <cstdint>
#include <cstdio>
#include <vector>

class RadioInterface;
class dabParams;

class ficHandler : public fibDecoder {
    Q_OBJECT
  public:
    ficHandler(RadioInterface *, dabParams *);
    ~ficHandler();
    void process_ficBlock(std::vector<int16_t>, int16_t);
    void stop();
    void reset();

  private:
    viterbiSpiral myViterbi;
    uint8_t bitBuffer_out[768];
    int16_t ofdm_input[2304];
    bool punctureTable[3072 + 24];

    void process_ficInput(int16_t);
    int16_t index;
    int16_t BitsperBlock;
    int16_t ficno;
    int16_t ficBlocks;
    int16_t ficMissed;
    int16_t ficRatio;
    uint16_t convState;
    uint8_t PRBS[768];
    //	uint8_t		shiftRegister	[9];
  signals:
    void showFicSuccess(bool);
};
#endif

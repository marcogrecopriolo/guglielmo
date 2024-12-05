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
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */
#include "band-handler.h"
#include "constants.h"

static dabFrequencies frequencies_1[] = {
    { "5A", 174928 },
    { "5B", 176640 },
    { "5C", 178352 },
    { "5D", 180064 },
    { "6A", 181936 },
    { "6B", 183648 },
    { "6C", 185360 },
    { "6D", 187072 },
    { "7A", 188928 },
    { "7B", 190640 },
    { "7C", 192352 },
    { "7D", 194064 },
    { "8A", 195936 },
    { "8B", 197648 },
    { "8C", 199360 },
    { "8D", 201072 },
    { "9A", 202928 },
    { "9B", 204640 },
    { "9C", 206352 },
    { "9D", 208064 },
    { "10A", 209936 },
    { "10B", 211648 },
    { "10C", 213360 },
    { "10D", 215072 },
    { "11A", 216928 },
    { "11B", 218640 },
    { "11C", 220352 },
    { "11D", 222064 },
    { "12A", 223936 },
    { "12B", 225648 },
    { "12C", 227360 },
    { "12D", 229072 },
    { "13A", 230784 },
    { "13B", 232496 },
    { "13C", 234208 },
    { "13D", 235776 },
    { "13E", 237488 },
    { "13F", 239200 },
    { "", 0 }
};

static dabFrequencies frequencies_2[] = {
    { "LA", 1452960 },
    { "LB", 1454672 },
    { "LC", 1456384 },
    { "LD", 1458096 },
    { "LE", 1459808 },
    { "LF", 1461520 },
    { "LG", 1463232 },
    { "LH", 1464944 },
    { "LI", 1466656 },
    { "LJ", 1468368 },
    { "LK", 1470080 },
    { "LL", 1471792 },
    { "LM", 1473504 },
    { "LN", 1475216 },
    { "LO", 1476928 },
    { "LP", 1478640 },
    { "", 0 }
};

bandHandler::bandHandler() {
}

bandHandler::~bandHandler() {
}

void bandHandler::setupChannels(uint8_t band) {
    if (band == BAND_III)
        selectedBand = frequencies_1;
    else
        selectedBand = frequencies_2;
    scanIndex = 0;
}

std::string bandHandler::nextChannel() {
    if (selectedBand[scanIndex].key.empty())
        return "";
    return selectedBand[scanIndex++].key;
}

int32_t bandHandler::frequency(std::string channel) {
    int32_t tunedFrequency = 0;
    int i;

    for (i = 0; !selectedBand[i].key.empty(); i++) {
        if (selectedBand[i].key == channel) {
            tunedFrequency = KHz(selectedBand[i].fKHz);
            break;
        }
    }

    if (tunedFrequency == 0) // should not happen
        tunedFrequency = KHz(selectedBand[0].fKHz);

    return tunedFrequency;
}

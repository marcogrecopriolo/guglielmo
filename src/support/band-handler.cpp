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
    { "5A", 174928, false },
    { "5B", 176640, false },
    { "5C", 178352, false },
    { "5D", 180064, false },
    { "6A", 181936, false },
    { "6B", 183648, false },
    { "6C", 185360, false },
    { "6D", 187072, false },
    { "7A", 188928, false },
    { "7B", 190640, false },
    { "7C", 192352, false },
    { "7D", 194064, false },
    { "8A", 195936, false },
    { "8B", 197648, false },
    { "8C", 199360, false },
    { "8D", 201072, false },
    { "9A", 202928, false },
    { "9B", 204640, false },
    { "9C", 206352, false },
    { "9D", 208064, false },
    { "10A", 209936, false },
    { "10B", 211648, false },
    { "10C", 213360, false },
    { "10D", 215072, false },
    { "11A", 216928, false },
    { "11B", 218640, false },
    { "11C", 220352, false },
    { "11D", 222064, false },
    { "12A", 223936, false },
    { "12B", 225648, false },
    { "12C", 227360, false },
    { "12D", 229072, false },
    { "13A", 230784, false },
    { "13B", 232496, false },
    { "13C", 234208, false },
    { "13D", 235776, false },
    { "13E", 237488, false },
    { "13F", 239200, false },
    { nullptr, 0, false }
};

static dabFrequencies frequencies_2[] = {
    { "LA", 1452960, false },
    { "LB", 1454672, false },
    { "LC", 1456384, false },
    { "LD", 1458096, false },
    { "LE", 1459808, false },
    { "LF", 1461520, false },
    { "LG", 1463232, false },
    { "LH", 1464944, false },
    { "LI", 1466656, false },
    { "LJ", 1468368, false },
    { "LK", 1470080, false },
    { "LL", 1471792, false },
    { "LM", 1473504, false },
    { "LN", 1475216, false },
    { "LO", 1476928, false },
    { "LP", 1478640, false },
    { nullptr, 0, false }
};

dabFrequencies alternatives[100];

bandHandler::bandHandler(const QString& a_band) {
    FILE* f;
    selectedBand = nullptr;

#ifndef IS_WINDOWS
    if (a_band == QString(""))
        return;
    if (a_band != QString("")) {
        f = fopen(a_band.toLatin1().data(), "r");
        if (f == nullptr)
            return;
    }

    //	OK we have a file with - hopefully - some input
    size_t amount = 128;
    int filler = 0;
    char* line = new char[256];
    while ((filler < 100) && (amount > 0)) {
        amount = getline(&line, &amount, f);
        if ((int)amount == 0) {
            break;
        }
        line[amount] = 0;
        char channelName[128];
        int freq;
        int res = sscanf(line, "%s %d", channelName, &freq);
        if (res != 2)
            continue;
        alternatives[filler].key = QString(channelName);
        alternatives[filler].fKHz = freq;
        alternatives[filler].skip = false;
        filler++;
    }

    free(line);
    alternatives[filler].key = "";
    alternatives[filler].fKHz = 0;
    fclose(f);
    selectedBand = alternatives;
#endif
}

bandHandler::~bandHandler() {
}

// The main program calls this once, the combobox will be filled
void bandHandler::setupChannels(QComboBox* s, uint8_t band) {
    int16_t i;
    int16_t c = s->count();

    if (selectedBand == nullptr) { // no preset band
        if (band == BAND_III)
            selectedBand = frequencies_1;
        else
            selectedBand = frequencies_2;
    }

    // clear the fields in the comboBox
    for (i = 0; i < c; i++)
        s->removeItem(c - (i + 1));

    // The table elements are by default all "+";
    for (int i = 0; selectedBand[i].fKHz != 0; i++) {
        s->insertItem(i, selectedBand[i].key, QVariant(i));
    }
}

// find the frequency for a given channel in a given band
int32_t bandHandler::Frequency(QString Channel) {
    int32_t tunedFrequency = 0;
    int i;

    for (i = 0; selectedBand[i].key != nullptr; i++) {
        if (selectedBand[i].key == Channel) {
            tunedFrequency = KHz(selectedBand[i].fKHz);
            break;
        }
    }

    if (tunedFrequency == 0) // should not happen
        tunedFrequency = KHz(selectedBand[0].fKHz);

    return tunedFrequency;
}

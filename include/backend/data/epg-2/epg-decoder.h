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

 *    Copyright (C) 2013 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef EPG_DECODER_2_H
#define EPG_DECODER_2_H

#include <QObject>
#include <QString>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

class progDesc {
  public:
    QString ident;
    QString shortName;
    QString mediumName;
    QString longName;
    int startTime;
    progDesc() { startTime = -1; }
    void clean() {
        startTime = -1;
        longName = "";
        mediumName = "";
        shortName = "";
        ident = "";
    }
};

class epgDecoder : public QObject {
    Q_OBJECT
  public:
    epgDecoder();
    ~epgDecoder();

    int process_epg(uint8_t *, int, uint32_t);

  private:
    uint32_t SId;
    QString stringTable[20];
    int getBit(uint8_t *, int);
    uint32_t getBits(uint8_t *, int, int);

    int process_epgElement(uint8_t *, int);
    int schedule_element(uint8_t *, int);
    int programme_element(uint8_t *, int, progDesc *);
    int process_mediaDescription(uint8_t *, int);
    int process_location(uint8_t *, int, progDesc *);
    int location_element(uint8_t *, int, progDesc *);
    int time_element(uint8_t *, int);
    int genre_element(uint8_t *, int);
    int bearer_element(uint8_t *, int);
    int multimedia(uint8_t *, int);

    void process_tokens(uint8_t *, int, int);
    int process_token(uint8_t *, int);
    void process_45(uint8_t *, int, int);
    void process_46(uint8_t *, int, int);
    QString process_471(uint8_t *, int, int);
    void process_472(uint8_t *, int, int);
    void process_473(uint8_t *, int, int);
    int process_474(uint8_t *, int, int);
    void process_475(uint8_t *, int, int);
    void process_476(uint8_t *, int, int);
    QString process_481(uint8_t *, int, int);
    void process_483(uint8_t *, int, int);
    void record(progDesc *);
  signals:
    void set_epgData(int, int, const QString &);
};
#endif

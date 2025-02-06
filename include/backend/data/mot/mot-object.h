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
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef MOT_OBJECT_H
#define MOT_OBJECT_H

#include "constants.h"
#include "dab-tables.h"
#include <QByteArray>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QObject>
#include <QString>
#include <iterator>
#include <map>

class RadioInterface;

class motObject : public QObject {
    Q_OBJECT
  public:
    motObject(RadioInterface *mr, bool dirElement, uint16_t transportId,
              uint8_t *segment, int32_t segmentSize, bool lastFlag);
    ~motObject();
    void addBodySegment(uint8_t *bodySegment, int16_t segmentNumber,
                        int32_t segmentSize, bool lastFlag);
    uint16_t get_transportId();
    int get_headerSize();

  private:
    bool dirElement;
    QString picturePath;
    uint16_t transportId;
    int16_t numofSegments;
    int32_t segmentSize;
    uint32_t headerSize;
    uint32_t bodySize;
    MOTContentType contentType;
    QString name;
    void handleComplete();
    std::map<int, QByteArray> motMap;

  signals:
    void the_picture(QByteArray, int, QString);
    void handleMotObject(QByteArray, QString, int, bool);
};
#endif

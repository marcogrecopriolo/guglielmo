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
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef IP_DATAHANDLER_H
#define IP_DATAHANDLER_H
#include "constants.h"
#include "ringbuffer.h"
#include "virtual-datahandler.h"
#include <vector>

class RadioInterface;

class ip_dataHandler : public virtual_dataHandler {
    Q_OBJECT
  public:
    ip_dataHandler(RadioInterface *, RingBuffer<uint8_t> *);
    ~ip_dataHandler();
    void add_mscDatagroup(std::vector<uint8_t>);

  private:
    void process_ipVector(std::vector<uint8_t>);
    void process_udpVector(uint8_t *, int16_t);
    int16_t handledPackets;
    RingBuffer<uint8_t> *dataBuffer;
  signals:
    void writeDatagram(int);
};
#endif

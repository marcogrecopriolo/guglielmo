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

#ifndef MOT_HANDLER_H
#define MOT_HANDLER_H

#include "constants.h"
#include "virtual-datahandler.h"
#include "mot-object.h"
#include <vector>
#include <QHash>

class RadioInterface;
class motObject;
class motDirectory;

class motHandler : public virtual_dataHandler {
  public:
    motHandler(RadioInterface *);
    ~motHandler();
    void add_mscDatagroup(std::vector<uint8_t>);

  private:
    RadioInterface *myRadioInterface;
    void setHandle(motObject *, uint16_t);
    motObject *getHandle(uint16_t);
    int orderNumber;
    motObject *currentObject;
    motDirectory *currentDirectory;
    motCache *cache;
};
#endif

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
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "backend-deconvolver.h"
#include "eep-protection.h"
#include "uep-protection.h"

backendDeconvolver::backendDeconvolver(serviceDescriptor *d) {
    if (d->shortForm)
        protectionHandler = new uep_protection(d->bitRate, d->protLevel);
    else
        protectionHandler = new eep_protection(d->bitRate, d->protLevel);
}

backendDeconvolver::~backendDeconvolver() { delete protectionHandler; }

void backendDeconvolver::deconvolve(int16_t *rawBits_in, int32_t length,
                                    uint8_t *outData) {
    protectionHandler->deconvolve(rawBits_in, length, outData);
}

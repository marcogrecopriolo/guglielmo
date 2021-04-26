#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dabMini
 *
 *    dabMini is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dabMini is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dabMini; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 	Default (void) implementation of
 * 	virtual input class
 */
#include	"device-handler.h"

	deviceHandler::deviceHandler	(void) {
	vfoFrequency	= 100000;
}

	deviceHandler::~deviceHandler	(void) {
}

bool	deviceHandler::restartReader	(int32_t) {
	return true;
}

void	deviceHandler::stopReader	(void) {
}

int32_t	deviceHandler::getSamples	(std::complex<float> *v,
	                                         int32_t amount) {
	(void)v; 
	(void)amount; 
	return amount;
}

int32_t	deviceHandler::Samples		(void) {
	return 1024;
}

void	deviceHandler::resetBuffer	(void) {
}

int32_t deviceHandler::getRate  (void) {
        return 192000;
}

void    deviceHandler::setIfGain      (int g) {
}

void    deviceHandler::setLnaGain      (int g) {
}

void    deviceHandler::setAgcControl  (int v) {
}

void    deviceHandler::configurationChanged(void) {
}

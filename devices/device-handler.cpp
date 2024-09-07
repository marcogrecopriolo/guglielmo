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

int32_t	deviceHandler::deviceCount	() {
	return 0;
}

QString	deviceHandler::deviceName	(int32_t) {
	return "";
}

void	deviceHandler::deviceModel	(int32_t, char *, int32_t) {
}

bool	deviceHandler::setDevice	(int32_t) {
	return true;
}

bool	deviceHandler::restartReader	(int32_t) {
	return true;
}

void	deviceHandler::stopReader	(void) {
}

int32_t	deviceHandler::getSamples	(std::complex<float> *v,
	                                         int32_t amount,
						 agcStats *stats) {
	(void)v; 
        stats->overflows = 0;
        stats->min = 0;
        stats->max = 0;
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

int32_t deviceHandler::amplitude  (void) {
        return pow(2, this->bitDepth());
}

void    deviceHandler::setIfGain      (int g) {
}

void    deviceHandler::setLnaGain      (int g) {
}

void    deviceHandler::setAgcControl  (int v) {
}

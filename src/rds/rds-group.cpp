#
/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sdr-j-fm
 *
 *    sdr-j-fm is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    sdr-j-fm is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdr-j-fm; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"rds-group.h"
#include	<stdio.h>

	RDSGroup::RDSGroup	(void) {
	clear ();
}

	RDSGroup::~RDSGroup	(void) {
}

void	RDSGroup::clear		(void) {
	rdsBlocks [BLOCK_A]	= 0;
	rdsBlocks [BLOCK_B]	= 0;
	rdsBlocks [BLOCK_C]	= 0;
	rdsBlocks [BLOCK_D]	= 0;
}

uint16_t	RDSGroup::getBlock	(RdsBlock b) const {
	return rdsBlocks [b];
}

uint16_t	RDSGroup::getBlock_A	() const {
	return rdsBlocks [BLOCK_A];
}

uint16_t	RDSGroup::getBlock_B	() const {
	return rdsBlocks [BLOCK_B];
}

uint16_t	RDSGroup::getBlock_C	() const {
	return rdsBlocks [BLOCK_C];
}

uint16_t	RDSGroup::getBlock_D	() const {
	return rdsBlocks [BLOCK_D];
}

void		RDSGroup::setBlock	(RdsBlock b, uint16_t v) {
	rdsBlocks [b] = v;
}

uint16_t	RDSGroup::getPiCode	(void) {
	return rdsBlocks [BLOCK_A] & 0xFFFF;
}

uint16_t	RDSGroup::getGroupType	(void) {
	return (rdsBlocks [BLOCK_B] >> 12) & 0xF;
}

bool		RDSGroup::isTypeBGroup	(void) {
	return ((rdsBlocks [BLOCK_B] >> 11) & 0x1) != 0;
}

bool		RDSGroup::isTpFlagSet	(void) {
	return ((rdsBlocks [BLOCK_B] >> 10) & 0x1) != 0;
}

uint16_t	RDSGroup::getProgrammeType (void) {
	return (rdsBlocks [BLOCK_B] >> 5) & 0x1F;
}


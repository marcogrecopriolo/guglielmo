#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__RDSGROUP
#define	__RDSGROUP

#include	"constants.h"

class RDSGroup {
public:
		RDSGroup		(void);
		~RDSGroup		(void);
	enum RdsBlock {
		BLOCK_A = 0,
		BLOCK_B = 1,
		BLOCK_C = 2,
		BLOCK_D = 3
	 	};

	 enum GroupType {
		BASIC_TUNING_AND_SWITCHING	= 0,
	        SLOW_LABELING_CODES		= 1,
		RADIO_TEXT			= 2,
	        OPEN_DATA			= 3,
	        CLOCKTIME_AND_DATE		= 4,
	        ODA				= 5,
	        ODA_2				= 6,
	        RADIO_PAGING			= 7,
		TMC_DATA			= 8,
	        EMERGENCY_WARNING		= 9,
	        PROGRAM_TYPE_NAME		= 10,
	        ODA_3                           = 11,
	        ODA_4				= 12,
	        ENHANCED_PAGING			= 13,
	        ENHANCED_OTHER_NETWORKS_INFO	= 14
		};

void		clear			(void);
uint16_t	getBlock		(RdsBlock) const;
uint16_t	getBlock_A		(void) const;
uint16_t	getBlock_B		(void) const;
uint16_t	getBlock_C		(void) const;
uint16_t	getBlock_D		(void) const;
void		setBlock		(RdsBlock, uint16_t);
uint16_t	getPiCode		(void);
uint16_t	getGroupType		(void);
bool		isTypeBGroup		(void);
bool		isTpFlagSet		(void);
uint16_t	getProgrammeType	(void);

static const uint8_t NUM_RDS_GROUPS	= 16;
static const uint8_t NUM_BLOCKS_PER_RDSGROUP	= BLOCK_D + 1;
private:
uint16_t	rdsBlocks [NUM_BLOCKS_PER_RDSGROUP];
};
#endif


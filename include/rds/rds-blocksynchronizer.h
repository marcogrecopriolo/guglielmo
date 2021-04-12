#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This part of the FM demodulation software is largely a
 *    rewrite and local adaptation of FMSTACK software
 *    Technical University of Munich, Institute for Integrated Systems (LIS)
 *    FMSTACK Copyright (C) 2010 Michael Feilen
 * 
 *    Author(s)       : Michael Feilen (michael.feilen@tum.de)
 *    Initial release : 01.09.2009
 *    Last changed    : 09.03.2010
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

#ifndef __RDS_BLOCK_SYNCHRONIZER
#define	__RDS_BLOCK_SYNCHRONIZER

#include	"constants.h"
#include	"rds-group.h"
#include	<QObject>
class	RadioInterface;

class	rdsBlockSynchronizer: public QObject {
Q_OBJECT
public:
		rdsBlockSynchronizer	(RadioInterface *);
		~rdsBlockSynchronizer	(void);
	void	setFecEnabled		(bool);

	enum SyncResult {
	   	   RDS_WAITING_FOR_BLOCK_A,
	   	   RDS_BUFFERING,
	   	   RDS_NO_SYNC,
	   	   RDS_NO_CRC,
	   	   RDS_COMPLETE_GROUP
	};

	void		reset			(void);
	SyncResult	pushBit			(bool, RDSGroup *);
	SyncResult	pushBitSynchronized	(bool, RDSGroup *);
	SyncResult	pushBitinBlockA		(bool, RDSGroup *);
	SyncResult	pushBitNotSynchronized	(bool, RDSGroup *);
	int16_t		getNumSyncErrors	(void);
	void		resetResyncErrorCounter	(void);
	void		resetCRCErrorCounter	(void);
	int16_t		getNumCRCErrors		(void);
	void		resync			(void);
	DSPFLOAT	getBitErrorRate		(void);

private:
	RadioInterface	*MyRadioInterface;
	bool		decodeBlock		(RDSGroup::RdsBlock,
	                                         uint32_t, bool);
	uint32_t	getSyndrome		(uint32_t, uint32_t);
	void		setNextBlock		(void);
	uint32_t	doMeggit		(uint32_t);
	uint32_t	getOffset		(RDSGroup::RdsBlock, bool);
	bool		crcFecEnabled;

	static const uint32_t NUM_BITS_CRC		= 10;
	static const uint32_t NUM_BITS_BLOCK_PAYLOAD	= 16;
	static const uint32_t NUM_BITS_PER_BLOCK	= NUM_BITS_CRC +
	                                                NUM_BITS_BLOCK_PAYLOAD;
	static const uint32_t OFFSET_WORD_BLOCK_A	= 0xFC;
	static const uint32_t OFFSET_WORD_BLOCK_B	= 0x198;
	static const uint32_t OFFSET_WORD_BLOCK_C1	= 0x168;
	static const uint32_t OFFSET_WORD_BLOCK_C2	= 0x350;
	static const uint32_t OFFSET_WORD_BLOCK_D	= 0x1B4;
//x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + 1
	static const uint32_t CRC_POLY			= 0x5B9;
	static const uint32_t REMAINDER_POLY		= 0x31B;
	static const uint32_t NUM_BITS_BER_CALC_RESET	= 4000;

	static const RDSGroup::RdsBlock SYNC_END_BLOCK	= RDSGroup::BLOCK_C;
	
	uint32_t	rdsBitstream;	// only interested in 26 bits
	bool		rdsIsSynchronized;
	RDSGroup::RdsBlock	rdsCurrentBlock;
	DSPFLOAT	rdsbitErrorRate;
	uint16_t	rdsBitsinBlock;
	uint16_t	rdsBitsProcessed;
	uint16_t	rdsNumofSyncErrors;
	uint16_t	rdsNumofCRCErrors;
	uint16_t	rdsNumofBitErrors;
signals:
	void		setRDSisSynchronized	(bool);
	void		setbitErrorRate		(double);
};

#endif


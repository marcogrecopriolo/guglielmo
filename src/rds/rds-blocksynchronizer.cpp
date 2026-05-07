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
 *    Taken from sdr-j-fm, with bug fixes and enhancements.
 *
 *    Copyright (C)  2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This part of the FM demodulation software is partly based on
 *    FMSTACK software
 *    Technical University of Munich, Institute for Integrated Systems (LIS)
 *    FMSTACK Copyright (C) 2010 Michael Feilen
 *
 *    and partly on CuteSDR
 *    (c) Moey Wheatly 2011
 *
 *    Error correction taken from RedSEA
 *    (c) Oona Räisänen
 */
#include "rds-blocksynchronizer.h"
#include "radio.h"
#include "logging.h"

rdsBlockSynchronizer::rdsBlockSynchronizer(RadioInterface* RI) {
    MyRadioInterface = RI;
    crcFecEnabled = true;
    maxSyncErrors = 3;
    maxBlerErrors = 3;

    this->reset();
    connect(this, SIGNAL(setRDSisSynchronized(bool)),
	MyRadioInterface, SLOT(showQuality(bool)));
}

rdsBlockSynchronizer::~rdsBlockSynchronizer(void) {
    disconnect(this, SIGNAL(setRDSisSynchronized(bool)),
	MyRadioInterface, SLOT(showQuality(bool)));
}

void rdsBlockSynchronizer::setParameters(bool c, int32_t s, int32_t b) {
    crcFecEnabled = c;
    maxSyncErrors = s;
    maxBlerErrors = b;
    log(LOG_RDS, LOG_MIN, "parameters e/c %i sync %i bler %i", c, s, b);
}

void rdsBlockSynchronizer::reset(void) {
    bitStream = 0;
    syncErrors = 0;
    isSynchronized = false;
    currentBlock = RDSGroup::BLOCK_A;
    bitsInBlock = 0;
    bitsProcessed = 0;
    bitErrors = 0;

    resetCRCErrorCounter();
    resetResyncErrorCounter();
    setRDSisSynchronized(false);

    blerHead = 0;
    blerErrors = 0;
    memset(blerWindow, 0, sizeof(blerWindow));
}

void rdsBlockSynchronizer::setNextBlock(void) {
    switch (currentBlock) {
    case RDSGroup::BLOCK_A:
	currentBlock = RDSGroup::BLOCK_B;
	break;
    case RDSGroup::BLOCK_B:
	currentBlock = RDSGroup::BLOCK_C;
	break;
    case RDSGroup::BLOCK_C:
	currentBlock = RDSGroup::BLOCK_D;
	break;
    case RDSGroup::BLOCK_D:
	currentBlock = RDSGroup::BLOCK_A;
	break;
    }
}

bool rdsBlockSynchronizer::reSynchronise(void) {
    return (syncErrors > maxSyncErrors);
}

void rdsBlockSynchronizer::resync(void) {
    currentBlock = RDSGroup::BLOCK_A;
    isSynchronized = false;
    setRDSisSynchronized(false);
    bitsInBlock = 0;

    blerHead = 0;
    blerErrors = 0;
    memset(blerWindow, 0, sizeof(blerWindow));
}

void rdsBlockSynchronizer::resetResyncErrorCounter(void) {
    syncErrors = 0;
}

void rdsBlockSynchronizer::resetCRCErrorCounter(void) {
    CRCErrors = 0;
}

uint32_t rdsBlockSynchronizer::getSyndrome(uint32_t bits,
    uint32_t offsetWord) {
    const uint32_t block = bits ^ offsetWord;
    uint32_t reg = 0;
    int16_t k;

    for (k = NUM_BITS_PER_BLOCK - 1; k >= 0; k--) {
	uint32_t msb = reg & (1 << (NUM_BITS_CRC - 1));
	reg <<= 1;
	if (msb > 0)
	    reg ^= CRC_POLY;
	if (((block >> k) & 0x1) != 0)
	    reg ^= REMAINDER_POLY;
    }

    return reg;
}

bool rdsBlockSynchronizer::decodeBlock(RDSGroup::RdsBlock b,
    uint32_t bits,
    bool isTypeBGroup) {
    uint32_t offsetWord = 0;
    uint32_t syndrome = 0;

    offsetWord = getOffset(b, isTypeBGroup);

    // calculate  crc and get the syndrome
    syndrome = getSyndrome(bits, offsetWord);

    // During synchronization we allow zero errors,
    // thus we check the syndrome register directly here
    if (!isSynchronized)
	return syndrome == 0;

    // Increment for BER calculations
    bitsProcessed += NUM_BITS_BLOCK_PAYLOAD;

    if (syndrome != 0 && crcFecEnabled)
	syndrome = correctBurstErrors(syndrome, b, isTypeBGroup);

    // if the syndrome is not equal to zero there was an error in the crc
    // When no fec is used, we mark all bits as erroneous
    if (syndrome != 0)
	bitErrors += NUM_BITS_BLOCK_PAYLOAD;

    // calc BER
    if (bitsProcessed >= NUM_BITS_BER_CALC_RESET) {
	bitErrors = 0;
	bitsProcessed = 0;
    }

    return syndrome == 0;
}

// Precomputed syndrome: error vector lookup table for burst error correction.
// Implements IEC 62106 / EN 50067 Annex B burst error correction.
// For each offset word, we precompute the syndrome produced by every possible
// 1-bit and 2-bit contiguous burst error at every bit position in the 26-bit block.
// Correction is then a single map lookup: O(1) per block, no iteration.
// This correctly handles burst errors anywhere in the block including check bits,
// and handles 2-bit bursts which Meggitt cannot.
//
// BurstErrorTable constructor ? builds the syndrome->error_vector lookup
// for 1-bit and 2-bit contiguous burst errors at every bit position,
// for each of the 5 RDS offset words.
BurstErrorTable::BurstErrorTable() {
    auto calcSyndrome = [](uint32_t bits, uint32_t offsetWord) -> uint32_t {
	const uint32_t block = bits ^ offsetWord;
	uint32_t reg = 0;
	for (int k = (int)NUM_BITS_PER_BLOCK - 1; k >= 0; k--) {
	    uint32_t msb = reg & (1U << (NUM_BITS_CRC - 1));
	    reg <<= 1;
	    if (msb > 0)
		reg ^= CRC_POLY;
	    if (((block >> k) & 0x1) != 0)
		reg ^= REMAINDER_POLY;
	}
	return reg;
    };

    const uint32_t offsetWords[5] = {
	OFFSET_WORD_BLOCK_A,
	OFFSET_WORD_BLOCK_B,
	OFFSET_WORD_BLOCK_C1,
	OFFSET_WORD_BLOCK_C2,
	OFFSET_WORD_BLOCK_D
    };

    for (int b = 0; b < 5; b++) {

	// 1-bit errors at every position
	for (uint32_t shift = 0; shift < NUM_BITS_PER_BLOCK; shift++) {
	    uint32_t errorVector = (1U << shift) & BLOCK_MASK;
	    uint32_t syndrome = calcSyndrome(errorVector, offsetWords[b]);
	    table[b][syndrome] = errorVector;
	}

	// 2-bit contiguous burst errors at every position
	for (uint32_t shift = 0; shift < NUM_BITS_PER_BLOCK; shift++) {
	    uint32_t errorVector = (0b11U << shift) & BLOCK_MASK;
	    uint32_t syndrome = calcSyndrome(errorVector, offsetWords[b]);
	    if (table[b].find(syndrome) == table[b].end())
		table[b][syndrome] = errorVector;
	}
    }
}

// Returns the table index for a given block / isTypeBGroup combination
static int burstTableIndex(RDSGroup::RdsBlock b, bool isTypeBGroup) {
    switch (b) {
      case RDSGroup::BLOCK_A:
	return 0;
      case RDSGroup::BLOCK_B:
	return 1;
      case RDSGroup::BLOCK_C:
	return isTypeBGroup? 3: 2;
      case RDSGroup::BLOCK_D:
	return 4;
      default:
	return 0;
    }
}

// error correction using precomputed burst error lookup table.
// Replaces the Meggitt shift register approach with an O(1) table lookup.
// Corrects any single-bit error or 2-bit contiguous burst anywhere in the
// full 26-bit block including check bits.
uint32_t rdsBlockSynchronizer::correctBurstErrors(uint32_t syndrome,
    RDSGroup::RdsBlock b, bool isTypeBGroup) {
    static const BurstErrorTable burstTable;

    const int idx = burstTableIndex(b, isTypeBGroup);

    const auto& tbl = burstTable.table[idx];
    const auto it = tbl.find(syndrome);
    if (it != tbl.end()) {
	bitStream ^= it->second;
	bitErrors++;
	return 0;
    }

    return syndrome & 0x3FF;
}

uint32_t rdsBlockSynchronizer::getOffset(RDSGroup::RdsBlock b,
    bool isTypeBGroup) {
    switch (b) {
    default:
    case RDSGroup::BLOCK_A:
	return OFFSET_WORD_BLOCK_A;
    case RDSGroup::BLOCK_B:
	return OFFSET_WORD_BLOCK_B;
    case RDSGroup::BLOCK_C:
	return isTypeBGroup? OFFSET_WORD_BLOCK_C2: OFFSET_WORD_BLOCK_C1;
    case RDSGroup::BLOCK_D:
	return OFFSET_WORD_BLOCK_D;
    }
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBit(bool b, RDSGroup* rdsGrp) {

    if (isSynchronized)
	return pushBitSynchronized(b, rdsGrp);

    // not synchronized, distinguish block A from the rest
    if (currentBlock == RDSGroup::BLOCK_A)
	return pushBitInBlockA(b, rdsGrp);
    else
	return pushBitNotSynchronized(b, rdsGrp);
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitSynchronized(bool b, RDSGroup* rdsGrp) {

    // assert isSynchronized == true
    bitStream = (bitStream << 1) | (b ? 01: 00);
    if (++bitsInBlock < NUM_BITS_PER_BLOCK)
	return RDS_BUFFERING;

    bitsInBlock = 0;

    // decode bitstream for current block
    bool blockOk = decodeBlock(currentBlock,
	    bitStream,
	    rdsGrp->isTypeBGroup());
    // update rolling BLER window
    uint8_t incoming = blockOk? 0: 1;
    blerErrors += incoming - blerWindow[blerHead];
    blerWindow[blerHead] = incoming;
    blerHead = (blerHead + 1) % BLER_WINDOW;

    if (!blockOk) {
        CRCErrors++;
        if (blerErrors > maxBlerErrors)
            return RDS_NO_CRC;

	// tolerate the error but skip the payload
        currentBlock = RDSGroup::BLOCK_A;
	bitsInBlock = 0;

	// that's a bit cheeky, but let's flag ourselves as synchronised
	setRDSisSynchronized(true);
        return RDS_SKIP_GROUP;
    }

    // Extract payload data
    rdsGrp->setBlock(currentBlock,
	(uint16_t)(bitStream >> NUM_BITS_CRC));

    // check to see whether we are at the end of the group
    SyncResult result = (currentBlock == RDSGroup::BLOCK_D) ? RDS_COMPLETE_GROUP: RDS_BUFFERING;
    setNextBlock();
    setRDSisSynchronized(true);
    return result;
}

// The first block is block-a. We keep shifting bits
// until we have a valid block a
rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitInBlockA(bool b, RDSGroup* rdsGrp) {
    uint32_t offsetWord = 0;
    uint32_t syndrome = 0;

    // assert isSynchronized != true and currentBlock == BLOCK_A
    bitStream = (bitStream << 1) | (b? 01: 00);
    offsetWord = getOffset(RDSGroup::BLOCK_A,
	rdsGrp->isTypeBGroup());

    // calculate  crc and get the syndrome
    syndrome = getSyndrome(bitStream & 0x3FFFFFF, offsetWord);

    // During synchronization phase NO errors are allowed
    // in case of error, we continue shifting since we are in BLOCK_A
    if (syndrome != 0)
	return RDS_WAITING_FOR_BLOCK_A;

    // syndrome == 0, extract the 16 bit data, we know it is block A
    rdsGrp->setBlock(RDSGroup::BLOCK_A,
	(uint16_t)(bitStream >> NUM_BITS_CRC));

    // go for the next block
    bitsInBlock = 0;
    setNextBlock();
    return RDS_BUFFERING;
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitNotSynchronized(bool b, RDSGroup* rdsGrp) {
    uint32_t offsetWord = 0;
    uint32_t syndrome = 0;

    // assert isSynchronized != true and currentBlock != BLOCK_A
    bitStream = (bitStream << 1) | (b? 01: 00);
    if (bitsInBlock < NUM_BITS_PER_BLOCK - 1) {
	bitsInBlock++;
	return RDS_BUFFERING;
    }

    bitsInBlock = 0;

    offsetWord = getOffset(currentBlock,
	rdsGrp->isTypeBGroup());

    // calculate  crc and get the syndrome
    syndrome = getSyndrome(bitStream, offsetWord);

    // we want zero errors during synchronization
    if (syndrome != 0) {
	syncErrors++;
	return RDS_NO_SYNC;
    }

    // Extract payload data
    rdsGrp->setBlock(currentBlock,
	(uint16_t)(bitStream >> NUM_BITS_CRC));

    // Look for next blocks in the group
    if (currentBlock < SYNC_END_BLOCK) {
	setNextBlock();
	return RDS_BUFFERING;
    }

    // if we are here, we reached the end block and
    // are synchronized, show it to the world
    isSynchronized = true;
    setRDSisSynchronized(true);

    // check to see whether sync was successful
    SyncResult result = (currentBlock == RDSGroup::BLOCK_D) ? RDS_COMPLETE_GROUP: RDS_BUFFERING;
    setNextBlock();
    return result;
}

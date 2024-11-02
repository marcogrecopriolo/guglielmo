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
 */
#include "rds-blocksynchronizer.h"
#include "radio.h"

rdsBlockSynchronizer::rdsBlockSynchronizer(RadioInterface* RI) {
    MyRadioInterface = RI;
    crcFecEnabled = true;

    this->reset();
    connect(this, SIGNAL(setRDSisSynchronized(bool)),
        MyRadioInterface, SLOT(showQuality(bool)));
}

rdsBlockSynchronizer::~rdsBlockSynchronizer(void) {
    disconnect(this, SIGNAL(setRDSisSynchronized(bool)),
        MyRadioInterface, SLOT(showQuality(bool)));
}

void rdsBlockSynchronizer::setFecEnabled(bool b) {
    crcFecEnabled = b;
}

void rdsBlockSynchronizer::reset(void) {
    rdsBitstream = 0;
    rdsIsSynchronized = false;
    rdsCurrentBlock = RDSGroup::BLOCK_A;
    rdsbitErrorRate = 0;
    rdsBitsinBlock = 0;
    rdsBitsProcessed = 0;
    rdsNumofBitErrors = 0;

    resetCRCErrorCounter();
    resetResyncErrorCounter();
    setRDSisSynchronized(false);
}

void rdsBlockSynchronizer::setNextBlock(void) {
    switch (rdsCurrentBlock) {
    case RDSGroup::BLOCK_A:
        rdsCurrentBlock = RDSGroup::BLOCK_B;
        break;
    case RDSGroup::BLOCK_B:
        rdsCurrentBlock = RDSGroup::BLOCK_C;
        break;
    case RDSGroup::BLOCK_C:
        rdsCurrentBlock = RDSGroup::BLOCK_D;
        break;
    case RDSGroup::BLOCK_D:
        rdsCurrentBlock = RDSGroup::BLOCK_A;
        break;
    }
}

int16_t rdsBlockSynchronizer::getNumSyncErrors(void) {
    return rdsNumofSyncErrors;
}

DSPFLOAT rdsBlockSynchronizer::getBitErrorRate(void) {
    return rdsbitErrorRate;
}

void rdsBlockSynchronizer::resync(void) {
    rdsCurrentBlock = RDSGroup::BLOCK_A;
    rdsIsSynchronized = false;
    setRDSisSynchronized(false);
    rdsBitsinBlock = 0;
}

void rdsBlockSynchronizer::resetResyncErrorCounter(void) {
    rdsNumofSyncErrors = 0;
}

int16_t rdsBlockSynchronizer::getNumCRCErrors(void) {
    return rdsNumofCRCErrors;
}

void rdsBlockSynchronizer::resetCRCErrorCounter(void) {
    rdsNumofCRCErrors = 0;
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
    if (!rdsIsSynchronized)
        return syndrome == 0;

    // Increment for BER calculations
    rdsBitsProcessed += NUM_BITS_BLOCK_PAYLOAD;

    if (syndrome != 0 && crcFecEnabled)
        doMeggit(syndrome);

    // if the syndrome is not equal to zero there was an error in the crc
    // When no fec is used, we mark all bits as erroneous
    if (syndrome != 0)
        rdsNumofBitErrors += NUM_BITS_BLOCK_PAYLOAD;

    // calc BER
    rdsbitErrorRate = (DSPFLOAT)rdsNumofBitErrors / rdsBitsProcessed;
    setbitErrorRate(rdsbitErrorRate);
    if (rdsBitsProcessed >= NUM_BITS_BER_CALC_RESET) {
        rdsNumofBitErrors = 0;
        rdsBitsProcessed = 0;
    }

    return syndrome == 0;
}

// error correction for the payload. The algorithm is described in
// the CuteSDR description of Moe Wheatley.
// If the msb of the syndrome is "1" and the 5 least significant bits
// of the syndrome are all zero, then the corresponding bit
// is corrected by inverting it.
uint32_t rdsBlockSynchronizer::doMeggit(uint32_t syndrome) {
    uint32_t correctedBits = 0;
    uint32_t correctMask = (1 << (NUM_BITS_PER_BLOCK - 1));
    uint16_t i;

    for (i = 0; i < NUM_BITS_BLOCK_PAYLOAD; i++) {
        if (syndrome & 0x200) {
            if ((syndrome & 0x1f) == 0) { // lsb 5 bits
                rdsBitstream ^= correctMask;
                correctedBits++;
                rdsNumofBitErrors++;
            } else {
                syndrome ^= CRC_POLY;
            }
        }
        syndrome <<= 1;
        correctMask >>= 1;
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
        return isTypeBGroup ? OFFSET_WORD_BLOCK_C2 : OFFSET_WORD_BLOCK_C1;

    case RDSGroup::BLOCK_D:
        return OFFSET_WORD_BLOCK_D;
    }
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBit(bool b, RDSGroup* rdsGrp) {

    if (rdsIsSynchronized)
        return pushBitSynchronized(b, rdsGrp);

    // not synchronized, distinguish block A from the rest
    if (rdsCurrentBlock == RDSGroup::BLOCK_A)
        return pushBitinBlockA(b, rdsGrp);
    else
        return pushBitNotSynchronized(b, rdsGrp);
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitSynchronized(bool b, RDSGroup* rdsGrp) {

    // assert rdsIsSynchronized == true
    rdsBitstream = (rdsBitstream << 1) | (b ? 01 : 00);
    if (++rdsBitsinBlock < NUM_BITS_PER_BLOCK)
        return RDS_BUFFERING;

    rdsBitsinBlock = 0;
 
    // decode bitstream for current block
    if (!decodeBlock(rdsCurrentBlock,
            rdsBitstream,
            rdsGrp->isTypeBGroup())) {
        rdsNumofCRCErrors++;
        return RDS_NO_CRC;
    }

    // Extract payload data
    rdsGrp->setBlock(rdsCurrentBlock,
        (uint16_t)(rdsBitstream >> NUM_BITS_CRC));

    // check to see whether we are at the end of the group
    SyncResult result = (rdsCurrentBlock == RDSGroup::BLOCK_D) ? RDS_COMPLETE_GROUP : RDS_BUFFERING;
    setNextBlock();
    setRDSisSynchronized(true);
    return result;
}

// The first block is block-a. We keep shifting bits
// until we have a valid block a
rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitinBlockA(bool b, RDSGroup* rdsGrp) {
    uint32_t offsetWord = 0;
    uint32_t syndrome = 0;

    // assert rdsIsSynchronized != true and rdsCurrentBlock == BLOCK_A
    rdsBitstream = (rdsBitstream << 1) | (b ? 01 : 00);
    offsetWord = getOffset(RDSGroup::BLOCK_A,
        rdsGrp->isTypeBGroup());
    //
    // calculate  crc and get the syndrome
    syndrome = getSyndrome(rdsBitstream & 0x3FFFFFF, offsetWord);

    // During synchronization phase NO errors are allowed
    // in case of error, we continue shifting since we are in BLOCK_A
    if (syndrome != 0) // still errors
        return RDS_WAITING_FOR_BLOCK_A;

    // syndrome == 0, extract the 16 bit data, we know
    // it is block A
    rdsGrp->setBlock(RDSGroup::BLOCK_A,
        (uint16_t)(rdsBitstream >> NUM_BITS_CRC));

    // go for the next block
    rdsBitsinBlock = 0;
    setNextBlock();
    setRDSisSynchronized(false);
    return RDS_BUFFERING;
}

rdsBlockSynchronizer::SyncResult
rdsBlockSynchronizer::pushBitNotSynchronized(bool b, RDSGroup* rdsGrp) {
    uint32_t offsetWord = 0;
    uint32_t syndrome = 0;

    // assert rdsIsSynchronized != true and rdsCurrentBlock != BLOCK_A
    rdsBitstream = (rdsBitstream << 1) | (b ? 01 : 00);
    if (rdsBitsinBlock < NUM_BITS_PER_BLOCK - 1) {
        rdsBitsinBlock++;
        return RDS_BUFFERING;
    }

    rdsBitsinBlock = 0;

    offsetWord = getOffset(rdsCurrentBlock,
        rdsGrp->isTypeBGroup());

    // calculate  crc and get the syndrome
    syndrome = getSyndrome(rdsBitstream, offsetWord);

    // we want zero errors during synchronization
    // this we check the syndrome register directly here
    if (syndrome != 0) {

        // error during synchronization
        rdsNumofSyncErrors++;
        return RDS_NO_SYNC;
    }

    // Extract payload data
    rdsGrp->setBlock(rdsCurrentBlock,
        (uint16_t)(rdsBitstream >> NUM_BITS_CRC));

    // Look for next blocks in the group
    if (rdsCurrentBlock < SYNC_END_BLOCK) {
        setNextBlock();
        return RDS_BUFFERING;
    }

    // if we are here, we reached the end block and
    // are synchronized, show it to the world
    rdsIsSynchronized = true;
    setRDSisSynchronized(true);

    // check to see whether sync was successful
    SyncResult result = (rdsCurrentBlock == RDSGroup::BLOCK_D) ? RDS_COMPLETE_GROUP : RDS_BUFFERING;
    setNextBlock();
    return result;
}

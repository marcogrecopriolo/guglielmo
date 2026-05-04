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
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */

#ifndef RDS_BLOCK_SYNCHRONIZER_H
#define RDS_BLOCK_SYNCHRONIZER_H
#include "constants.h"
#include "rds-group.h"
#include <QObject>
#include <array>
#include <unordered_map>

class RadioInterface;

// RDS (26,16) cyclic code constants
static const uint32_t NUM_BITS_CRC = 10;
static const uint32_t NUM_BITS_BLOCK_PAYLOAD = 16;
static const uint32_t NUM_BITS_PER_BLOCK = NUM_BITS_CRC + NUM_BITS_BLOCK_PAYLOAD;
static const uint32_t BLOCK_MASK = (1U << NUM_BITS_PER_BLOCK) - 1U;
static const uint32_t OFFSET_WORD_BLOCK_A = 0xFC;
static const uint32_t OFFSET_WORD_BLOCK_B = 0x198;
static const uint32_t OFFSET_WORD_BLOCK_C1 = 0x168;
static const uint32_t OFFSET_WORD_BLOCK_C2 = 0x350;
static const uint32_t OFFSET_WORD_BLOCK_D = 0x1B4;
// x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + 1
static const uint32_t CRC_POLY = 0x5B9;
static const uint32_t REMAINDER_POLY = 0x31B;
static const uint32_t NUM_BITS_BER_CALC_RESET = 4000;

// Precomputed burst error correction lookup table.
// Maps syndrome -> error_vector for 1-bit and 2-bit contiguous burst errors
// at every bit position in the 26-bit block, for each of the 5 offset words.
// Built once at startup, used for O(1) correction in decodeBlock.
struct BurstErrorTable {
    std::unordered_map<uint32_t, uint32_t> table[5]; // 0=A, 1=B, 2=C1, 3=C2, 4=D
    BurstErrorTable();
};

class rdsBlockSynchronizer: public QObject {
    Q_OBJECT

public:
    rdsBlockSynchronizer(RadioInterface*);
    ~rdsBlockSynchronizer(void);
    void setFecEnabled(bool);
    enum SyncResult {
	RDS_WAITING_FOR_BLOCK_A,
	RDS_BUFFERING,
	RDS_NO_SYNC,
	RDS_NO_CRC,
	RDS_COMPLETE_GROUP
    };
    void reset(void);
    SyncResult pushBit(bool, RDSGroup*);
    SyncResult pushBitSynchronized(bool, RDSGroup*);
    SyncResult pushBitInBlockA(bool, RDSGroup*);
    SyncResult pushBitNotSynchronized(bool, RDSGroup*);
    bool reSynchronise(void);
    void resetResyncErrorCounter(void);
    void resetCRCErrorCounter(void);
    void resync(void);

private:
    RadioInterface* MyRadioInterface;
    bool decodeBlock(RDSGroup::RdsBlock, uint32_t, bool);
    uint32_t getSyndrome(uint32_t, uint32_t);
    void setNextBlock(void);
    uint32_t correctBurstErrors(uint32_t syndrome, RDSGroup::RdsBlock b, bool isTypeBGroup);
    uint32_t getOffset(RDSGroup::RdsBlock, bool);
    bool crcFecEnabled;
    static const RDSGroup::RdsBlock SYNC_END_BLOCK = RDSGroup::BLOCK_C;
    uint32_t rdsBitstream;
    uint32_t rdsBitCount;
    bool rdsIsSynchronized;
    RDSGroup::RdsBlock rdsCurrentBlock;
    uint16_t rdsBitsinBlock;
    uint16_t rdsBitsProcessed;
    uint16_t rdsNumofSyncErrors;
    uint16_t rdsNumofCRCErrors;
    uint16_t rdsNumofBitErrors;

signals:
    void setRDSisSynchronized(bool);
};
#endif

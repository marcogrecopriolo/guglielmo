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
 *
 *	This part of the jsdr is a mixture of code  based on code from
 *	various sources. Two in particular:
 *	
 *    FMSTACK Copyright (C) 2010 Michael Feilen
 * 
 *    Author(s)       : Michael Feilen (michael.feilen@tum.de)
 *    Initial release : 01.09.2009
 *    Last changed    : 09.03.2010
 *	
 *	cuteSDR (c) M Wheatly 2011
 */

#ifndef RDS_DECODER_H
#define RDS_DECODER_H

#include "constants.h"
#include "fft.h"
#include "iir-filters.h"
#include "rds-blocksynchronizer.h"
#include "rds-group.h"
#include "rds-groupdecoder.h"
#include "trigtabs.h"
#include <QObject>

class RadioInterface;

class rdsDecoder: public QObject {
    Q_OBJECT

public:
    rdsDecoder(RadioInterface*, int32_t, trigTabs*);
    ~rdsDecoder(void);
    enum RdsMode {
        NO_RDS = 0,
        RDS1 = 1,
        RDS2 = 2
    };
    void doDecode(DSPFLOAT, DSPFLOAT*, RdsMode);
    void reset(void);

private:
    void processBit(bool);
    void doDecode1(DSPFLOAT, DSPFLOAT*);
    void doDecode2(DSPFLOAT, DSPFLOAT*);
    int32_t sampleRate;
    int32_t numofFrames;
    trigTabs* fastTrigTabs;
    RadioInterface* MyRadioInterface;
    RDSGroup* my_rdsGroup;
    rdsBlockSynchronizer* my_rdsBlockSync;
    rdsGroupDecoder* my_rdsGroupDecoder;
    DSPFLOAT omegaRDS;
    int32_t symbolCeiling;
    int32_t symbolFloor;
    bool prevBit;
    DSPFLOAT bitIntegrator;
    DSPFLOAT bitClkPhase;
    DSPFLOAT prev_clkState;
    bool Resync;

    DSPFLOAT* rdsBuffer;
    DSPFLOAT* rdsKernel;
    int16_t ip;
    int16_t rdsfilterSize;
    DSPFLOAT Match(DSPFLOAT);
    BandPassIIR* sharpFilter;
    DSPFLOAT rdsLastSyncSlope;
    DSPFLOAT rdsLastSync;
    DSPFLOAT rdsPrevSync;
    DSPFLOAT rdsLastData;
    DSPFLOAT rdsPrevData;
    bool previousBit;
    DSPFLOAT* syncBuffer;
    int16_t p;
    void synchronizeOnBitClk(DSPFLOAT*, int16_t);

signals:
    void setCRCErrors(int);
    void setSyncErrors(int);
    void setbitErrorRate(int);
};
#endif

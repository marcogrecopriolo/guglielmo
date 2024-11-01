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

#ifndef	FM_PROCESSOR_H
#define	FM_PROCESSOR_H

#include <QThread>
#include <QObject>
#include <sndfile.h>
#include "constants.h"
#include "fir-filters.h"
#include "fft-filters.h"
#include "trigtabs.h"
#include "pll.h"
#include "ringbuffer.h"
#include "rds-decoder.h"

class deviceHandler;
class RadioInterface;
class fmDemodulator;
class audioBase;
class newConverter;

class fmProcessor:public QThread {
Q_OBJECT

public:
    enum rdsDemodMode {
	FM_RDS_AUTO	= 0,
	FM_RDS_PILOT	= 1,
	FM_RDS_PILOTPLL	= 2,
	FM_RDS_PLL	= 3
    };
    enum fmMode {
	FM_STEREO	= 0,
	FM_MONO		= 1
    };

    fmProcessor(deviceHandler *, RadioInterface *,
	       int32_t,  // inputRate
	       int32_t,  // decimation
	       int32_t,  // workingRate
	       int32_t,  // audioRate,
	       int16_t); // threshold scanning
    ~fmProcessor(void);
    void stop(void);
    void setFMMode(bool);
    void setFMDecoder(int8_t);
    void setSoundBalance(int16_t);
    void setDeemphasis(int16_t);
    void setAudioGain(int16_t);
    void setSquelchValue(int16_t);
    void setAudioBandwidth(int32_t);
    void setBandwidth(int32_t);
    void setBandFilterDegree(int32_t);
    void setSignalGain(int16_t);
    void setFMRDSSelector(rdsDecoder::RdsMode);
    void setFMRDSDemod(rdsDemodMode);
    void resetRDS(void);
    void setSink(audioBase *);
    void startScan(void);
    void stopScan(void);
    void startFullScan(void);
    void stopFullScan(void);

private:
    virtual void run(void);

    deviceHandler *device;
    RadioInterface *radioInterface;
    int32_t fmRate;
    int32_t workingRate;
    int32_t decimatingScale;
    int32_t audioRate;
    int16_t threshold;
    bool initScan;
    bool scanning;
    bool running;

    int16_t signalGain;

    trigTabs *fastTrigTabs;
    common_fft *signalFft;
    DecimatingFIR *fmBandFilter;
    bool newFilter;
    int32_t fmBandwidth;
    int32_t fmFilterDegree;
    LowPassFIR *fmFilter;
    fftFilter *pilotBandFilter;
    pilotPll *pilotPllFilter;
    DSPFLOAT pilotDelay;
    fftFilter *rdsBandFilter;
    HilbertFilter *rdsHilbertFilter;
    pll *rdsPllDecoder;
    fftFilter *rdsLowPassFilter;

    uint8_t fmMode;
    uint8_t soundSelector;
    fmDemodulator *demodulator;

    bool initRDS;
    rdsDecoder *rdsDataDecoder;
    rdsDecoder::RdsMode rdsMode;
    rdsDemodMode rdsDemod;

    DSPFLOAT xkm1;
    DSPFLOAT ykm1;
    DSPFLOAT alpha;

    int32_t audioBandwidth;
    LowPassFIR *audioFilter;
    DSPFLOAT audioGain;
    DSPFLOAT baseAudioGain;
    int16_t squelchValue;
    DSPFLOAT leftChannel;
    DSPFLOAT rightChannel;
    DSPCOMPLEX *audioOut;
    newConverter *audioDecimator;
    newConverter *audioConverter;
    audioBase *audioSink;

signals:
    void showStrength(float);
    void showSoundMode(bool);
    void scanresult(void);
};
#endif
#ifndef __FM_DEMODULATOR
#define __FM_DEMODULATOR

#include "constants.h"
#include "pll.h"
#include "trigtabs.h"

#define PLL_PILOT_GAIN 3000

class fmDemodulator {
public:
    enum fm_demod {
        FM1DECODER = 0001,
        FM2DECODER = 0002,
        FM3DECODER = 0003,
        FM4DECODER = 0004,
        FM5DECODER = 0005,
        FM6DECODER = 0006
    };

private:
    int8_t selectedDecoder;
    DSPFLOAT max_freq_deviation;
    int32_t rateIn;
    DSPFLOAT fm_afc;
    DSPFLOAT fm_cvt;
    DSPFLOAT K_FM;
    pll* myfm_pll;
    trigTabs* fastTrigTabs;
    int32_t ArcsineSize;
    DSPFLOAT* Arcsine;
    DSPFLOAT Imin1;
    DSPFLOAT Qmin1;
    DSPFLOAT Imin2;
    DSPFLOAT Qmin2;

public:
    fmDemodulator(int32_t Rate_in,
        trigTabs* fastTrigTabs,
        DSPFLOAT K_FM);
    ~fmDemodulator(void);

    void setDecoder(int8_t);
    const char* nameOfDecoder(void);
    DSPFLOAT demodulate(DSPCOMPLEX);
    DSPFLOAT get_DcComponent(void);
};
#endif

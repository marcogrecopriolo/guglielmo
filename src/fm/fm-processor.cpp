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
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */
#include "fm-processor.h"
#include "radio.h"
#include "fm-demodulator.h"
#include "settings.h"
#include "constants.h"
#include "rds-decoder.h"
#include "audio-base.h"
#include "squelchClass.h"
#include "trigtabs.h"
#include "device-handler.h"
#include "newconverter.h"
#include "logging.h"

#define	AUDIO_MAX_PEAK		2.0
#define	PILOT_FREQUENCY		19000
#define PILOT_WIDTH             500
#define PILOT_MIN_THRESHOLD	6.0
#define PILOT_THRESHOLD		12.0
#define PILOT_SAMPLES		(fmRate*4)
#define	OMEGA_DEMOD		(2*M_PI/fmRate)
#define	OMEGA_PILOT		((DSPFLOAT (PILOT_FREQUENCY))/fmRate)*(2*M_PI)
#define	SIGNAL_FREQUENCY	3000
#define SIGNAL_WIDTH		2000
#define	RDS_FREQUENCY		(DSPFLOAT (3*PILOT_FREQUENCY))
#define RDS_WIDTH		2400
#define RDS_LP_WIDTH		1200		// as close to 1187.5 as we can get
#define RDS_PLL_WIDTH		200
#define RDS_SAMPLES		(fmRate*4)
#define RDS_SKIP		fmRate
#define RDS_MEAN_DRIFT_LIMIT	(2*M_PI/100.0)
#define RDS_AVG_DRIFT_LIMIT	(2*M_PI/300.0)
#define NOISE_FREQUENCY		70000
#define NOISE_WIDTH		500
#define	RDS_DECIMATOR		4
#define SIGNAL_SIZE		1024
#define BUFFER_SIZE		16384

#define DEF_SIGNAL_GAIN		100
#define DEF_AUDIO_GAIN		40
#define DEF_AUDIO_BANDWIDTH	-1
#define DEF_FILTER_DEGREE	15


fmProcessor::fmProcessor(deviceHandler *device, RadioInterface *radioInterface, int32_t inputRate,
			 int32_t fmRate, int32_t workingRate, int32_t audioRate, int16_t threshold) {
    running = false;
    initScan = false;
    scanning = false;
    this->device = device;
    this->radioInterface = radioInterface;
    this->fmRate = fmRate;
    this->decimatingScale = inputRate/fmRate;
    this->workingRate = workingRate;
    this->audioRate = audioRate;
    this->threshold = threshold;
    rdsMode = rdsDecoder::NO_RDS;
    rdsDemod = FM_RDS_AUTO;

    fastTrigTabs = new trigTabs(fmRate);
    signalFft = new common_fft(SIGNAL_SIZE);
    fmBandFilter = new DecimatingFIR(15*decimatingScale, fmRate/2, inputRate, decimatingScale);
    fmBandwidth = 0.95*fmRate;
    fmFilterDegree = DEF_FILTER_DEGREE;
    fmFilter = NULL;
    newFilter = false;

    // to isolate the pilot signal, we need a reasonable
    // filter. The filtered signal is beautified by a pll
    pilotBandFilter = new fftFilter(FFT_SIZE, PILOT_FILTER_SIZE);
    pilotBandFilter->setBand(PILOT_FREQUENCY-PILOT_WIDTH,
			     PILOT_FREQUENCY+PILOT_WIDTH, fmRate);
    pilotPllFilter = new pilotPll(OMEGA_PILOT, 25*OMEGA_DEMOD, fastTrigTabs);
    pilotDelay = (FFT_SIZE-PILOT_FILTER_SIZE)*OMEGA_PILOT;

    // highest freq in message
    DSPFLOAT F_G = 0.65*fmRate/2;
    DSPFLOAT Delta_F = 0.95*fmRate/2;
    DSPFLOAT B_FM = 2*(Delta_F+F_G);
    DSPFLOAT K_FM = B_FM*M_PI/F_G;
    fmDemodulator = new fm_Demodulator(fmRate, fastTrigTabs, K_FM);

    rdsDataDecoder = new rdsDecoder(radioInterface, fmRate / RDS_DECIMATOR, fastTrigTabs);
    rdsLowPassFilter = new fftFilter(FFT_SIZE, RDS_LOWPASS_SIZE);
    rdsLowPassFilter->setLowPass(RDS_LP_WIDTH, fmRate);
    rdsBandFilter = new fftFilter(FFT_SIZE, RDS_BAND_FILTER_SIZE);
    rdsBandFilter->setSimple(RDS_FREQUENCY-RDS_WIDTH, RDS_FREQUENCY+RDS_WIDTH, fmRate);
    rdsHilbertFilter = new HilbertFilter(HILBERT_SIZE, RDS_FREQUENCY/fmRate, fmRate);
    rdsPllDecoder = new pll(fmRate, RDS_FREQUENCY, RDS_FREQUENCY-50, RDS_FREQUENCY+50,
			    RDS_PLL_WIDTH, fastTrigTabs);

    // for the deemphasis we use an in-line filter with
    xkm1 = 0;
    ykm1 = 0;
    alpha = 1.0/(fmRate/(1000000.0/50.0+1));

    fmMode = FM_STEREO;
    leftChannel = 0.5;
    rightChannel = 0.5;
    signalGain = DEF_SIGNAL_GAIN;
    baseAudioGain = DEF_AUDIO_GAIN;
    squelchValue = 100;
    audioDecimator = new newConverter(fmRate, workingRate, workingRate/200);
    audioOut = new DSPCOMPLEX[audioDecimator->getOutputsize()];
    audioBandwidth = DEF_AUDIO_BANDWIDTH;
    audioFilter = NULL;
    if (audioRate != workingRate)
	audioConverter = new newConverter(workingRate, audioRate, workingRate/20);
    else
	audioConverter = NULL;

    connect(this, SIGNAL(showStrength(float)),
	    radioInterface, SLOT(showStrength(float)));
    connect(this, SIGNAL(showSoundMode(bool)),
	    radioInterface, SLOT(showSoundMode(bool)));
}

fmProcessor::~fmProcessor(void) {
    stop();
    disconnect(this, SIGNAL(showStrength(float)),
	    radioInterface, SLOT(showStrength(float)));
    disconnect(this, SIGNAL(showSoundMode(bool)),
	    radioInterface, SLOT(showSoundMode(bool)));
    delete fmBandFilter;
    delete signalFft;
    delete fmDemodulator;
    delete rdsPllDecoder;
    delete pilotPllFilter;
    delete rdsHilbertFilter;
    delete rdsBandFilter;
    delete rdsDataDecoder;
    delete pilotBandFilter;
    delete audioDecimator;
    delete fastTrigTabs;
    if (fmFilter != NULL)
	delete fmFilter;
    if (audioFilter != NULL)
	delete audioFilter;
}

void fmProcessor::stop(void) {
    if (running) {
	running	= false;
	while (!isFinished())
	    usleep(100);
    }
}

void fmProcessor::setSink(audioBase *audioSink) {
    this->audioSink = audioSink;
}

void fmProcessor::setSquelchValue(int16_t n) {
    squelchValue = n;
}

void fmProcessor::setSoundBalance(int16_t balance) {
    leftChannel	= -(balance-50.0)/100.0;
    rightChannel = (balance+50.0)/100.0;
}

void fmProcessor::setAudioGain(int16_t gain) {
    baseAudioGain = gain;
}

void fmProcessor::setBandwidth(int32_t b) {
    fmBandwidth	= b;
    newFilter = true;
}

void fmProcessor::setBandFilterDegree(int32_t d) {
    fmFilterDegree = d;
    newFilter = true;
}

void fmProcessor::setAudioBandwidth(int32_t bandwidth) {
    audioBandwidth = bandwidth;
}

void fmProcessor::setFMMode(bool m) {
    fmMode = m? FM_STEREO: FM_MONO;
}

void fmProcessor::setFMDecoder(int8_t d) {
    fmDemodulator->setDecoder(d);
}

// Deemphasis	= 50 usec (3183 Hz, Europe)
// Deemphasis	= 75 usec (2122 Hz US)
// tau		= 2 * M_PI * Freq = 1000000 / time
void fmProcessor::setDeemphasis(int16_t v) {
    DSPFLOAT Tau;

    switch (v) {
    default:
	v = 1;
	/* fallthrough */
    case 1:
    case 50:
    case 75:
	Tau = 1000000.0/v;
	alpha = 1.0/(DSPFLOAT(fmRate)/Tau+1.0);
    }
}

void fmProcessor::setSignalGain(int16_t g) {
    signalGain = g;
}

void fmProcessor::startScan(void) {
    initScan = true;
    connect(this, SIGNAL(scanresult(void)),
	    radioInterface, SLOT(scanDone(void)));
}

void fmProcessor::startFullScan(void) {
    initScan = true;
    connect(this, SIGNAL(scanresult(void)),
	    radioInterface, SLOT(scanFound(void)));
}

void fmProcessor::stopScan(void) {
    scanning = false;
    disconnect(this, SIGNAL(scanresult(void)),
	    radioInterface, SLOT(scanDone(void)));
}

void fmProcessor::stopFullScan(void) {
    scanning = false;
    disconnect(this, SIGNAL(scanresult(void)),
	    radioInterface, SLOT(scanFound(void)));
}

void fmProcessor::setFMRDSSelector(rdsDecoder::RdsMode m) {
    rdsMode = m;
}

void fmProcessor::setFMRDSDemod(rdsDemodMode m) {
    rdsDemod = m;
    initRDS = true;
}

void fmProcessor::resetRDS(void) {
    initRDS = true;
    if (rdsDataDecoder == NULL)
	return;
    rdsPllDecoder->reset();
    rdsDataDecoder->reset();
}

static
DSPFLOAT getLevel(DSPCOMPLEX *v, int min, int max) {
    DSPFLOAT sum = 0;
    int32_t i;

    for (i = min; i < max; i++)
	sum += abs(v[i]);
    return sum/(max-min);
}

// In this variant, we have a separate thread for the fm processing
void fmProcessor::run(void) {
    DSPCOMPLEX result;
    DSPFLOAT rdsPhase = 0;
    DSPCOMPLEX dataBuffer[BUFFER_SIZE];
    int32_t signalPointer = 0;
    int32_t peakLevelCount = 0;
    DSPFLOAT peakLevel = -100;
    int32_t audioGainCount = 0;
    DSPFLOAT audioGainAverage = 0;
    DSPCOMPLEX *signalBuffer = signalFft->getVector();
    DSPFLOAT binWidth = fmRate/SIGNAL_SIZE;
    int signalMinBin = int((SIGNAL_FREQUENCY-SIGNAL_WIDTH)/binWidth);
    int signalMaxBin = int((SIGNAL_FREQUENCY+SIGNAL_WIDTH)/binWidth);
    int pilotMinBin = int((PILOT_FREQUENCY-PILOT_WIDTH)/binWidth);
    int pilotMaxBin = int((PILOT_FREQUENCY+PILOT_WIDTH)/binWidth);
    int noiseMinBin = int((NOISE_FREQUENCY-NOISE_WIDTH)/binWidth);
    int noiseMaxBin = int((NOISE_FREQUENCY+NOISE_WIDTH)/binWidth);
    DSPFLOAT totPilotSnr = 0.0;
    int snrCount = 0;
    int signalCount = 0;
    int stereoCount = 0;
    int rdsCount = 0;
    DSPFLOAT totDrift = 0.0;
    DSPFLOAT minDrift = 2 *M_PI;
    DSPFLOAT maxDrift = 0.0;
    int driftCount = 0;
    bool noPilot = true;
    bool pilot = false;
    squelch squelchControl(1, workingRate/10, workingRate/20, workingRate);
    int16_t oldSquelchValue = -1;
    bool squelchOn = (squelchValue < 100);
    int audioAmount;
    int32_t oldAudioBandwidth = audioBandwidth;

    initRDS = true;
    running = true;
    scanning = false;
    audioGain = 0;
    audioDecimator->reset();
    if (audioConverter != NULL)
	audioConverter->reset();
    while (running) {
	while (running && device->Samples() < BUFFER_SIZE)
	    sleep(1);
	   
	if (!running)
	    break;

	// process any setting changes
	if (newFilter) {
	    if (fmFilter != NULL) {
		delete fmFilter;
		fmFilter = NULL;
	    }
	    if (fmBandwidth > 0)
		fmFilter = new LowPassFIR (fmFilterDegree, fmBandwidth/2, fmRate);
	    newFilter = false;
	}

	if (audioBandwidth != oldAudioBandwidth) {
	    if (audioFilter != NULL) {
		delete audioFilter;
		audioFilter = NULL;
	    }
	    if (audioBandwidth > 0)
		audioFilter = new LowPassFIR (AUDIO_FILTER_SIZE, audioBandwidth, fmRate);
	    oldAudioBandwidth= audioBandwidth;
	}
	if (squelchValue != oldSquelchValue) {
	    squelchControl.setSquelchLevel(squelchValue);
	    oldSquelchValue = squelchValue;
	    squelchOn = (squelchValue < 100);
	}

	if (initRDS) {
	    if (rdsDemod == FM_RDS_AUTO) {
		snrCount = PILOT_SAMPLES;
		totPilotSnr = 0.0;
		signalPointer = 0;
		driftCount = RDS_SAMPLES;
		totDrift = 0.0;
		maxDrift = 0.0;
		minDrift = 2 * M_PI;
		noPilot = true;
		pilot = false;
	    } else {
		 noPilot = (rdsDemod == FM_RDS_PLL);
		 pilot = (rdsDemod == FM_RDS_PILOT);
		 driftCount = 0;
		 snrCount = 0;
	    }
	    initRDS = false;
	}

	// collect samples and process
	agcStats stats;
	int32_t amount = device -> getSamples(dataBuffer, BUFFER_SIZE, &stats);
	radioInterface -> processGain(&stats, amount);
	for (int i = 0; i < amount; i ++) {
	    DSPCOMPLEX v = cmul(dataBuffer[i], signalGain);

	    // decimate if necessary
	    if ((decimatingScale > 1) && !fmBandFilter->Pass(v, &v))
		continue;
	        
	    // if scanning, just look for suitable signal
	    if (initScan) {
		signalPointer = 0;
		scanning = true;
		initScan = false;
	    }

	    if (scanning) {
	        signalBuffer[signalPointer ++] = v;
	        if (signalPointer >= SIGNAL_SIZE) {
		    signalPointer = 0;
		    signalFft->do_FFT();
		    DSPFLOAT signal = getLevel(signalBuffer, signalMinBin, signalMaxBin);
		    DSPFLOAT noise = getLevel(signalBuffer, noiseMinBin, noiseMaxBin);
		    if (get_db(signal, 256)-get_db(noise, 256) > threshold) {
		        log(LOG_FM, LOG_MIN, "signal found %f %f",
		                get_db(signal, 256), get_db(noise, 256));
		        scanning = false;
			scanresult();
	            }
	        }
		continue;
	    }

	    // filter unprocessed signal
	    if (fmFilter != NULL)
		v = fmFilter->Pass(v);

	    //	keep track of peak level for audio gain and signal strength
	    if (abs(v) > peakLevel)
		peakLevel = abs(v);
	    if (++peakLevelCount >= fmRate/4) {
		DSPFLOAT tmpGain = 0;

		if (peakLevel > 0)
		    tmpGain = AUDIO_MAX_PEAK/peakLevel;
		if (tmpGain <= 0.1)
		    tmpGain = 0.1;

		// avoid compiler warning on undefined autoincrement value
		audioGainAverage *= audioGainCount;
		audioGainCount++;
		audioGain = (audioGainAverage+tmpGain)/audioGainCount;
		audioGainAverage = audioGain;
		peakLevelCount = 0;
		peakLevel = -DEF_SIGNAL_GAIN;
	    }

	    // check pilot for rds phase
	    if (snrCount > 0) {
		snrCount--;
		signalBuffer [signalPointer ++] = v;
		if (signalPointer >= SIGNAL_SIZE) {
		    signalPointer	= 0;
		    signalFft -> do_FFT ();
	            DSPFLOAT noise	= getLevel (signalBuffer, noiseMinBin, noiseMaxBin);
	            DSPFLOAT pilot	= getLevel (signalBuffer, pilotMinBin, pilotMaxBin);
		    DSPFLOAT pilotDb = get_db(pilot, 256) - get_db(noise, 256);
		    totPilotSnr += pilotDb;
		    if (pilotDb >= PILOT_THRESHOLD) {
			noPilot = false;
			log(LOG_FM, LOG_MIN, "pilot %f usePilot %i", pilotDb, !noPilot);
			snrCount = 0;
		    } else if (snrCount == 0) {
			DSPFLOAT avgPilotSnr = totPilotSnr / PILOT_SAMPLES;
			noPilot = (avgPilotSnr < PILOT_MIN_THRESHOLD);
			log(LOG_FM, LOG_MIN, "pilot %f snr  %f usePilot %i", pilotDb, avgPilotSnr, !noPilot);
		    }
		}
	    }

	    // demodulate and output
	    DSPFLOAT demod = fmDemodulator->demodulate(v);
	    if (fmMode == FM_STEREO) {
		DSPFLOAT currentPilotPhase = pilotPllFilter->doPll(5*pilotBandFilter->Pass(5*demod));
		DSPFLOAT phaseForLRDiff	= 2*(currentPilotPhase+pilotDelay);
		DSPFLOAT LRDiff	= fastTrigTabs->getCos(phaseForLRDiff)*demod;
		rdsPhase = 3*(currentPilotPhase+pilotDelay);
		xkm1 = (demod-xkm1)*alpha+xkm1;
		ykm1 = (LRDiff-ykm1)*alpha+ykm1;
		result = DSPCOMPLEX(xkm1+ykm1, xkm1-ykm1);
		if (ykm1 > 0)
		    stereoCount++;
	    } else {
		xkm1 = (demod-xkm1)*alpha+xkm1;
		ykm1 = (demod-ykm1)*alpha+ykm1;
		result = DSPCOMPLEX(xkm1, ykm1);
	    }

	    // audio gain correction
 	    result = cmul(result, audioGain*baseAudioGain);
	    if (audioFilter != NULL)
		 result = audioFilter -> Pass (result);
	    DSPCOMPLEX out = DSPCOMPLEX(leftChannel*real(result),
					rightChannel*imag(result));

	    if (audioDecimator->convert(out, audioOut, &audioAmount)) {
		DSPCOMPLEX pcmSample;

		for (int k = 0; k < audioAmount; k ++) {
		    if (squelchOn)
			pcmSample = squelchControl.do_squelch(audioOut[k]);
		    else
			pcmSample = audioOut [k];
		    if (audioRate == workingRate)
			audioSink->putSample(pcmSample);
		    else {
			_VLA(DSPCOMPLEX, out, audioConverter->getOutputsize());
			int32_t amount;

			if (audioConverter->convert(pcmSample, out, &amount))
			    audioSink->putSamples(out, amount);
		    }
		}
	    }

	    // demod RDS
	    if ((rdsMode != rdsDecoder::NO_RDS)) {
		DSPFLOAT rdsData;

		// if there's no pilot we band pass the RDS signal
		// beautify with a Hilbert filter, PLL and low pass
		if (noPilot) {
		    DSPCOMPLEX rdsBase = rdsBandFilter->Pass(DSPCOMPLEX(demod, demod));
		    rdsBase = rdsHilbertFilter->Pass(rdsBase);
		    DSPFLOAT rdsDelay = rdsPllDecoder->doPll(rdsBase);
		    rdsData = rdsLowPassFilter->Pass(rdsDelay*demod);

		// if there's a pilot and the phase doesn't shift, we use it
		} else if (pilot) {
		    DSPFLOAT mixerValue	= fastTrigTabs->getSin(rdsPhase);
		    rdsData = rdsLowPassFilter->Pass(mixerValue*demod);

		// otherwise we PLL using the pilot phase
		} else {
		    DSPCOMPLEX rdsBase = rdsBandFilter->Pass(DSPCOMPLEX(demod, demod));
		    rdsBase = rdsHilbertFilter->Pass(rdsBase);
		    rdsPhase = toBaseRadians(rdsPhase);
		    DSPFLOAT rdsDelay = rdsPllDecoder->doPll(rdsBase, rdsPhase);
		    rdsData = rdsLowPassFilter->Pass(rdsDelay*demod);

		    // if in auto mode, let the PLL phase settle, and check that it converges after a while
		    // if it does, we can just move to the pilot phase, and skip the extra load
		    if (driftCount-- > 0 && driftCount <= (RDS_SAMPLES-RDS_SKIP)) {
			DSPFLOAT drift = toBaseRadians(rdsPhase-rdsPllDecoder->getNcoPhase());
			totDrift += drift;
			if (drift < minDrift)
			    minDrift = drift;
			else if (drift > maxDrift)
			    maxDrift = drift;
			if (driftCount == 0) {
			   DSPFLOAT avgDrift = totDrift/(RDS_SAMPLES-RDS_SKIP);
			   DSPFLOAT meanDrift = (maxDrift+minDrift)/2;
			   DSPFLOAT meanDriftDiff = (maxDrift-minDrift)/2;
			   DSPFLOAT avgDriftDiff = abs(avgDrift-meanDrift);

			   // we check the max width and the distribution of drifts
			   pilot = (meanDriftDiff < RDS_MEAN_DRIFT_LIMIT && avgDriftDiff < RDS_AVG_DRIFT_LIMIT); 
			   log(LOG_FM, LOG_MIN, "pll drift avg %f%% mean %f%% pilot %i", 100*avgDriftDiff/2/M_PI, 100*meanDriftDiff/2/M_PI, pilot);
			}
		    }
		}

		if (++rdsCount >= RDS_DECIMATOR) {
		    DSPFLOAT mag;

	            rdsDataDecoder -> doDecode (rdsData, &mag, rdsMode);
	            rdsCount = 0;
	         }
	    }

	    if (++signalCount > fmRate) {
		signalCount = 0;

		// in reality the peak level is as good a signal measurement as getting the signal
		// after a FFT on the samples buffer, much like we did for rds and noise
	        showStrength (2*(get_db(peakLevel, 128)-get_db(0, 128)));
		showSoundMode(stereoCount > 0);
		stereoCount = 0;
		log(LOG_FM, LOG_CHATTY, "signal strength %f audio gain %f", peakLevel, audioGain);
	    }
	}
    }
}

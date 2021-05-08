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
#include	"fm-processor.h"
#include	"radio.h"
#include	"fm-demodulator.h"
#include	"constants.h"
#include	"rds-decoder.h"
#include	"audio-base.h"
#include	"squelchClass.h"
#include	"sincos.h"
#include	"device-handler.h"
#include	"newconverter.h"

#define	AUDIO_FREQ_DEV_PROPORTION 0.85f
#define	PILOT_FREQUENCY		19000
#define PILOT_WIDTH             500
#define	OMEGA_DEMOD		2 * M_PI / fmRate
#define	OMEGA_PILOT		((DSPFLOAT (PILOT_FREQUENCY)) / fmRate) * (2 * M_PI)
#define	RDS_FREQUENCY		(3 * PILOT_FREQUENCY)
#define RDS_WIDTH               2400
#define RDS_LP_WIDTH            1500
#define	RDS_DECIMATOR		4

//
//	Note that no decimation done as yet: the samplestream is still
//	full speed
	fmProcessor::fmProcessor (deviceHandler		*vi,
	                          RadioInterface	*RI,
	                          int32_t		inputRate,
	                          int32_t		fmRate,
	                          int32_t		workingRate,
	                          int32_t		audioRate,
	                          int16_t		filterDepth,
	                          int16_t		thresHold) {
	running				= false;
	this	-> myRig		= vi;
	this	-> myRadioInterface	= RI;
	this	-> inputRate		= inputRate;
	this	-> fmRate		= fmRate;
	this	-> decimatingScale	= inputRate / fmRate;
	this	-> workingRate		= workingRate;
	this	-> audioRate		= audioRate;
	this	-> filterDepth		= filterDepth;
	this	-> thresHold		= thresHold;
	this	-> rdsModus		= rdsDecoder::NO_RDS;
	this	-> rdsDemod		= FM_RDS_NOPILOT;
	this	-> squelchOn		= false;
	this	-> scanning		= false;
	Lgain				= 20;
	Rgain				= 20;

	myRdsDecoder			= NULL;

	this	-> localOscillator	= new Oscillator (inputRate);
	this	-> mySinCos		= new SinCos (fmRate);
	this	-> lo_frequency		= 0;
	this	-> fmBandwidth		= 0.95 * fmRate;
	this	-> fmFilterDegree	= 21;
	this	-> fmFilter		= new LowPassFIR (this -> fmFilterDegree,
	                                                  0.95 * fmRate / 2,
	                                                  fmRate);
	this	-> newFilter		= false;
/*
 *	default values, will be set through the user interface
 *	to their appropriate values
 */
	this	-> fmModus		= FM_STEREO;
	this	-> selector		= S_STEREO;
	this	-> balance		= 0;
	this	-> leftChannel		= - (balance - 50.0) / 100.0;
	this	-> rightChannel		= (balance + 50.0) / 100.0;
	this	-> Volume		= 40.0;
	this	-> inputMode		= IandQ;
	this	-> audioDecimator	=
	                         new newConverter (fmRate,
	                                           workingRate,
	                                           workingRate / 200);
	this	-> audioOut		=	
	                         new DSPCOMPLEX [audioDecimator -> getOutputsize ()];
/*
 *	averagePeakLevel and audioGain are set
 *	prior to calling the processFM method
 */
	this	-> peakLevel		= -100;
	this	-> peakLevelcnt		= 0;
	this	-> max_freq_deviation	= 0.95 * (0.5 * fmRate);
	this	-> norm_freq_deviation	= 0.6 * max_freq_deviation;
	this	-> audioGain		= 0;
	this	-> audioGainAverage	= 0;
	this	-> audioGainCnt		= 0;
//
//	Since data is coming with a pretty high rate, we need to filter
//	and decimate in an efficient way. We have an optimized
//	decimating filter (optimized or not, it takes quite some
//	cycles when entering with high rates)
	fmBandfilter		= new DecimatingFIR (15 * decimatingScale,
	                                             fmRate / 2,
	                                             inputRate,
	                                             decimatingScale);

//	to isolate the pilot signal, we need a reasonable
//	filter. The filtered signal is beautified by a pll
	pilotBandFilter		= new fftFilter (FFT_SIZE, PILOTFILTER_SIZE);
	pilotBandFilter		-> setBand (PILOT_FREQUENCY - PILOT_WIDTH,
		                            PILOT_FREQUENCY + PILOT_WIDTH,
		                            fmRate);
	pilotRecover		= new pilotRecovery (fmRate,
	                                             OMEGA_PILOT,
	                                             25 * OMEGA_DEMOD,
	                                             mySinCos);
	pilotDelay		= (FFT_SIZE - PILOTFILTER_SIZE) * OMEGA_PILOT;
	rdsLowPassFilter	= new fftFilter (FFT_SIZE, RDSLOWPASS_SIZE);
	rdsLowPassFilter	-> setLowPass (RDS_LP_WIDTH, fmRate);
//
//	the constant K_FM is still subject to many questions
	DSPFLOAT	F_G	= 0.65 * fmRate / 2; // highest freq in message
	DSPFLOAT	Delta_F	= 0.95 * fmRate / 2;	//
	DSPFLOAT	B_FM	= 2 * (Delta_F + F_G);
	K_FM			= B_FM * M_PI / F_G;
	TheDemodulator		= new fm_Demodulator (fmRate,
	                                              mySinCos, K_FM);
	fmAudioFilter		= NULL;
//
//	In the case of mono we do not assume a pilot
//	to be available. We borrow the approach from CuteSDR
	rdsBandFilter		= new fftFilter (FFT_SIZE,
	                                         RDSBANDFILTER_SIZE);
	rdsBandFilter		-> setSimple (RDS_FREQUENCY - RDS_WIDTH,
	                                      RDS_FREQUENCY + RDS_WIDTH,
	                                      fmRate);
	rdsHilbertFilter	= new HilbertFilter (HILBERT_SIZE,
						     (DSPFLOAT)RDS_FREQUENCY / fmRate,
	                                             fmRate);
	rds_plldecoder		= new pllC (fmRate,
	                                    RDS_FREQUENCY,
	                                    RDS_FREQUENCY - 50,
	                                    RDS_FREQUENCY + 50,
	                                    200,
	                                    mySinCos);

//	for the deemphasis we use an in-line filter with
	xkm1			= 0;
	ykm1			= 0;
	alpha			= 1.0 / (fmRate / (1000000.0 / 50.0 + 1));
	dumping			= false;
	dumpFile		= NULL;

	connect (this, SIGNAL (showStrength (float)),
	         myRadioInterface, SLOT (showStrength (float)));
	connect (this, SIGNAL (showSoundMode (bool)),
	         myRadioInterface, SLOT (showSoundMode (bool)));
	connect (this, SIGNAL (scanresult (void)),
	         myRadioInterface, SLOT (scanDone (void)));
	squelchValue		= 0;
	old_squelchValue	= 0;

	theConverter		= NULL;
	if (audioRate != workingRate) 
	   theConverter	= new newConverter (workingRate, audioRate,
	                                    workingRate / 20);
	myCount			= 0;
	stereoCount		= 0;
}

	fmProcessor::~fmProcessor (void) {
	stop	();

//	delete	TheDemodulator;
//	delete	rds_plldecoder;
//	delete	pilotRecover;
//	delete	rdsHilbertFilter;
//	delete	rdsBandFilter;
//	delete	pilotBandFilter;
//	delete	audioDecimator;
//	delete	mySinCos;
	if (fmAudioFilter != NULL)
	   delete fmAudioFilter;
}

void	fmProcessor::stop	(void) {
	if (running) {
	   running	= false;
	   while (!isFinished ())
	      usleep (100);
	}
}

void	fmProcessor::setSink	(audioBase *mySink) {
	this	-> theSink		= mySink;
}

void	fmProcessor::set_squelchValue	(int16_t n) {
	squelchValue	= n;
}

const char  *fmProcessor::nameofDecoder	(void) {
//	if (running)
	   return TheDemodulator -> nameofDecoder ();
	return " ";
}
//
//	changing a filter is in two steps: here we set a marker,
//	but the actual filter is created in the mainloop of
//	the processor
void	fmProcessor::setBandwidth	(int32_t b) {
	fmBandwidth	= b;
	newFilter	= true;
}

void	fmProcessor::setBandfilterDegree	(int32_t d) {
	fmFilterDegree	= d;
	newFilter	= true;
}

void	fmProcessor::setfmMode (uint8_t m) {
	fmModus	= m ? FM_STEREO : FM_MONO;
}

void	fmProcessor::setFMdecoder (int8_t d) {
	if (running)
	   TheDemodulator	-> setDecoder (d);
}

void	fmProcessor::setSoundMode (uint8_t selector) {
	this	-> selector = selector;
}

void	fmProcessor::setSoundBalance (int16_t balance) {
	this	-> balance = balance;
	leftChannel	= - (balance - 50.0) / 100.0;
	rightChannel	= (balance + 50.0) / 100.0;
}

//	Deemphasis	= 50 usec (3183 Hz, Europe)
//	Deemphasis	= 75 usec (2122 Hz US)
//	tau		= 2 * M_PI * Freq = 1000000 / time
void	fmProcessor::setDeemphasis	(int16_t v) {
DSPFLOAT	Tau;
	switch (v) {
	   default:
	      v	= 1;
	   case	1:
	   case 50:
	   case 75:
//	pass the Tau
	      Tau	= 1000000.0 / v;
	      alpha	= 1.0 / (DSPFLOAT (fmRate) / Tau + 1.0);
	}
}

void	fmProcessor::setVolume (int16_t Vol) {
	Volume = Vol;
}

DSPCOMPLEX	fmProcessor::audioGainCorrection (DSPCOMPLEX z) {
	return cmul (z, audioGain * Volume);
	return z;
}

void	fmProcessor::startDumping	(SNDFILE *f) {
	if (dumping)
	   return;
//	do not change the order here, another thread might get confused
	dumpFile 	= f;
	dumping		= true;
}

void	fmProcessor::stopDumping	(void) {
	dumping = false;
}

void	fmProcessor::setAttenuation (int16_t l, int16_t r) {
	Lgain	= l;
	Rgain	= r;
}

void	fmProcessor::startScanning	(void) {
	scanning	= true;
}

void	fmProcessor::stopScanning	(void) {
	scanning	= false;
}

//
//	In this variant, we have a separate thread for the
//	fm processing

void	fmProcessor::run (void) {
DSPCOMPLEX	result;
DSPFLOAT 	rdsData;
DSPFLOAT	PhaseforRds;
int		rdsCnt = 0;
int32_t		bufferSize	= 2 * 8192;
DSPCOMPLEX	dataBuffer [bufferSize];
int32_t		i, k;
DSPCOMPLEX	out;
DSPCOMPLEX	pcmSample;
int32_t		amount;
squelch		mySquelch (1, workingRate / 10, workingRate / 20, workingRate); 
int32_t		audioAmount;
int32_t		signalPointer	= 0;
common_fft	*signal_fft  	= new common_fft (1024);
DSPCOMPLEX	*signalBuffer	= signal_fft -> getVector ();
	myRdsDecoder		= new rdsDecoder (myRadioInterface,
	                                                  fmRate / RDS_DECIMATOR,
	                                                  mySinCos);

	running	= true;		// will be set from the outside
	while (running) {
	   while (running && (myRig -> Samples () < bufferSize)) {
	      msleep (1);	// should be enough
	   }
	   
	   if (!running)
	      break;

//	First: update according to potentially changed settings
	   if (newFilter && (fmBandwidth < 0.95 * fmRate)) {
	      delete fmFilter;
	      fmFilter	= new LowPassFIR (fmFilterDegree,
	                                  fmBandwidth / 2,
	                                  fmRate);
	   }
	   newFilter = false;

	   if (squelchValue != old_squelchValue) {
	      mySquelch. setSquelchLevel (squelchValue);
	      old_squelchValue = squelchValue;
	   }

	   amount = myRig -> getSamples (dataBuffer, bufferSize);
//
	   if (dumping) {
	      float dumpBuffer [2 * amount];
	      for (i = 0; i < amount; i ++) {
	         dumpBuffer [2 * i] = real (dataBuffer [i]);
	         dumpBuffer [2 * i + 1] = imag (dataBuffer [i]);
	      }
	      sf_writef_float (dumpFile, dumpBuffer, amount);
	   }
//	Here we really start

//	We assume that if/when the pilot is no more than 3 db's above
//	the noise around it, it is better to decode mono
	   for (i = 0; i < amount; i ++) {
	      DSPCOMPLEX v = 
	          DSPCOMPLEX (real (dataBuffer [i]) * Lgain,
	                      imag (dataBuffer [i]) * Rgain);
	      v	= v * localOscillator -> nextValue (lo_frequency);
//
//	first step: decimating (and filtering)
	      if ((decimatingScale > 1) && !fmBandfilter -> Pass (v, &v))
	         continue;
	        
//	second step: if we are scanning, do the scan
	      if (scanning) {
	         signalBuffer [signalPointer ++] = v;
	         if (signalPointer >= 1024) {
	            signalPointer	= 0;
	            signal_fft -> do_FFT ();
	            float signal	= getSignal	(signalBuffer, 1024);
	            float Noise		= getNoise	(signalBuffer, 1024);
	            if (get_db (signal, 256) - get_db (Noise, 256) > 
	                               this -> thresHold) {
	               fprintf (stderr, "signal found %f %f\n",
	                        get_db (signal, 256), get_db (Noise, 256));
	               scanresult ();
	            }
	         }
	         continue;	// no signal processing!!!!
	      }

//	third step: if requested, apply filtering
	      if (fmBandwidth < 0.95 * fmRate)
	         v	= fmFilter	-> Pass (v);
//	Now we have the signal ready for decoding
//	keep track of the peaklevel, we take segments
	      if (abs (v) > peakLevel)
	         peakLevel = abs (v);
	      if (++peakLevelcnt >= fmRate / 4) {
		 DSPFLOAT	tmpGain = 0;
	         DSPFLOAT	ratio	= 
	                          max_freq_deviation / norm_freq_deviation;
	         if (peakLevel > 0)
	            tmpGain	=
	                  (ratio / peakLevel) / AUDIO_FREQ_DEV_PROPORTION;
	         if (tmpGain <= 0.1)
	            tmpGain = 0.1;

		 // avoid compiler warning on undefined autoincrement value
		 audioGainAverage *= audioGainCnt;
		 audioGainCnt++;
	         audioGain	= (audioGainAverage + tmpGain) / audioGainCnt;
	         audioGainAverage = audioGain;
	         peakLevelcnt	= 0;
	         peakLevel	= -100;
	      }

	      DSPFLOAT demod	= TheDemodulator  -> demodulate (v);

	      if ((fmModus == FM_STEREO)) {
	         stereo (demod, &result, &PhaseforRds);
		 if (imag (result) > 0)
			stereoCount++;
	         switch (selector) {
	            default:
	            case S_STEREO:
	               result = DSPCOMPLEX (real (result) + imag (result),
	                                  - (- real (result) + imag (result)));
	               break;

	            case S_LEFT:
	               result = DSPCOMPLEX (real (result) + imag (result), 0);
	               break;

	            case S_RIGHT:
	               result = DSPCOMPLEX (0, - (imag (result) - real (result)));
	               break;

	            case S_LEFTplusRIGHT:
	               result = DSPCOMPLEX (real (result),  real (result));
	               break;

	            case S_LEFTminusRIGHT:
	               result = DSPCOMPLEX (imag (result), imag (result));
	               break;
	         }
	      }
	      else
	         mono (demod, &result);
//
//	"result" now contains the audio sample, either stereo
//	or mono
	      result = audioGainCorrection (result);
	      if (fmAudioFilter != NULL)
	         result = fmAudioFilter -> Pass (result);
	      out  = DSPCOMPLEX (leftChannel * real (result),
	                         rightChannel * imag (result));

	      if (audioDecimator -> convert (out, audioOut, &audioAmount)) {
	         for (k = 0; k < audioAmount; k ++) {
	            if (squelchOn)
	               pcmSample = mySquelch. do_squelch (audioOut [k]);
	            else
	               pcmSample = audioOut [k];
	            sendSampletoOutput (pcmSample);
	         }
	      }

	      if ((rdsModus != rdsDecoder::NO_RDS)) {
		 if (fmModus != FM_STEREO || rdsDemod == FM_RDS_NOPILOT ) {

		    // fully inspired by cuteSDR, we try to decode the rds stream
		    // by simply am decoding it
		    DSPCOMPLEX rdsBase	= DSPCOMPLEX (5 * demod, 5 * demod);
		    rdsBase		= rdsBandFilter -> Pass (rdsBase);
		    rdsBase		= rdsHilbertFilter -> Pass (rdsBase);
		    rds_plldecoder	-> do_pll (rdsBase);
		    DSPFLOAT rdsDelay	= real (rds_plldecoder -> getDelay ());
		    rdsData		= rdsLowPassFilter -> Pass (5 * rdsDelay);
		 } else {
		    DSPFLOAT MixerValue	= mySinCos -> getCos (PhaseforRds);
		    rdsData		= 5 * rdsLowPassFilter -> Pass (MixerValue * demod);
		 }

	         if (++rdsCnt >= RDS_DECIMATOR) {
		    DSPFLOAT mag;

	            myRdsDecoder -> doDecode (rdsData, &mag, rdsModus);
	            rdsCnt = 0;
	         }
	      }

/*	      // accumulate data for signal measurement
	      signalBuffer [signalPointer ++] = v;
	      if (signalPointer >= 1024)
	         signalPointer	= 0;
*/
	      if (++myCount > fmRate) {
	         myCount = 0;

		 // in reality the peak level is as good a signal measurement as getting the signal 
		 // after a FFT on the samples buffer
/*	         signal_fft -> do_FFT ();
	         float signal	= getSignal (signalBuffer, 1024);
	         showStrength (2 * (get_db(signal, 128) - get_db (0, 128)));
*/
	         showStrength (2 * (get_db(peakLevel, 128) - get_db (0, 128)));
		 showSoundMode (stereoCount > 0);
	         stereoCount = 0;
	      }
	   }
	}
}


void	fmProcessor::mono (float	demod,
	                   DSPCOMPLEX	*audioOut) {
DSPFLOAT	Re, Im;

//	deemphasize
	Re	= xkm1 = (demod - xkm1) * alpha + xkm1;
	Im	= ykm1 = (demod - ykm1) * alpha + ykm1;
	*audioOut	= DSPCOMPLEX (Re, Im);
}

void	fmProcessor::stereo (float	demod,
	                     DSPCOMPLEX	*audioOut,
	                     DSPFLOAT	*PhaseforRds) {

DSPFLOAT	LRPlus	= 0;
DSPFLOAT	LRDiff	= 0;
DSPFLOAT	pilot	= 0;
DSPFLOAT	currentPilotPhase;
DSPFLOAT	PhaseforLRDiff	= 0;
/*
 */
	LRPlus = LRDiff = pilot	= demod;
/*
 *	get the phase for the "carrier to be inserted" right
 */
	pilot		= pilotBandFilter -> Pass (5 * pilot);
	currentPilotPhase = pilotRecover -> getPilotPhase (5 * pilot);
/*
 *	Now we have the right - i.e. synchronized - signal to work with
 */
	PhaseforLRDiff	= 2 * (currentPilotPhase + pilotDelay);
//
//	Due to filtering the real amplitude of the LRDiff might have
//	to be adjusted, we guess
	LRDiff	= 2.0 * mySinCos	-> getCos (PhaseforLRDiff) * LRDiff;

	*PhaseforRds	= 3 * (currentPilotPhase + pilotDelay);

//	apply deemphasis
	LRPlus		= xkm1	= (LRPlus - xkm1) * alpha + xkm1;
	LRDiff		= ykm1	= (LRDiff - ykm1) * alpha + ykm1;
        *audioOut		= DSPCOMPLEX (LRPlus, LRDiff);
}
//
//	Since setLFcutoff is only called from within the "run"  function
//	from where the filter is also called, it is safe to remove
//	it here
//	
void	fmProcessor::setLFcutoff (int32_t Hz) {
	if (fmAudioFilter != NULL)
	   delete	fmAudioFilter;
	fmAudioFilter	= NULL;
	if (Hz > 0)
	   fmAudioFilter	= new LowPassFIR (11, Hz, fmRate);
}

void	fmProcessor::sendSampletoOutput (DSPCOMPLEX s) {

	if (audioRate == workingRate)
	   theSink	-> putSample (s);
	else {
	   DSPCOMPLEX out [theConverter -> getOutputsize ()];
	   int32_t amount;
	   if (theConverter -> convert (s, out, &amount))
	         theSink -> putSamples (out, amount);
	}
}

void	fmProcessor::setfmRdsSelector (rdsDecoder::RdsMode m) {
	rdsModus	= m;
}

void	fmProcessor::setfmRdsDemod (RdsDemod m) {
	rdsDemod	= m;
}

void	fmProcessor::resetRds	(void) {
	if (myRdsDecoder == NULL)
	   return;
	myRdsDecoder	-> reset ();
}

void	fmProcessor::set_localOscillator (int32_t lo) {
	lo_frequency = lo;
}

bool	fmProcessor::ok		(void) {
	return running;
}

void	fmProcessor::set_squelchMode (bool b) {
	squelchOn	= b;
}

void	fmProcessor::setInputMode	(uint8_t m) {
	inputMode	= m;
}

DSPFLOAT	fmProcessor::getSignal	(DSPCOMPLEX *v, int32_t size) {
DSPFLOAT sum = 0;
int16_t	i;

	for (i = 5; i < 25; i ++)
	   sum += abs (v [i]);
	for (i = 5; i < 25; i ++)
	   sum += abs (v [size - 1 - i]);
	return sum / 40;
}

DSPFLOAT	fmProcessor::getNoise	(DSPCOMPLEX *v, int32_t size) {
DSPFLOAT sum	= 0;
int16_t	i;

	for (i = 5; i < 25; i ++)
	   sum += abs (v [size / 2 - 1 - i]);
	for (i = 5; i < 25; i ++)
	   sum += abs (v [size / 2 + 1 + i]);
	return sum / 40;
}

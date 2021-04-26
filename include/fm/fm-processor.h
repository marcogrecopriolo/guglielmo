#
/*
 *    Copyright (C) 2008, 2009, 2010
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
 *
 */

#ifndef	__FM_PROCESSOR__
#define	__FM_PROCESSOR__

#include	<QThread>
#include	<QObject>
#include	<sndfile.h>
#include	"constants.h"
#include	"fir-filters.h"
#include	"fft-filters.h"
#include	"sincos.h"
#include	"pllC.h"
#include	"ringbuffer.h"
#include	"oscillator.h"

class		deviceHandler;
class		RadioInterface;
class		fm_Demodulator;
class		rdsDecoder;
class		audioBase;
class		newConverter;

class	fmProcessor:public QThread {
Q_OBJECT
public:
			fmProcessor (deviceHandler *,
	                             RadioInterface *,
	                             int32_t,	// inputRate
	                             int32_t,	// decimation
	                             int32_t,	// workingRate
	                             int32_t,	// audioRate,
	                             int32_t,	// displaySize
	                             int32_t,	// averageCount
	                             int32_t,	// repeatRate
	                             int16_t,	// filterDepth
	                             int16_t);	// threshold scanning
	        	~fmProcessor (void);
	void		stop		(void);		// stop the processor
	void		setfmMode	(uint8_t);
	void		setFMdecoder	(int8_t);
	void		setSoundMode	(uint8_t);
	void		setSoundBalance	(int16_t);
	void		setDeemphasis	(int16_t);
	void		setVolume	(int16_t);
	void		setLFcutoff	(int32_t);
	void		startDumping	(SNDFILE *);
	void		stopDumping	(void);
	void		setBandwidth	(int32_t);
	void		setBandfilterDegree	(int32_t);
	void		setAttenuation	(int16_t, int16_t);
	void		setfmRdsSelector (int8_t);
	void		resetRds	(void);
	void		set_localOscillator	(int32_t);
	void		setFreezer	(bool);
	void		set_squelchMode	(bool);
	void		setInputMode	(uint8_t);
	void		setSink		(audioBase *);

	int32_t		totalAmount;
	bool		ok			(void);
	DSPFLOAT	get_pilotStrength	(void);
	DSPFLOAT	get_rdsStrength		(void);
	DSPFLOAT	get_noiseStrength	(void);
	DSPFLOAT	get_dcComponent		(void);
	void		startScanning		(void);
	void		stopScanning		(void);
	const char *	nameofDecoder	(void);

	enum Channels {
	   S_STEREO		= 0,
	   S_LEFT		= 1,
	   S_RIGHT		= 2,
	   S_LEFTplusRIGHT	= 0103,
	   S_LEFTminusRIGHT	= 0104
	};
	enum Mode {
	   FM_STEREO	= 0,
	   FM_MONO	= 1
	};

	void		set_squelchValue	(int16_t);
private:
virtual	void		run		(void);
	void		mapSpectrum	(DSPCOMPLEX *, double *);
	void		add_to_average	(double *, double *);
	void		extractLevels	(double *, int32_t);
	deviceHandler	*myRig;
	RadioInterface	*myRadioInterface;
	audioBase	*theSink;
	int32_t		inputRate;
	int32_t		fmRate;
	int32_t		workingRate;
	int32_t		audioRate;
	int32_t		displaySize;
	int32_t		averageCount;
	int32_t		repeatRate;
	RingBuffer<double> *hfBuffer;
	RingBuffer<double> *lfBuffer;
	int16_t		filterDepth;
	uint8_t		inputMode;
	int32_t		freezer;
	bool		scanning;
	int16_t		thresHold;
	DSPFLOAT	getSignal	(DSPCOMPLEX *, int32_t);
	DSPFLOAT	getNoise	(DSPCOMPLEX *, int32_t);
	bool		squelchOn;
	int32_t		spectrumSize;
	common_fft	*spectrum_fft_hf;
	common_fft	*spectrum_fft_lf;
	DSPCOMPLEX	*spectrumBuffer_hf;
	DSPCOMPLEX	*spectrumBuffer_lf;
	double		*displayBuffer;
	void		sendSampletoOutput	(DSPCOMPLEX);
	DecimatingFIR	*fmBandfilter;
	Oscillator	*localOscillator;
	newConverter	*theConverter;
	int32_t		lo_frequency;
	bool		running;
	SinCos		*mySinCos;
	LowPassFIR	*fmFilter;
	int32_t		fmBandwidth;
	int32_t		fmFilterDegree;
	bool		newFilter;

	int16_t		old_squelchValue;
	int16_t		squelchValue;

	bool		dumping;
	SNDFILE		*dumpFile;
	int32_t		decimatingScale;

	int32_t		myCount;
	int32_t		stereoCount;
	int16_t		Lgain;
	int16_t		Rgain;

	newConverter	*audioDecimator;
	DSPCOMPLEX	*audioOut;
	rdsDecoder	*myRdsDecoder;

	void		stereo	(float, DSPCOMPLEX *, DSPFLOAT *);
	void		mono	(float, DSPCOMPLEX *, DSPFLOAT *);
	fftFilter	*pilotBandFilter;
	fftFilter	*rdsBandFilter;
	fftFilter	*rdsLowPassFilter;
	HilbertFilter	*rdsHilbertFilter;

	DSPFLOAT	pilotDelay;
	DSPCOMPLEX	audioGainCorrection	(DSPCOMPLEX);
	DSPFLOAT	Volume;
	DSPFLOAT	audioGain;
	DSPFLOAT	audioGainAverage;
	int		audioGainCnt;
	int32_t		max_freq_deviation;
	int32_t		norm_freq_deviation;
	DSPFLOAT	omega_demod;
	LowPassFIR	*fmAudioFilter;

	int16_t		balance;
	DSPFLOAT	leftChannel;
	DSPFLOAT	rightChannel;
	uint8_t		fmModus;
	uint8_t		selector;
	DSPFLOAT	peakLevel;
	int32_t		peakLevelcnt;
	fm_Demodulator	*TheDemodulator;

	int8_t		rdsModus;

	float		noiseLevel;
	float		pilotLevel;
	float		rdsLevel;
	int8_t		viewSelector;
	pllC		*rds_plldecoder;
	DSPFLOAT	K_FM;

	DSPFLOAT	xkm1;
	DSPFLOAT	ykm1;
	DSPFLOAT	alpha;
	class	pilotRecovery {
	   private:
	      int32_t	Rate_in;
	      DSPFLOAT	pilot_OscillatorPhase;
	      DSPFLOAT	pilot_oldValue;
	      DSPFLOAT	omega;
	      DSPFLOAT	gain;
	      SinCos	*mySinCos;
	      DSPFLOAT	pilot_Lock;
	      bool	pll_isLocked;
	      DSPFLOAT	quadRef;
	      DSPFLOAT	accumulator;
	      int32_t	count;
	   public:
	      pilotRecovery (int32_t	Rate_in,
	                     DSPFLOAT	omega,
	                     DSPFLOAT	gain,
	                     SinCos	*mySinCos) {
	         this	-> Rate_in	= Rate_in;
	         this	-> omega	= omega;
	         this	-> gain		= gain;
	         this	-> mySinCos	= mySinCos;
	         pll_isLocked		= false;
	         pilot_Lock		= 0;
	         pilot_oldValue		= 0;
	         pilot_OscillatorPhase	= 0;
	      }

	      ~pilotRecovery (void) {
	      }

	      bool	isLocked (void) {
	         return pll_isLocked;
	      }

	      DSPFLOAT	getPilotPhase	(DSPFLOAT pilot) {
	      DSPFLOAT	OscillatorValue =
	                  mySinCos -> getCos (pilot_OscillatorPhase);
	      DSPFLOAT	PhaseError	= pilot * OscillatorValue;
	      DSPFLOAT	currentPhase;
	         pilot_OscillatorPhase += PhaseError * gain;
	         currentPhase		= PI_Constrain (pilot_OscillatorPhase);

	         pilot_OscillatorPhase =
	                   PI_Constrain (pilot_OscillatorPhase + omega);
	         
	         quadRef	= (OscillatorValue - pilot_oldValue) / omega;
//	         quadRef	= PI_Constrain (quadRef);
	         pilot_oldValue	= OscillatorValue;
	         pilot_Lock	= 1.0 / 30 * (- quadRef * pilot) +
	                          pilot_Lock * (1.0 - (1.0 / 30)); 
	         pll_isLocked	= pilot_Lock > 0.1;
	         return currentPhase;
	      }
	};
	      
	pilotRecovery	*pilotRecover;

private slots:
	void		handleConfigurationChange	(void);
signals:
	void		setPLLisLocked		(bool);
	void		hfBufferLoaded		(void);
	void		lfBufferLoaded		(void);
	void		showStrength		(float);
	void		showSoundMode		(bool);
	void		scanresult		(void);
};

#endif


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

#ifndef	_FM_PROCESSOR_H
#define	_FM_PROCESSOR_H

#include	<QThread>
#include	<QObject>
#include	<sndfile.h>
#include	"constants.h"
#include	"fir-filters.h"
#include	"fft-filters.h"
#include	"trigtabs.h"
#include	"pll.h"
#include	"ringbuffer.h"
#include	"rds-decoder.h"

class		deviceHandler;
class		RadioInterface;
class		fm_Demodulator;
class		audioBase;
class		newConverter;

class	fmProcessor:public QThread {
Q_OBJECT
public:
	enum RdsDemod {
	   FM_RDS_AUTO		= 0,
	   FM_RDS_PILOT		= 1,
	   FM_RDS_PILOTPLL	= 2,
	   FM_RDS_PLL		= 3
	};
	enum Channels {
	   S_STEREO		= 0,
	   S_LEFT		= 1,
	   S_RIGHT		= 2,
	   S_LEFT_PLUS_RIGHT	= 3,
	   S_LEFT_MINUS_RIGHT	= 4
	};
	enum Mode {
	   FM_STEREO	= 0,
	   FM_MONO	= 1
	};

			fmProcessor (deviceHandler *,
	                             RadioInterface *,
	                             int32_t,	// inputRate
	                             int32_t,	// decimation
	                             int32_t,	// workingRate
	                             int32_t,	// audioRate,
	                             int16_t);	// threshold scanning
	        	~fmProcessor (void);
	void		stop		(void);
	void		setfmMode	(uint8_t);
	void		setFMdecoder	(int8_t);
	void		setSoundMode	(uint8_t);
	void		setSoundBalance	(int16_t);
	void		setDeemphasis	(int16_t);
	void		setVolume	(int16_t);
	void		setSquelchValue	(int16_t);
	void		setSquelchMode	(bool);
	void		setLFcutoff	(int32_t);
	void		setBandwidth	(int32_t);
	void		setBandfilterDegree	(int32_t);
	void		setAttenuation	(int16_t, int16_t);
	void		setfmRdsSelector (rdsDecoder::RdsMode);
	void		setfmRdsDemod	(RdsDemod);
	void		resetRds	(void);
	void		setSink		(audioBase *);
	void		startScanning		(void);
	void		stopScanning		(void);

private:
virtual	void		run		(void);
	void		sendSampletoOutput	(DSPCOMPLEX);
	DSPCOMPLEX	audioGainCorrection	(DSPCOMPLEX);
	void		stereo	(float, DSPCOMPLEX *, DSPFLOAT *);
	void		mono	(float, DSPCOMPLEX *);

	deviceHandler	*device;
	RadioInterface	*radioInterface;
	audioBase	*audioSink;
	int32_t		fmRate;
	int32_t		workingRate;
	int32_t		audioRate;
	bool		initScan;
	bool		scanning;
	int16_t		threshold;
	DecimatingFIR	*fmBandfilter;
	newConverter	*audioConverter;
	bool		running;
	bool		initRds;
	trigTabs	*fastTrigTabs;
	LowPassFIR	*fmFilter;
	int32_t		fmBandwidth;
	int32_t		fmFilterDegree;
	bool		newFilter;

	int16_t		oldSquelchValue;
	int16_t		squelchValue;
	bool		squelchOn;

	int32_t		decimatingScale;

	int32_t		signalCount;
	int32_t		stereoCount;
	int16_t		Lgain;
	int16_t		Rgain;

	newConverter	*audioDecimator;
	DSPCOMPLEX	*audioOut;
	rdsDecoder	*rdsDataDecoder;

	int		audioGainCnt;
	DSPFLOAT	volume;
	DSPFLOAT	audioGain;
	DSPFLOAT	audioGainAverage;

	DSPFLOAT	pilotDelay;
	LowPassFIR	*fmAudioFilter;

	fftFilter	*pilotBandFilter;
	fftFilter	*rdsBandFilter;
	HilbertFilter	*rdsHilbertFilter;
	pll		*rdsPllDecoder;
	fftFilter	*rdsLowPassFilter;

	DSPFLOAT	leftChannel;
	DSPFLOAT	rightChannel;
	uint8_t		fmModus;
	uint8_t		selector;
	DSPFLOAT	peakLevel;
	int32_t		peakLevelcnt;
	fm_Demodulator	*fmDemodulator;

	rdsDecoder::RdsMode rdsModus;
	RdsDemod	rdsDemod;

	DSPFLOAT	K_FM;

	DSPFLOAT	xkm1;
	DSPFLOAT	ykm1;
	DSPFLOAT	alpha;
	pilotPll	*pilotPllFilter;

signals:
	void		showStrength		(float);
	void		showSoundMode		(bool);
	void		scanresult		(void);
};
#endif

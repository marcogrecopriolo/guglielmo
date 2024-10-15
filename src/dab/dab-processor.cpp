#
/*
 *    Copyright (C) 2014 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"dab-processor.h"

#include	<utility>
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"radio.h"
#include	"process-params.h"
#include	"dab-params.h"
#include	"timesyncer.h"
#include	"logging.h"
//
/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  *	It is the main interface to the qt-dab program,
  *	local are classes ofdmDecoder, ficHandler and mschandler.
  */

	dabProcessor::dabProcessor	(RadioInterface	*mr,
	                                 deviceHandler	*inputDevice,
	                                 processParams	*p):
	                                 params (p -> dabMode),
	                                 myReader (mr,
	                                           inputDevice,
	                                           p -> spectrumBuffer),
	                                 my_ficHandler (mr, p -> dabMode),
	                                 my_mscHandler (mr, p -> dabMode,
	                                                p -> frameBuffer),
	                                 phaseSynchronizer (mr, p),
	                                 my_TII_Detector (p -> dabMode,
	                                                  p -> tii_depth), 
	                                 my_ofdmDecoder (mr, 
	                                                 p -> dabMode,
	                                                 inputDevice -> bitDepth(),
	                                                 p -> iqBuffer) {

	this	-> myRadioInterface	= mr;
	this	-> inputDevice		= inputDevice;
	this	-> frequency		= 220000000;	// default
	this	-> threshold		= p -> threshold;
	this	-> tiiBuffer		= p -> tiiBuffer;
	this	-> T_null		= params. get_T_null();
	this	-> T_s			= params. get_T_s();
	this	-> T_u			= params. get_T_u();
	this	-> T_g			= T_s - T_u;
	this	-> T_F			= params. get_T_F();
	this	-> nrBlocks		= params. get_L();
	this	-> carriers		= params. get_carriers();
	this	-> carrierDiff		= params. get_carrierDiff();

	this	-> tii_delay		= p -> tii_delay;
	this	-> tii_counter		= 0;

	ofdmBuffer. resize (2 * T_s);
	fineOffset			= 0;	
	coarseOffset			= 0;	
	correctionNeeded		= true;
	attempts			= 0;

	goodFrames			= 0;
	badFrames			= 0;
	totalFrames			= 0;
	scanMode			= false;
        connect (this, SIGNAL (showStrength(float)),
                 mr, SLOT (showStrength(float)));
	my_TII_Detector. reset();
}

	dabProcessor::~dabProcessor() {
	   if (isRunning()) {
	   myReader. setRunning (false);
	                                // exception to be raised
	                        	// through the getSample(s) functions.
	   msleep (100);
	   while (isRunning()) {
	      usleep (100);
	   }
	}
}

void	dabProcessor::start (int frequency) {
	this	-> frequency	= frequency;
	my_ficHandler. reset	();
	transmitters. clear ();
	if (!scanMode)
	   my_mscHandler. reset_Channel ();
	log (LOG_DAB, LOG_MIN, "dab processor starting");
	QThread::start ();
}

void	dabProcessor::stop	() {
	myReader. setRunning (false);
	while (isRunning ())
	   wait ();
	usleep (10000);
	log (LOG_DAB, LOG_MIN, "dab processor stopped");
}
/***
   *	\brief run
   *	The main thread, reading samples,
   *	time synchronization and frequency synchronization
   *	Identifying blocks in the DAB frame
   *	and sending them to the ofdmDecoder who will transfer the results
   *	Finally, estimating the small freqency error
   */
void	dabProcessor::run	() {
int32_t		startIndex;
int32_t		i;
std::complex<float>	FreqCorr;
timeSyncer	myTimeSyncer (&myReader);
int		attempts;
std::vector<int16_t> ibits;
int	frameCount	= 0;
int	sampleCount	= 0;
int	totalSamples	= 0;
double	cLevel		= 0;
int	cCount		= 0;
//	inputDevice	-> resetBuffer ();
//	inputDevice	-> restartReader (frequency);
	ibits. resize (2 * params. get_carriers());
	fineOffset		= 0;
	coarseOffset		= 0;
	correctionNeeded	= true;
	attempts		= 0;
	myReader. setRunning (true);	// useful after a restart
//
//	to get some idea of the signal strength
	try {
	   for (i = 0; i < T_F / 5; i ++) {
	      myReader. getSample (0);
	   }
//Initing:
notSynced:
	   totalFrames ++;
	   totalSamples	= 0;
	   frameCount	= 0;
	   sampleCount	= 0;

	   setSynced (false);
	   my_TII_Detector. reset ();
	   switch (myTimeSyncer. sync (T_null, T_F)) {
	      case TIMESYNC_ESTABLISHED:
	         break;			// yes, we are ready

	      case NO_DIP_FOUND:
	         if (++ attempts >= 8) {
	            emit (No_Signal_Found());
	            attempts = 0;
	         }
	         goto notSynced;

	      default:			// does not happen
	      case NO_END_OF_DIP_FOUND:
	         goto notSynced;
	   }
	   myReader. getSamples (ofdmBuffer. data(),
	                        T_u, coarseOffset + fineOffset);
/**
  *	Looking for the first sample of the T_u part of the sync block.
  *	Note that we probably already had 30 to 40 samples of the T_g
  *	part
  */
	   startIndex = phaseSynchronizer. findIndex (ofdmBuffer, threshold);
	   if (startIndex < 0) { // no sync, try again
	      if (!correctionNeeded) {
	         setSyncLost();
	      }
	      badFrames ++;
	      goto notSynced;
	   }
	   sampleCount	= startIndex;
	   goto SyncOnPhase;
//
Check_endofNULL:
	   totalFrames ++;
	   frameCount ++;
	   totalSamples	+= sampleCount;
	   if (frameCount > 10) {
	      show_clockErr (totalSamples - frameCount * 196608);
	      totalSamples = 0;
	      frameCount = 0;
	   }
	   myReader. getSamples (ofdmBuffer. data(),
	                         T_u, coarseOffset + fineOffset);
/**
  *	We now have to find the exact first sample of the non-null period.
  *	We use a correlation that will find the first sample after the
  *	cyclic prefix.
  */
	   startIndex = phaseSynchronizer. findIndex (ofdmBuffer,
	                                              3 * threshold);
	   if (startIndex < 0) { // no sync, try again
	      if (!correctionNeeded) {
	         setSyncLost();
	      }
	      badFrames	++;
	      goto notSynced;
	   }

	   sampleCount = startIndex;

SyncOnPhase:
	   goodFrames ++;
	   cLevel	= 0;
	   cCount	= 0;
/**
  *	Once here, we are synchronized, we need to copy the data we
  *	used for synchronization for block 0
  */
	   memmove (ofdmBuffer. data (),
	        &((ofdmBuffer. data()) [startIndex]),
	                  (T_u - startIndex) * sizeof (std::complex<float>));
	   int ofdmBufferIndex	= T_u - startIndex;

//Block_0:
/**
  *	Block 0 is special in that it is used for fine time synchronization,
  *	for coarse frequency synchronization
  *	and its content is used as a reference for decoding the
  *	first datablock.
  *	We read the missing samples in the ofdm buffer
  */
	   setSynced (true);
	   myReader. getSamples (&((ofdmBuffer. data()) [ofdmBufferIndex]),
	                           T_u - ofdmBufferIndex,
	                           coarseOffset + fineOffset);
	   sampleCount	+= T_u;
	   my_ofdmDecoder. processBlock_0 (ofdmBuffer);
	   if (!scanMode)
	      my_mscHandler.  processBlock_0 (ofdmBuffer. data());

//	Here we look only at the block_0 when we need a coarse
//	frequency synchronization.
	   correctionNeeded	= !my_ficHandler. syncReached();
	   if (correctionNeeded) {
	      int correction	=
	            phaseSynchronizer. estimate_CarrierOffset (ofdmBuffer);
	      if (correction != 100) {
	         coarseOffset	+= 0.4 * correction * carrierDiff;
	         if (abs (coarseOffset) > Khz (35))
	            coarseOffset = 0;
	      }
	   }
/**
  *	after block 0, we will just read in the other (params -> L - 1) blocks
  */
//Data_blocks:
/**
  *	The first ones are the FIC blocks these are handled within
  *	the thread executing this "task", the other blocks
  *	are passed on to be handled in the mscHandler, running
  *	in a different thread.
  *	We immediately start with building up an average of
  *	the phase difference between the samples in the cyclic prefix
  *	and the	corresponding samples in the datapart.
  */
	   cCount	= 0;
	   cLevel	= 0;
	   FreqCorr	= std::complex<float> (0, 0);
	   for (int ofdmSymbolCount = 1;
	        ofdmSymbolCount < 4; ofdmSymbolCount ++) {
	      myReader. getSamples (ofdmBuffer. data(),
	                              T_s, coarseOffset + fineOffset);
	      sampleCount += T_s;
	      for (i = (int)T_u; i < (int)T_s; i ++) {
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	      }
	      for (int i = 0; i < (int)T_s; i ++)
	         cLevel += abs (ofdmBuffer [i]);
	      cCount += T_s;
	      my_ofdmDecoder. decode (ofdmBuffer,
	                            ofdmSymbolCount, ibits. data());
	      my_ficHandler. process_ficBlock (ibits, ofdmSymbolCount);
	      if (!scanMode)
	         my_mscHandler. process_Msc  (&((ofdmBuffer. data()) [T_g]),
	                                                    ofdmSymbolCount);
	   }

	   for (int ofdmSymbolCount = 4;
	        ofdmSymbolCount < nrBlocks; ofdmSymbolCount ++) {
	      myReader. getSamples (ofdmBuffer. data(),
	                              T_s, coarseOffset + fineOffset);
	      sampleCount += T_s;
	      for (i = (int)T_u; i < (int)T_s; i ++) {
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	      }
	      for (i = 0; i < (int)T_s; i ++) 
	         cLevel += abs (ofdmBuffer [i]);
	      cCount += T_s;
	      if (!scanMode)
                 my_mscHandler. process_Msc  (&((ofdmBuffer. data()) [T_g]),
                                                            ofdmSymbolCount);

	   }
/**
  *	OK,  here we are at the end of the frame
  *	Assume everything went well and skip T_null samples
  */
	   myReader. getSamples (ofdmBuffer. data(),
	                         T_null, coarseOffset + fineOffset);
	   sampleCount += T_null;
	   float sum	= 0;
	   for (i = 0; i < T_null; i ++)
	      sum += abs (ofdmBuffer [i]);
	   sum /= T_null;
	   static	float snr	= 0;
	   snr = 0.9 * snr +
	     0.1 * 20 * log10 ((myReader. get_sLevel() + 0.005) / sum);
	   static int ccc	= 0;
	   if (++ccc >= 2 ) {
	      ccc = 0;

	      // FIXME consistent strength measure
	      showStrength((get_db(myReader. get_sLevel(), 128)-get_db(0, 128))*100);
	   }
/*
 *	The TII data is encoded in the null period of the
 *	odd frames 
 */
          if (params. get_dabMode () == 1) {
	     if (wasSecond (my_ficHandler. get_CIFcount(), &params)) {
	         my_TII_Detector. addBuffer (ofdmBuffer);
	         if (++tii_counter >= tii_delay) {
	            uint16_t res = my_TII_Detector. processNULL ();
	            if (res != 0) {
	               uint8_t mainId	= res >> 8;
	               uint8_t subId	= res & 0xFF;
	               tiiBuffer -> putDataIntoBuffer (ofdmBuffer. data(), T_u);
	               show_tii (mainId, subId);
	            }
	            tii_counter = 0;
	            my_TII_Detector. reset();
	         }
	      }
	   }
/**
  *	The first sample to be found for the next frame should be T_g
  *	samples ahead. Before going for the next frame, we
  *	we just check the fineCorrector
  */
//NewOffset:
//     we integrate the newly found frequency error with the
//     existing frequency error.
//
	   fineOffset += 0.05 * arg (FreqCorr) / (2 * M_PI) * carrierDiff;

	   if (fineOffset > carrierDiff / 2) {
	      coarseOffset += carrierDiff;
	      fineOffset -= carrierDiff;
	   }
	   else
	   if (fineOffset < -carrierDiff / 2) {
	      coarseOffset -= carrierDiff;
	      fineOffset += carrierDiff;
	   }

//ReadyForNewFrame:
///	and off we go, up to the next frame
	   goto Check_endofNULL;
	}
	catch (int e) {
	   if (e != 20 && e != 21)
		log (LOG_DAB, LOG_CHATTY, "dabProcessor caugth %i", e);
	}
//	inputDevice	-> stopReader ();
}
//
//
void	dabProcessor::set_scanMode	(bool b) {
	scanMode	= b;
	attempts	= 0;
}

void	dabProcessor::getFrameQuality	(int	*totalFrames,
	                                 int	*goodFrames,
	                                 int	*badFrames) {
	*totalFrames		= this	-> totalFrames;
	*goodFrames		= this	-> goodFrames;
	*badFrames		= this	-> badFrames;
	this	-> totalFrames	= 0;
	this	-> goodFrames	= 0;
	this	-> badFrames	= 0;
}
//
//	just convenience functions
//	ficHandler abstracts channel data

QString	dabProcessor::findService	(uint32_t SId, int SCIds) {
	return my_ficHandler. findService (SId, SCIds);
}

void	dabProcessor::getParameters	(const QString &s,
	                                 uint32_t *p_SId, int*p_SCIds) {
	my_ficHandler. getParameters (s, p_SId, p_SCIds);
}

std::vector<serviceId>	dabProcessor::getServices	(int n) {
	return my_ficHandler. getServices (n);
}

bool	dabProcessor::is_audioService	(const QString &s) {
audiodata ad;
	my_ficHandler. dataforAudioService (s, &ad);
	return ad. defined;
}

int	dabProcessor::getSubChId	(const QString &s, uint32_t SId) {
	return my_ficHandler. getSubChId (s, SId);
}

bool	dabProcessor::is_packetService	(const QString &s) {
packetdata pd;
	my_ficHandler. dataforPacketService (s, &pd, 0);
	return pd. defined;
}

void	dabProcessor::dataforAudioService	(const QString &s,
	                                         audiodata *d) {
	my_ficHandler. dataforAudioService (s, d);
}

void	dabProcessor::dataforPacketService	(const QString &s,
	                                         packetdata *pd,
	                                         int16_t compnr) {
	my_ficHandler. dataforPacketService (s, pd, compnr);
}

uint8_t	dabProcessor::get_ecc 		() {
	return my_ficHandler. get_ecc();
}

int32_t dabProcessor::get_ensembleId	() {
	return my_ficHandler. get_ensembleId();
}

QString dabProcessor::get_ensembleName	() {
	return my_ficHandler. get_ensembleName();
}

void	dabProcessor::set_epgData	(int SId,
	                                 int32_t theTime, const QString &s) {
	my_ficHandler. set_epgData (SId, theTime, s);
}

bool	dabProcessor::has_timeTable	(uint32_t SId) {
	return my_ficHandler. has_timeTable (SId);
}

std::vector<epgElement>	dabProcessor::find_epgData	(uint32_t SId) {
	return my_ficHandler. find_epgData (SId);
}

//
//	for the mscHandler:
void	dabProcessor::reset_Services	() {
	if (!scanMode)
	   my_mscHandler. reset_Channel ();
}

void	dabProcessor::stopService (descriptorType *d) {
	if (!scanMode)
	   my_mscHandler. stopService (d);
}

bool    dabProcessor::set_audioChannel (audiodata *d,
	                                      RingBuffer<int16_t> *b) {
	if (!scanMode)
	   return my_mscHandler. set_Channel (d, b, (RingBuffer<uint8_t> *)nullptr);
	else
	   return false;
}

bool    dabProcessor::set_dataChannel (packetdata *d,
	                                      RingBuffer<uint8_t> *b) {
	if (!scanMode)
	   return my_mscHandler. set_Channel (d, (RingBuffer<int16_t> *)nullptr, b);
	else
	   return false;
}

void	dabProcessor::startDumping	(SNDFILE *f) {
	myReader. startDumping (f);
}

void	dabProcessor::stopDumping() {
	myReader. stopDumping();
}

bool	dabProcessor::wasSecond (int16_t cf, dabParams *p) {
    switch (p -> get_dabMode()) {
	   default:
	   case 1:
	      return (cf & 07) >= 4;
	   case 2:
	   case 3:
	      return (cf & 02);
	   case 4:
	      return (cf & 03) >= 2;
	}
}


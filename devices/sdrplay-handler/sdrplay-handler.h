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
 *    Taken from dab-mini, with bug fixes and enhancements.
 *
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 */
#ifndef __SDRPLAY_HANDLER__
#define	__SDRPLAY_HANDLER__

#include	<atomic>
#include	"constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"mirsdrapi-rsp.h"

typedef void (*mir_sdr_StreamCallback_t)(int16_t *xi,
	                                 int16_t *xq,
	                                 uint32_t firstSampleNum,
	                                 int32_t grChanged,
	                                 int32_t rfChanged,
	                                 int32_t fsChanged,
	                                 uint32_t numSamples,
	                                 uint32_t reset,
	                                 uint32_t hwRemoved,
	                                 void *cbContext);
typedef	void	(*mir_sdr_GainChangeCallback_t)(uint32_t gRdB,
	                                        uint32_t lnaGRdB,
	                                        void *cbContext);

// Dll and ".so" function prototypes
typedef mir_sdr_ErrT (*pfn_mir_sdr_StreamInit) (int *gRdB, double fsMHz,
						double rfMHz, mir_sdr_Bw_MHzT bwType,
						mir_sdr_If_kHzT ifType, int LNAEnable,
						int *gRdBsystem, int useGrAltMode,
						int *samplesPerPacket,
						mir_sdr_StreamCallback_t StreamCbFn,
						mir_sdr_GainChangeCallback_t GainChangeCbFn,
						void *cbContext);

typedef mir_sdr_ErrT (*pfn_mir_sdr_Reinit) (int *gRdB, double fsMHz, double rfMHz,
		      mir_sdr_Bw_MHzT bwType, mir_sdr_If_kHzT ifType, mir_sdr_LoModeT,
		      int, int*, int, int*, mir_sdr_ReasonForReinitT);

typedef mir_sdr_ErrT (*pfn_mir_sdr_StreamUninit)(void);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetRf)(double drfHz, int abs, int syncUpdate);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetFs)(double dfsHz, int abs, int syncUpdate, int reCal);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetGr)(int gRdB, int abs, int syncUpdate);
typedef mir_sdr_ErrT (*pfn_mir_sdr_RSP_SetGr)(int gRdB, int lnaState,
                                              int abs, int syncUpdate);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetGrParams)(int minimumGr, int lnaGrThreshold);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetDcMode)(int dcCal, int speedUp);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetDcTrackTime)(int trackTime);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetSyncUpdateSampleNum)(unsigned int sampleNum);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetSyncUpdatePeriod)(unsigned int period);
typedef mir_sdr_ErrT (*pfn_mir_sdr_ApiVersion)(float *version);
typedef mir_sdr_ErrT (*pfn_mir_sdr_ResetUpdateFlags)(int resetGainUpdate, int resetRfUpdate,
						     int resetFsUpdate);
typedef mir_sdr_ErrT (*pfn_mir_sdr_AgcControl)(uint32_t, int, int, uint32_t,
	                                       uint32_t, int, int);
typedef mir_sdr_ErrT (*pfn_mir_sdr_DCoffsetIQimbalanceControl) (uint32_t, uint32_t);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetPpm)(double);
typedef mir_sdr_ErrT (*pfn_mir_sdr_DebugEnable)(uint32_t);
typedef mir_sdr_ErrT (*pfn_mir_sdr_GetDevices) (mir_sdr_DeviceT *, uint32_t *, uint32_t);
typedef mir_sdr_ErrT (*pfn_mir_sdr_GetCurrentGain) (mir_sdr_GainValuesT *);
typedef mir_sdr_ErrT (*pfn_mir_sdr_GetHwVersion) (unsigned char *);
typedef mir_sdr_ErrT (*pfn_mir_sdr_RSPII_AntennaControl) (mir_sdr_RSPII_AntennaSelectT);
typedef mir_sdr_ErrT (*pfn_mir_sdr_rspDuo_TunerSel) (mir_sdr_rspDuo_TunerSelT);
typedef mir_sdr_ErrT (*pfn_mir_sdr_SetDeviceIdx) (unsigned int);
typedef mir_sdr_ErrT (*pfn_mir_sdr_ReleaseDeviceIdx) ();

class sdrplayHandler: public deviceHandler {
Q_OBJECT
public:
    sdrplayHandler(void);
    ~sdrplayHandler(void);

    int32_t deviceCount(void);
    QString deviceName(int32_t devNo);
    bool setDevice(int32_t devNo);
    bool restartReader(int32_t);
    void stopReader(void);
    int32_t getSamples(std::complex<float> *, int32_t, agcStats *stats);
    int32_t Samples(void);
    void resetBuffer(void);
    int16_t bitDepth(void);
    int32_t getRate(void);

    void getIfRange(int *, int *);
    void getLnaRange(int *, int *);
    void setIfGain(int);
    void setLnaGain(int);
    void setAgcControl(int);

//  The buffer should be visible by the callback function
    RingBuffer<std::complex<float>> _I_Buffer;
    float denominator;

private:
    QString errorCodes(mir_sdr_ErrT);
    bool switchDevice(int32_t devNo, mir_sdr_DeviceT *devDesc);
    bool loadFunctions(void);

    int	 GRdB;
    int	 lnaGain;
    int	 lnaGainMax;
    bool agcMode;
    pfn_mir_sdr_StreamInit my_mir_sdr_StreamInit;
    pfn_mir_sdr_Reinit my_mir_sdr_Reinit;
    pfn_mir_sdr_StreamUninit my_mir_sdr_StreamUninit;
    pfn_mir_sdr_SetRf my_mir_sdr_SetRf;
    pfn_mir_sdr_SetFs my_mir_sdr_SetFs;
    pfn_mir_sdr_SetGr my_mir_sdr_SetGr;
    pfn_mir_sdr_RSP_SetGr my_mir_sdr_RSP_SetGr;
    pfn_mir_sdr_SetGrParams my_mir_sdr_SetGrParams;
    pfn_mir_sdr_SetDcMode my_mir_sdr_SetDcMode;

    pfn_mir_sdr_SetDcTrackTime my_mir_sdr_SetDcTrackTime;
    pfn_mir_sdr_SetSyncUpdateSampleNum my_mir_sdr_SetSyncUpdateSampleNum;
    pfn_mir_sdr_SetSyncUpdatePeriod my_mir_sdr_SetSyncUpdatePeriod;
    pfn_mir_sdr_ApiVersion my_mir_sdr_ApiVersion;
    pfn_mir_sdr_ResetUpdateFlags my_mir_sdr_ResetUpdateFlags;
    pfn_mir_sdr_AgcControl my_mir_sdr_AgcControl;
    pfn_mir_sdr_DCoffsetIQimbalanceControl my_mir_sdr_DCoffsetIQimbalanceControl;
    pfn_mir_sdr_SetPpm my_mir_sdr_SetPpm;
    pfn_mir_sdr_DebugEnable my_mir_sdr_DebugEnable;
    pfn_mir_sdr_GetDevices my_mir_sdr_GetDevices;
    pfn_mir_sdr_GetCurrentGain my_mir_sdr_GetCurrentGain;
    pfn_mir_sdr_GetHwVersion my_mir_sdr_GetHwVersion;
    pfn_mir_sdr_RSPII_AntennaControl my_mir_sdr_RSPII_AntennaControl;
    pfn_mir_sdr_rspDuo_TunerSel my_mir_sdr_rspDuo_TunerSel;
    pfn_mir_sdr_SetDeviceIdx my_mir_sdr_SetDeviceIdx;
    pfn_mir_sdr_ReleaseDeviceIdx my_mir_sdr_ReleaseDeviceIdx;

    int16_t hwVersion;
    int16_t deviceIndex;
    int32_t inputRate;
    bool libraryLoaded;
    std::atomic<bool> running;
    HINSTANCE Handle;
    int16_t nrBits;

};
#endif

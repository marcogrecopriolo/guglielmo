/*
 *    Copyright (C) 2022
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
 *    Taken from the Qt-DAB program with bug fixes and enhancements.
 *
 *    Copyright (C) 2015 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef DAB_PROCESSOR_H
#define DAB_PROCESSOR_H
/*
 *	dabProcessor is the embodying of all functionality related
 *	to the actal DAB processing.
 */
#include "constants.h"
#include "device-handler.h"
#include "fic-handler.h"
#include "msc-handler.h"
#include "ofdm-decoder.h"
#include "phasereference.h"
#include "ringbuffer.h"
#include "sample-reader.h"
#include "tii_detector.h"
#include <QByteArray>
#include <QObject>
#include <QStringList>
#include <QThread>
#include <cstdint>
#include <sndfile.h>
#include <vector>

class RadioInterface;
class dabParams;
class processParams;

class dabProcessor : public QThread {
    Q_OBJECT
  public:
    dabProcessor(RadioInterface *, deviceHandler *, processParams *);
    ~dabProcessor();
    void start(int32_t);
    void stop();
    void startDumping(SNDFILE *);
    void stopDumping();
    void set_scanMode(bool);
    void getFrameQuality(int *, int *, int *);

    //	inheriting from our delegates
    //	for the ficHandler:
    QString findService(uint32_t, int);
    void getParameters(const QString &, uint32_t *, int *);
    std::vector<serviceId> getServices(int);
    bool is_audioService(const QString &s);
    bool is_packetService(const QString &s);
    void dataforAudioService(const QString &, audiodata *);
    void dataforPacketService(const QString &, packetdata *, int16_t);
    int getSubChId(const QString &, uint32_t);
    uint8_t get_ecc();
    int32_t get_ensembleId();
    QString get_ensembleName();
    void set_epgData(int32_t, int32_t, const QString &);
    bool has_timeTable(uint32_t);
    std::vector<epgElement> find_epgData(uint32_t);

    //	for the mscHandler
    void reset_Services();
    void stopService(serviceDescriptor *);
    bool set_audioChannel(audiodata *, RingBuffer<int16_t> *);
    bool set_dataChannel(packetdata *, RingBuffer<uint8_t> *);

  private:
    int frequency;
    int threshold;
    int totalFrames;
    int goodFrames;
    int badFrames;
    bool tiiSwitch;
    int16_t tii_depth;
    int16_t echo_depth;
    deviceHandler *inputDevice;
    dabParams params;
    RingBuffer<std::complex<float>> *tiiBuffer;
    int16_t tii_delay;
    int16_t tii_counter;

    sampleReader myReader;
    RadioInterface *myRadioInterface;
    ficHandler my_ficHandler;
    mscHandler my_mscHandler;
    phaseReference phaseSynchronizer;
    TII_Detector my_TII_Detector;
    ofdmDecoder my_ofdmDecoder;

    int16_t attempts;
    bool scanMode;
    int32_t T_null;
    int32_t T_u;
    int32_t T_s;
    int32_t T_g;
    int32_t T_F;
    int32_t nrBlocks;
    int32_t carriers;
    int32_t carrierDiff;
    int16_t fineOffset;
    int32_t coarseOffset;
    QByteArray transmitters;
    bool correctionNeeded;
    std::vector<std::complex<float>> ofdmBuffer;
    bool wasSecond(int16_t, dabParams *);
    virtual void run();

  signals:
    void setSynced(bool);
    void No_Signal_Found();
    void setSyncLost();
    void show_tii(int, int);
    void show_Spectrum(int);
    void showStrength(float);
    void show_clockErr(int);
};
#endif

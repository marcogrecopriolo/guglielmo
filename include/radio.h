/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RADIO_H__
#define __RADIO_H__

#include <QMainWindow>
#include <QStringList>
#include <QStandardItemModel>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QTimer>
#include <sndfile.h>
#ifdef HAVE_MPRIS
#include <Mpris>
#include <MprisPlayer>
#endif
#include "ui_guglielmo.h"
#include "ui_settings.h"
#include "constants.h"
#include "dab-processor.h"
#include "fm-processor.h"
#include "ringbuffer.h"
#include "band-handler.h"
#include "device-handler.h"
#include "process-params.h"

// dialogs
void warning(QWidget *parent, QString what);
bool yesNo(QWidget *parent);
QString chooseFileName(QWidget *parent, QSettings *settings, QString what, QString filters,
			QString state, QString fileName);

class audioBase;

class dabService {
public:
    QString serviceName;
    uint32_t SId;
    int SCIds;
    bool valid;
    bool fromEnd;
};

enum deviceControls {
    AGC =	0x01,
    IF_GAIN =	0x02,
    LNA_GAIN =	0x04
};

class deviceDescriptor {
public:
    QString deviceName;
    deviceHandler *device;
    int controls;
};

class RadioInterface: public QWidget, private Ui_guglielmo {
Q_OBJECT
public:
    RadioInterface(QSettings *, QWidget	*parent = nullptr);
    ~RadioInterface();

private:

// DAB
    RingBuffer<std::complex<float>> iqBuffer;
    RingBuffer<std::complex<float>> tiiBuffer;
    RingBuffer<float> responseBuffer;
    RingBuffer<uint8_t> frameBuffer;
    RingBuffer<int16_t> audioBuffer;

    dabService nextService;
    dabService currentService;
    bandHandler DABband;
    processParams DABglobals;
    int serviceOrder;
    int scanIndex;

// FM
    double FMfreq;
    int32_t fmRate;
    int32_t workingRate;
    int32_t audioRate;
    int16_t FMthreshold;
    int scanInterval;

    int FMdecoder;
    int deemphasis;
    int lowPassFilter;
    int FMfilter;
    int FMdegree;
    int FMaudioGain;

// UI
    QDialog *settingsDialog;
    Ui::settings settingsUi;
    QAction *aboutAction;
    QAction *settingsAction;
    QAction *stationsAction;
    QAction *slidesAction;
    QStandardItemModel ensembleModel;
    QString stereoStyle;
    QSettings *settings;
    std::vector<serviceId> serviceList;
    std::vector<deviceDescriptor> deviceList;

// Control
#ifdef HAVE_MPRIS
    MprisPlayer player;
    QVariantMap metadata;
    int lastPreset;
    bool skipPresetMode;
    QString currentPicFile;
#endif

// devices, processors
    dabProcessor *DABprocessor;
    fmProcessor *FMprocessor;
    deviceHandler *inputDevice;
    SNDFILE *recordingFile;
    int deviceUiControls;
    QString deviceName;
    int ifGain;
    int lnaGain;
    bool agc;
    bool isFM;
    bool isSlides;
    bool playing;
    bool recording;
    bool scanning;
    int16_t ficBlocks;
    int16_t ficSuccess;
    QTimer *scanTimer;
    double scanIncrement;

// audio
    audioBase *soundOut;
    bool isQtAudio;
    int latency;
    int soundChannel;

// settings;
    bool descendingOrder;
    bool saveIsFM;
    double saveFMfreq;
    int saveChannel;
    dabService saveService;

// operation
    void findDevices();
    void makeDABprocessor();
    void makeFMprocessor();
    void startDAB(const QString &);
    void stopDAB();
    void startDABService(dabService *);
    void stopDABService();
    void showSlides(QByteArray data, int contentType, QString pictureName, int dirs);
    void showSlides(QPixmap p);
    void startFM(int32_t);
    void stopFM();
    void startFMscan(bool);
    void stopFMscan();
    void toFM();
    void toDAB();
    void setPlaying();
    void setRecording();
    void stopRecording();
    void setScanning();
    void changePreset(int);
    void changeStation(int);
    void cleanScreen();
    void terminateProcess();

// MPRIS
#ifdef HAVE_MPRIS
    void mprisLabelAndText(QString, QString);
    void mprisEmptyArt(bool);
#endif

public slots:
    void addToEnsemble(const QString &, uint);
    void nameOfEnsemble(int, const QString &);
    void ensembleLoaded(int);
    void showQuality(bool);
    void showStrength(float);
    void showLabel(QString);
    void showText(QString);
    void showSoundMode(bool);
    void handleMotObject(QByteArray, QString, int, bool);
    void changeInConfiguration();
    void newAudio(int, int);
    void scanDone();
    void scanFound();
    void scanEnsembleLoaded(int);
    void closeEvent(QCloseEvent *event);

private slots:

// tuner
    void handleAddFMPreset();
    void handleDeleteFMPreset();
    void handleScanDown();
    void handleScanUp();
    void handleStopScan();
    void handleFMfrequency(double);
    void handleFMButton();
    void handleAddDABPreset();
    void handleDeleteDABPreset();
    void handleNextChanButton();
    void handlePrevChanButton();
    void handleSelectChannel(int);
    void handleSelectService(QModelIndex);
    void handleDABButton();
    void handlePresetSelector(int);
    void handlePlayButton();
    void handlePauseButton();
    void handleRecordButton();
    void handleStopRecordButton();
    void handleVolume(double);
    void handleSquelch(double);
    void handleAboutAction();
    void handleSettingsAction();
    void handleStationsAction();
    void handleSlidesAction();

// settings
    void settingsClose(void);

    void startFullScan();
    void stopFullScan();
    void dropPreset();
    void lowerPreset();
    void liftPreset();
    void sortPresets();
    void clearScanList();
    void copyStation();

    void setUiStyle(int);

#ifdef HAVE_MPRIS
    void setRemoteMode(int);
#endif

    void setSoundMode(int);
    void setSoundOutput(int);
    void setLatency(int);

    void setDecoder(int);
    void setDeemphasis(int);
    void setLowPassFilter(int);
    void setFMFilter(int);
    void setFMDegree(int);
    void setFMaudioGain(int);

    void setDevice(int);
    void setAgcControl(int);
    void setIfGain(int);
    void setLnaGain(int);

// scan
    void nextFrequency();
    void nextFullScanFrequency();
    void nextFullDABScan();

#ifdef HAVE_MPRIS
// MPRIS
    void mprisPlayButton();
    void mprisPauseButton();
    void mprisPlayPause();
    void mprisNextButton();
    void mprisPreviousButton();
    void mprisVolume(double);
    void mprisClose();
#endif
};
#endif		// __RADIO_H__

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

#ifndef RADIO_H
#define RADIO_H

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

// UI
#define ICON_LISTVIEW_SIZE 16, 16
#define ICON_MIN_SIZE 32
#define ICON_MAX_SIZE 64
#define SLIDE_MIN_SIZE 128

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
    int slidePriority;
    bool valid;
    bool fromEnd;
    bool autoPlay;
    void setValid() { valid = (serviceName != ""); }
};

enum deviceControls {
    HW_AGC =	0x01,
    IF_GAIN =	0x02,
    LNA_GAIN =	0x04,
    SW_AGC =	0x08,
    COMBO_AGC =	0x10
};

enum agcMode {
    AGC_OFF =		0,
    AGC_ON =		1,
    AGC_SOFTWARE =	2,
    AGC_COMBINED =	3
};

enum dabDisplay {
    DD_STATIONS =	0,
    DD_SLIDES =		1,
    DD_COUNT  = 	2
};

class deviceDescriptor {
public:
    QString deviceType;
    deviceHandler *device;
    int controls;
};

class RadioInterface: public QWidget, private Ui_guglielmo {
Q_OBJECT
public:
    RadioInterface(QSettings *, QWidget	*parent = nullptr);
    ~RadioInterface();
    void processGain(agcStats *stats, int amount);

private:

// DAB
    RingBuffer<std::complex<float>> iqBuffer;
    RingBuffer<std::complex<float>> tiiBuffer;
    RingBuffer<float> responseBuffer;
    RingBuffer<uint8_t> frameBuffer;
    RingBuffer<int16_t> audioBuffer;
#ifdef USE_SPI
    RingBuffer<uint8_t> dataBuffer;
#endif

    dabService nextService;
    bandHandler DABband;
    dabService currentService;

    // currently we only support one data service
    dabService dataService;
    processParams DABglobals;
    int serviceOrder;
    int scanIndex;
    int scanRetryCount;

// FM
    double FMfreq;
    int32_t workingRate;
    int32_t audioRate;
    int16_t FMthreshold;

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
    QString originalTitle;
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
    QString deviceType;
    QString deviceId;

    int ifGain;
    int minIfGain;
    int maxIfGain;
    int lnaGain;
    int minLnaGain;
    int maxLnaGain;
    int agc;
    int swAgc;
    int swAgcSkip;
    int swAgcAccrue;
    int swAgcAmount;
    agcStats stats;
    int minSignal;
    int maxSignal;

    int dabDisplay;
    bool isFM;
    bool playing;
    bool recording;
    bool scanning;
    int16_t ficBlocks;
    int16_t ficSuccess;
    QTimer *scanTimer;
    double scanIncrement;
    int scanInterval;
    int scanRetry;

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
    void startDAB();
    void stopDAB();
    void startDABService(dabService *);
    void stopDABService();
    void startDataService(QString, uint);
    void stopDataServices();
    void handleSlides(QByteArray data, int contentType, QString pictureName, int dirs);
    void showSlides(QPixmap p);
    void handleEPGPicture(QByteArray data, const char *type, QString pictureName);
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
    void checkLnaGain();
    void checkIfGain();
    void cleanScreen();
    void terminateProcess();
    void dabDisplayOn();
    void dabDisplayOff();

//UI
    void setIconAndTitle();

// MPRIS
#ifdef HAVE_MPRIS
    void mprisLabelAndText(QString, QString);
    void mprisEmptyArt(bool);
#endif

// SW AGC
    void resetSwAgc(void);
    void resetAgcStats(void);

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
    void scanEnsembleLoaded();
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);

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
    void handleSelectService(int);
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
    void handleLeftDisplayButton();
    void handleRightDisplayButton();

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
    void setSoundChannels(void);
    void setSoundOutput(int);
    void setLatency(int);

    void setDecoder(int);
    void setDeemphasis(int);
    void setLowPassFilter(int);
    void setFMFilter(int);
    void setFMDegree(int);
    void setFMaudioGain(int);

    void setDevice(int);
    void setDeviceName(int);
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

signals:
    void advanceScan(int);
};
#endif		// RADIO_H

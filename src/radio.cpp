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
 */
#include <QSettings>
#include <QMenu>
#include <QStringList>
#include <QCloseEvent>
#include <QFileInfo>
#include <fstream>
#include "constants.h"
#include "settings.h"
#include "fm-processor.h"
#include "fm-demodulator.h"
#include "rds-decoder.h"
#include <iostream>
#include <numeric>
#include <unistd.h>
#include <vector>
#include "radio.h"
#include "band-handler.h"
#include "Qt-audio.h"
#include "audiosink.h"
#ifdef	HAVE_RTLSDR
#include "rtlsdr-handler.h"
#endif
#ifdef	HAVE_SDRPLAY
#include "sdrplay-handler.h"
#endif
#ifdef	HAVE_AIRSPY
#include "airspy-handler.h"
#endif
#ifdef	HAVE_HACKRF
#include "hackrf-handler.h"
#endif
#ifdef	HAVE_LIME
#include "lime-handler.h"
#endif
#ifdef	HAVE_PLUTO
#include "pluto-handler.h"
#endif

// A few repeated Ui constants
#define BAD_SERVICE	"cannot run this service"
#define BAD_PRESET	"this preset is not valid"

// most buffer not used locally, but within the DAB processor
RadioInterface::RadioInterface (QSettings *Si, QWidget	 *parent):
	QWidget(parent),
        iqBuffer(2 * 1536),
        tiiBuffer(32768),
        responseBuffer(32768),
        frameBuffer(2 * 32768),
        audioBuffer(8 * 32768),
	DABband("") {

    QString presetName;
    QString channel;
    int i;
    int	lastVolume;
    int	lastSquelch;
    QPalette p;

    settings = Si;

    // UI
    setupUi(this);
    settingsDialog = nullptr;

    p = signalQuality->palette();
    p.setColor(QPalette::Base, Qt::transparent);
    signalQuality->setPalette(p);
    signalQuality->setRangeFlags(QwtInterval::ExcludeMinimum);
    p = signalStrength->palette();
    p.setColor(QPalette::Base, Qt::transparent);
    signalStrength->setPalette(p);
    signalStrength->setRangeFlags(QwtInterval::ExcludeMinimum);
    menuButton->setIcon(QIcon(":/menubutton.png"));
    QMenu *menu = new QMenu();
    aboutAction = new QAction(QIcon(":/copyright.png"), "about");
    settingsAction = new QAction(QIcon(":/settings.png"), "settings");
    menu->addAction(settingsAction);
    menu->addAction(aboutAction);
    menu->setLayoutDirection(Qt::LeftToRight);
    menuButton->setMenu(menu);
    prevChanButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    nextChanButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    scanBackButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    stopScanButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    scanForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

    // setting the background to transparent hides the label, so we 
    // store the initial style instead
    stereoStyle = stereoLabel->styleSheet();

    serviceList.clear();
    ensembleModel.clear();
    ensembleDisplay->setModel(&ensembleModel);

    // Settings
    // UI
    settings->beginGroup(GROUP_UI);
    QString theme = settings->value(UI_THEME, "").toString();
    if (theme != "")
	QApplication::setStyle(theme);
    settings->endGroup();

    // DAB
    DABprocessor = nullptr;
    settings->beginGroup(GROUP_DAB);
    DABglobals.threshold = settings->value(DAB_THRESHOLD, DAB_DEF_THRESHOLD).toInt();
    DABglobals.diff_length = settings->value(DAB_DIFF_LENGTH, DIFF_LENGTH).toInt();
    DABglobals.tii_delay = settings->value(DAB_TII_DELAY, DAB_DEF_TII_DELAY).toInt();
    if (DABglobals.tii_delay < DAB_MIN_TII_DELAY)
	DABglobals.tii_delay = DAB_MIN_TII_DELAY;
    DABglobals.tii_depth = settings->value(DAB_TII_DEPTH, DAB_DEF_TII_DEPTH).toInt();
    DABglobals.echo_depth = settings->value(DAB_ECHO_DEPTH, DAB_DEF_ECHO_DEPTH).toInt();
    serviceOrder = settings->value(DAB_SERVICE_ORDER, DAB_DEF_SERVICE_ORDER).toInt();
    settings->endGroup();

    // FM settings
    FMprocessor = nullptr;
    scanTimer = nullptr;
    settings->beginGroup(GROUP_FM);
    workingRate = settings->value(FM_WORKING_RATE, FM_DEF_WORKING_RATE).toInt();
    FMthreshold = settings->value(FM_THRESHOLD, FM_DEF_THRESHOLD).toInt();
    scanInterval = settings->value(FM_SCAN_INTERVAL, FM_DEF_SCAN_INTERVAL).toInt();

    FMfilter = settings->value(FM_FILTER, FM_DEF_FILTER).toInt();
    FMdegree = settings->value(FM_DEGREE, FM_DEF_DEGREE).toInt();
    FMdecoder = settings->value(FM_DECODER, FM_DEF_DECODER).toInt();
    deemphasis = settings->value(FM_DEEMPHASIS, FM_DEF_DEEMPHASIS).toInt();
    lowPassFilter = settings->value(FM_LOW_PASS_FILTER, FM_DEF_LOW_PASS_FILTER).toInt();
    FMaudioGain = settings->value(FM_AUDIO_GAIN, FM_DEF_AUDIO_GAIN).toInt();
    settings->endGroup();

    settings->beginGroup(GROUP_SOUND);
    isQtAudio = (settings->value(SOUND_MODE, SOUND_DEF_MODE).toString() == SOUND_QT);
    latency = settings->value(SOUND_LATENCY, SOUND_DEF_LATENCY).toInt();
    audioRate = settings->value(SOUND_AUDIO_RATE, SOUND_DEF_AUDIO_RATE).toInt();
    soundChannel = settings->value(SOUND_CHANNEL, SOUND_DEF_CHANNEL).toInt();
    if (isQtAudio) {
	soundOut = new Qt_Audio;
    } else {
	soundOut = new audioSink(latency);
	if (!((audioSink *) soundOut)->selectDevice(soundChannel))
	    ((audioSink *) soundOut)->selectDefaultDevice();
    }
    settings->endGroup();

    // base settings
    lastVolume  = settings->value(GEN_VOLUME, GEN_DEF_VOLUME).toInt();
    if (lastVolume < 0)
	lastVolume = 0;
    else if (lastVolume > 100)
	lastVolume = 100;
    soundOut->setVolume(qreal(lastVolume)/100);
    volumeKnob->setValue(double(lastVolume));

    findDevices();
    DABband.setupChannels(channelSelector, BAND_III);
    nextService.valid = false;
    currentService.serviceName = settings->value(GEN_SERVICE_NAME, "").toString();
    currentService.valid = currentService.serviceName != "";
    FMfreq = settings->value (GEN_FM_FREQUENCY, DEF_FM).toDouble();
    if (FMfreq < MIN_FM)
	FMfreq = MIN_FM;
    else if (FMfreq > MAX_FM)
	FMfreq = MAX_FM;
    frequencyKnob->setValue(double(FMfreq));
    frequencyLCD->display(int(FMfreq*1000));

    // presets
    int size = settings->beginReadArray(GROUP_PRESETS);
    for (int i = 0; i < size; ++i) {
	settings->setArrayIndex(i);
	presetSelector->addItem(settings->value(PRESETS_NAME).toString());
    }
    settings->endArray();

    makeDABprocessor();
    makeFMprocessor();

    lastSquelch = settings->value(GEN_SQUELCH, GEN_DEF_SQUELCH).toInt();
    if (lastSquelch < 0)
	lastSquelch = 0;
    else if (lastSquelch > 100)
	lastSquelch = 100;
    squelchKnob->setValue(double(lastSquelch));
    if (FMprocessor != nullptr) {
	FMprocessor->setSquelchValue(100-int(lastSquelch));
    }

    playing = false;
    recording = false;
    scanning = false;
    isFM = (settings->value(GEN_TUNER_MODE, GEN_DEF_TUNER_MODE).toString() == GEN_FM);
    if (isFM)
	toFM();
    else
	toDAB();

    qApp->installEventFilter(this);
    connect(aboutAction, SIGNAL(triggered(bool)),
		this, SLOT(handleAboutAction()));
    connect(settingsAction, SIGNAL(triggered(bool)),
		this, SLOT(handleSettingsAction()));
    connect(presetSelector, SIGNAL(activated(const QString &)),
		this, SLOT(handlePresetSelector (const QString &)));
    connect(ensembleDisplay, SIGNAL(clicked(QModelIndex)),
		this, SLOT(handleSelectService(QModelIndex)));
    connect(channelSelector, SIGNAL(activated(const QString &)),
		this, SLOT(handleSelectChannel(const QString &)));
    connect(prevChanButton, SIGNAL(clicked(void)),
		this, SLOT(handlePrevChanButton(void)));
    connect(nextChanButton, SIGNAL(clicked(void)),
		this, SLOT(handleNextChanButton(void)));
    connect(DABButton, SIGNAL(clicked(void)),
		this, SLOT(handleDABButton(void)));
    connect(frequencyKnob, SIGNAL(valueChanged(double)),
		this, SLOT(handleFMfrequency(double)));
    connect(scanBackButton, SIGNAL(clicked(void)),
		this, SLOT(handleScanDown(void)));
    connect(scanForwardButton, SIGNAL(clicked(void)),
		this, SLOT(handleScanUp(void)));
    connect(stopScanButton, SIGNAL(clicked(void)),
		this, SLOT(handleStopScan(void)));
    connect(FMButton, SIGNAL(clicked(void)),
		this, SLOT(handleFMButton(void)));
    connect(volumeKnob, SIGNAL (valueChanged (double)),
		this, SLOT(handleVolume(double)));
    connect(squelchKnob, SIGNAL(valueChanged(double)),
		this, SLOT(handleSquelch(double)));

    channel = settings->value(GEN_CHANNEL, GEN_DEF_CHANNEL).toString();
    i = channelSelector->findText(channel);
    if (i != -1)
	channelSelector->setCurrentIndex(i);
    setPlaying();
    setRecording();
    setScanning();

    ficSuccess = 0;
    ficBlocks = 0;
    if (!isFM)
	startDAB(channelSelector->currentText());
}

RadioInterface::~RadioInterface() {
}

void RadioInterface::makeDABprocessor() {
    if (inputDevice == nullptr)
	return;
    DABglobals.spectrumBuffer = nullptr;
    DABglobals.iqBuffer = &iqBuffer;
    DABglobals.responseBuffer = &responseBuffer;
    DABglobals.tiiBuffer = &tiiBuffer;

    // FIXME frameBuffer is used for the "secondService"
    DABglobals.frameBuffer = &frameBuffer;
    DABglobals.dabMode = 1;
    DABprocessor = new dabProcessor(this, inputDevice, &DABglobals);
}

// input decimating rate gets progressively higher with input rate to contain cost
static
int32_t mapRates(int32_t inputRate) {
    return inputRate %256000 == 0? 256000:
	   inputRate % 192000 == 0? 192000:
	   inputRate < 400000? inputRate:
	   inputRate < 850000? inputRate/4:
	   inputRate < 1300000? inputRate/6:
	   inputRate < 1900000? inputRate/8:
	   inputRate < 3000000? inputRate/10:
	   inputRate < 4000000? inputRate/15:
	   inputRate < 5000000? inputRate/20:
	   inputRate < 6000000? inputRate/25:
	   inputRate/30;
}

void RadioInterface::makeFMprocessor() {
    int32_t inputRate;

    if (inputDevice == nullptr)
	return;
    inputRate = inputDevice->getRate();
    fmRate = mapRates(inputRate);
    if (FMfilter <= 0)
	FMfilter = 0.95*fmRate;
    FMprocessor = new fmProcessor(inputDevice, this, inputRate, fmRate,
				  workingRate, audioRate, FMthreshold);
    FMprocessor->setSink(soundOut);
    FMprocessor->setFMRDSSelector(rdsDecoder::RDS1);
    FMprocessor->setFMRDSDemod(fmProcessor::FM_RDS_AUTO);
    FMprocessor->setFMMode(true);
    FMprocessor->setBandwidth(FMfilter);
    FMprocessor->setBandFilterDegree(FMdegree);
    FMprocessor->setFMDecoder(fm_Demodulator::fm_demod(FMdecoder));
    FMprocessor->setDeemphasis(deemphasis);
    FMprocessor->setAudioBandwidth(lowPassFilter);
    FMprocessor->setAudioGain(FMaudioGain);
}

void RadioInterface::terminateProcess() {

    // stop
    if (isFM)
	stopFM();
    else
	stopDAB();
    usleep(1000);

    // save base settings
    settings->setValue(GEN_SERVICE_NAME, currentService.serviceName);
    settings->setValue(GEN_CHANNEL, channelSelector->currentText());
    settings->setValue(GEN_FM_FREQUENCY, FMfreq);
    settings->setValue(GEN_TUNER_MODE, isFM? GEN_FM: GEN_DAB);
    settings->setValue(GEN_VOLUME, int(volumeKnob->value()));
    settings->setValue(GEN_SQUELCH, int(squelchKnob->value()));
    settings->beginWriteArray(GROUP_PRESETS);

    // skip 'presets'
    for (int i = 1; i < presetSelector->count(); i++) {
	settings->setArrayIndex(i-1);
	settings->setValue(PRESETS_NAME, presetSelector->itemText(i));
    }
    settings->endArray();
    settings->sync();

    // destruct
    delete soundOut;
    delete inputDevice;
    if (DABprocessor != nullptr)
	delete DABprocessor;
    if (FMprocessor != nullptr)
	delete FMprocessor;
    if (scanTimer != nullptr)
	delete scanTimer;
    if (settingsDialog != nullptr)
	delete settingsDialog;
}

void RadioInterface::findDevices() {
    device discoveredDevice;

#ifdef	HAVE_SDRPLAY
    try {
	    discoveredDevice.device = new sdrplayHandler();
	    discoveredDevice.deviceName = "Sdrplay";
	    discoveredDevice.controls = AGC|IF_GAIN|LNA_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef	HAVE_RTLSDR
// no LNA
    try {
	    discoveredDevice.device = new rtlsdrHandler();
	    discoveredDevice.deviceName = "RtlSdr";
	    discoveredDevice.controls = AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef	HAVE_AIRSPY
// no LNA
    try {
	    discoveredDevice.device = new airspyHandler();
	    discoveredDevice.deviceName = "AirSpy";
	    discoveredDevice.controls = AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef	HAVE_LIME
    try {
	    discoveredDevice.device = new limeHandler();
	    discoveredDevice.deviceName = "Lime";
	    discoveredDevice.controls = AGC|IF_GAIN|LNA_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef	HAVE_PLUTO
// no LNA
    try {
	    discoveredDevice.device = new plutoHandler();
	    discoveredDevice.deviceName = "Pluto";
	    discoveredDevice.controls = AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef	HAVE_HACKRF
// no AGC
    try {
	    discoveredDevice.device = new hackrfHandler();
	    discoveredDevice.deviceName = "HackRF";
	    discoveredDevice.controls =IF_GAIN|LNA_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
    if (deviceList.size() == 0)
	inputDevice = nullptr;
    else {
	inputDevice = deviceList[0].device;
	deviceName = deviceList[0].deviceName;
	deviceUiControls = deviceList[0].controls;
	settings->beginGroup(deviceName);
	if (deviceUiControls & AGC) {
		agc = (settings->value(DEV_AGC, DEV_DEF_AGC).toInt() > 0);
		inputDevice->setAgcControl(agc);
	}
	if (deviceUiControls & IF_GAIN) {
		ifGain = settings->value(DEV_IF_GAIN, DEV_DEF_IF_GAIN).toInt();
		inputDevice->setIfGain(ifGain);
	}
	if (deviceUiControls & LNA_GAIN) {
		lnaGain = settings->value(DEV_LNA_GAIN, DEV_DEF_LNA_GAIN).toInt();
		inputDevice->setLnaGain(lnaGain);
	}
	settings->endGroup();
    }
}

/**
  *	If a change is detected, we have to restart the selected
  *	service - if any. If the service is a secondary service,
  *	it might be the case that we have to start the main service
  *	how do we find that?
  */
void RadioInterface::changeInConfiguration() {
    dabService s;

    if (currentService.valid) { 
	s = currentService;
	stopDABService();
    }

    // we rebuild the services list from the fib and
    // then we (try to) restart the service
    serviceList = DABprocessor->getServices(serviceOrder);
    ensembleModel.clear();
    for (const auto serv:serviceList) {
	ensembleModel.appendRow(new QStandardItem(serv.name));
	for (int i = 0; i < ensembleModel.rowCount(); i ++) {
	    ensembleModel.setData(ensembleModel.index(i, 0),
			QFont ("Cantarell", 11), Qt::FontRole);
	}
	ensembleDisplay->setModel(&ensembleModel);
    }

    // and restart the one that was running
    if (s.valid) {
	if (s.SCIds != 0) {			// secondary service may be gone
	    if (DABprocessor->findService(s.SId, s.SCIds) == s.serviceName) {
		startDABService(&s);
		return;
	    } else {
		s.SCIds = 0;
	        s.serviceName = DABprocessor->findService(s.SId, s.SCIds);
	    }
	}

	// checking for the main service
	if (s.serviceName != DABprocessor->findService(s. SId, s.SCIds)) {
	    warning(this, tr(BAD_SERVICE));
            return;
        }
	startDABService (&s);
    }
}

//	In order to not overload with an enormous amount of
//	signals, we trigger this function at most 10 times a second
void RadioInterface::newAudio(int amount, int rate) {
    int16_t vec[amount];

    while (audioBuffer.GetRingBufferReadAvailable() > amount) {
	audioBuffer.getDataFromBuffer(vec, amount);
	soundOut->audioOut(vec, amount, rate);
    }
}

//	scans

//	controls

void RadioInterface::startFMscan(bool down) {
    if (down) {
	if (FMfreq <= MIN_FM)
	    return;
	scanIncrement = -1;
    } else {
	if (FMfreq >= MAX_FM)
	    return;
	scanIncrement = 1;
    }
    if (scanTimer == nullptr) {
	scanTimer = new QTimer();
	scanTimer->setInterval(scanInterval);
	connect(scanTimer, SIGNAL(timeout()),
		this, SLOT(nextFrequency()));
    }
    stopFM();
    playing = false;
    scanning = true;
    cleanScreen();
    setPlaying();
    setRecording();
    setScanning();
    FMfreq = (FMfreq*10+scanIncrement)/10;
    frequencyKnob->setValue(double(FMfreq));
    frequencyLCD->display(int(FMfreq*1000));
    inputDevice->restartReader(int(FMfreq*1000000));
    FMprocessor->start();
    FMprocessor->startScanning();
    scanTimer->start();
}

// FIXME ideally stopping a scan and switching frequency should be serialized
void RadioInterface::stopFMscan() {
    if (scanTimer != nullptr && scanTimer->isActive())
	scanTimer->stop();
    scanning = false;
    FMprocessor->stopScanning();
    stopFM();
    setPlaying();
    setRecording();
    setScanning();
}

//	slots

void RadioInterface::nextFrequency(void) {
    if (FMfreq <= MIN_FM || FMfreq >= MAX_FM) {
	stopFMscan();
    } else {
	FMprocessor->stopScanning();
	FMprocessor->stop();
	inputDevice->stopReader();
	FMfreq = (FMfreq*10+scanIncrement)/10;
	frequencyKnob->setValue(double(FMfreq));
	frequencyLCD->display(int(FMfreq*1000));
	inputDevice->restartReader(int(FMfreq*1000000));
	FMprocessor->start();
	FMprocessor->startScanning();
	scanTimer->start();
    }
}

void RadioInterface::scanDone(void) {
    scanTimer->stop();
    FMprocessor->stopScanning();
    scanning = false;
    stopFM();
    playing = true;
    startFM(int(FMfreq*1000000));
    setPlaying();
    setRecording();
    setScanning();
}

//	visual elements

//	slots exercised by the processors

void RadioInterface::addToEnsemble(const QString &serviceName, int32_t SId) {
    serviceId ed;
    int lastId = 0;
    QString lastName = "";
    bool inserted = false;
    std::vector<serviceId>::iterator i;

    if (!DABprocessor->is_audioService(serviceName))
       return;
    for (const auto serv: serviceList)
	if (serv.name == serviceName)
	    return;

    ed.name = serviceName;
    ed.SId = SId;

    if (serviceList.size() == 0) {
	serviceList.push_back(ed);
	inserted = true;
    } else if (serviceOrder == ID_BASED) {
	for (i=serviceList.begin(); i<serviceList.end(); i++) {
	    if (lastId < SId && SId <= (int) i->SId) {
	    	serviceList.insert(i, ed);
		inserted = true;
		break;
	    }
	    lastId = i->SId;
	}
    } else {
	for (i=serviceList.begin(); i<serviceList.end(); i++) {
	    if (lastName < serviceName && serviceName <= i->name) {
	    	serviceList.insert(i, ed);
		inserted = true;
		break;
	    }
	    lastName = i->name;
	}
    }
    if (!inserted)
	serviceList.push_back(ed);

    ensembleModel.clear();
    for (const auto serv: serviceList)
	ensembleModel.appendRow(new QStandardItem(serv.name));
    for (int i = 0; i < ensembleModel.rowCount(); i++) {
	if (currentService.valid && serviceList.at(i).name == currentService.serviceName)
	    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
	ensembleModel.setData(ensembleModel.index(i, 0), QFont("Cantarell", 11), Qt::FontRole);
    }

    ensembleDisplay->setModel(&ensembleModel);
    if (nextService.valid && nextService.serviceName == serviceName) {
	startDABService(&nextService);
	nextService.valid = false;
    }
}

void RadioInterface::nameOfEnsemble(int id, const QString &v) {
    ensembleId->setAlignment(Qt::AlignLeft);
    ensembleId->setText(v + " (" + QString::number(id, 16) + ")");
}

void RadioInterface::showStrength(float strength) {
    if (strength < 50)
	signalStrength->setFillBrush(QBrush(Qt::red));
    else
	signalStrength->setFillBrush(QBrush(Qt::green));
    signalStrength->setValue(strength);
}

void RadioInterface::showQuality(bool b) {
    if (b) 
	ficSuccess++;
    if (++ficBlocks >= 25) {
	if (ficSuccess > 12) {
	    signalQuality->setFillBrush(QBrush(Qt::green));
	} else {
	    signalQuality->setFillBrush(QBrush(Qt::red));
	}
	signalQuality->setValue(ficSuccess*4);
	ficBlocks = 0;
	ficSuccess = 0;
    }
}

void RadioInterface::showText(QString s) {
    dynamicLabel->setText(s);
}

void RadioInterface::showSoundMode(bool s) {
    stereoLabel->setStyleSheet (s?  "QLabel {background-color: green; color: white}":
                         "QLabel {background-color: red; color: white}");
    stereoLabel->setText(s? "stereo" : "mono");
}

void RadioInterface::showLabel(const QString s) {
    serviceLabel->setAlignment(Qt::AlignLeft);
    serviceLabel->setText(s);
}

// 	close button
void RadioInterface::closeEvent(QCloseEvent *event) {
    if (yesNo(this)) {
	terminateProcess();
	event->accept();
    } else 
	event->ignore();
}

//	audio

void RadioInterface::handleVolume(double vol) {
    soundOut->setVolume(qreal(vol)/100);
}

void RadioInterface::handleSquelch(double val) {
    FMprocessor->setSquelchValue(100-int(val));
}

//	preset selection

static
void addPreset(QComboBox *presetSelector, QString preset, QString comment) {

    // don't add duplicates
    for (int i = 0; i < presetSelector->count(); i ++)
	if (presetSelector->itemText(i).contains(preset))
	    return;
    if (comment != "")
	preset = preset + ":" + comment;
    presetSelector->addItem(preset);
}

static
void deletePreset(QComboBox *presetSelector, QString preset) {
    for (int i = 0; i < presetSelector->count(); i ++)
	if (presetSelector->itemText(i).contains(preset)) {
	    presetSelector->removeItem(i);
	    return;
	}
}

void RadioInterface::handlePresetSelector(const QString &preset) {
    if ((preset == "Presets") || (presetSelector->currentIndex() == 0))
	return;

    QStringList list = preset.split(":", QString::SkipEmptyParts);
    if (list.length() != 2 && list.length() != 3) {
	warning(this, tr(BAD_PRESET));
	deletePreset(presetSelector, preset);
	return;
    }
    playing = false;
    stopRecording();

    if (list.at(0) == "FM") {
	bool ok;
	double newFreq;

	if (!isFM) {
	    stopDAB();
	    toFM();
	} else
	    stopFM();

	newFreq = list.at(1).toDouble(&ok);
	if (ok && newFreq > MIN_FM && newFreq < MAX_FM) {
		FMfreq = newFreq;
	        frequencyKnob->setValue(double(FMfreq));
	        frequencyLCD->display(int(FMfreq*1000));
		startFM(int(FMfreq*1000000));
		playing = true;
	} else {
		warning(this, tr(BAD_PRESET));
		deletePreset(presetSelector, preset);
	}
	setPlaying();
	setRecording();
	return;
    }

    QString channel = list.at(0);
    QString service = list.at(1);

    // preset is current channel
    if (!isFM && channel == channelSelector->currentText()) {
	dabService s;

	stopDABService();
	DABprocessor->getParameters(service, &s.SId, &s.SCIds);
	if (s.SId == 0) {
            warning(this, tr(BAD_PRESET));
	    deletePreset(presetSelector, preset);
            return;
	}

	s.serviceName = service;
	startDABService(&s);
	return;
    }

    // have to start channel first
    disconnect (channelSelector, SIGNAL(activated(const QString &)),
	            this, SLOT(handleSelectChannel(const QString &)));
    int k = channelSelector->findText(channel);

    if (k != -1)
	channelSelector->setCurrentIndex(k);
    else {
	warning(this, tr(BAD_PRESET));
	deletePreset(presetSelector, preset);
    }
    connect(channelSelector, SIGNAL(activated(const QString &)),
	         this, SLOT(handleSelectChannel(const QString &)));
    if (k == -1)
	return;

    toDAB();
    stopDAB();
    nextService.valid = true;
    nextService.serviceName = service;
    nextService.SId = 0;
    nextService.SCIds = 0;

    // the preset service will be picked up by the addToEnsemble() slot
    startDAB(channelSelector->currentText());
}

void RadioInterface::handleAddDABPreset() {
    if (currentService.serviceName.at(1) == ' ')
	return;

    addPreset(presetSelector, channelSelector->currentText() + ":" + currentService.serviceName, "");
}

void RadioInterface::handleDeleteDABPreset() {
    if (currentService.serviceName.at(1) == ' ')
	return;

    deletePreset(presetSelector, channelSelector->currentText() + ":" + currentService.serviceName);
}

void RadioInterface::handleAddFMPreset() {
    addPreset(presetSelector, "FM:" + QString::number(FMfreq), serviceLabel->text().trimmed());
}

void RadioInterface::handleDeleteFMPreset() {
    deletePreset(presetSelector, "FM:" + QString::number(FMfreq));
}

//	DAB services

void RadioInterface::stopDABService() {
    presetSelector->setCurrentIndex (0);
    if (currentService.valid) {
	audiodata ad;

	DABprocessor->dataforAudioService(currentService.serviceName, &ad);
	DABprocessor->stopService(&ad);
	usleep(1000);
	soundOut->stop();
	playing = false;
	stopRecording();
	setPlaying();
	setRecording();
    }
    currentService.valid = false;
    cleanScreen();
}

void RadioInterface::handleSelectService(QModelIndex ind) {
    dabService s;

    QString currentProgram = ind.data(Qt::DisplayRole).toString();
    if (currentProgram == "")
	return;

    stopDABService();
    DABprocessor->getParameters(currentProgram, &s. SId, &s.SCIds);
    if (s.SId == 0)
	   warning(this, tr(BAD_SERVICE));	
    else {
	s.serviceName = currentProgram;
	startDABService(&s);
    }
}

//	DAB OPs

void RadioInterface::startDABService(dabService *s) {
    QString serviceName = s->serviceName;

    if (playing && !isFM && currentService.valid) {
	fprintf (stderr, "service %s is still valid\n",
		currentService.serviceName.toLatin1().data());
	stopDABService();
    }

    ficBlocks = 0;
    ficSuccess = 0;
    currentService = *s;
    currentService.valid = false;
    for (int i = 0; i < (int) ensembleModel.rowCount(); i ++) {
	QString itemText = ensembleModel.index(i, 0).data(Qt::DisplayRole).toString ();
	if (itemText == serviceName) {
	    audiodata ad;

	    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
	    serviceLabel->setStyleSheet("QLabel {color: black}");
	    serviceLabel->setText (serviceName);
	    if (DABprocessor->is_audioService(serviceName)) {
		DABprocessor->dataforAudioService (serviceName, &ad);
		if (!ad.defined)
		    warning(this, tr(BAD_SERVICE));
		else {
		    ad.procMode = __ONLY_SOUND;
	 	    DABprocessor->set_audioChannel (&ad, &audioBuffer);
		    soundOut->restart ();
		}
	        currentService.valid = true;
	        currentService.serviceName = serviceName;
		playing = true;
		setPlaying();
		setRecording();
	    } else
		warning(this, tr(BAD_SERVICE));
	    return;
	}
    }
}

void RadioInterface::startDAB(const QString &channel) {
    int tunedFrequency = DABband.Frequency(channel);

    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    inputDevice->restartReader(tunedFrequency);
    DABprocessor->start(tunedFrequency);
}

void RadioInterface::stopDAB() {
    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    playing = false;
    stopRecording();
    setPlaying();
    setRecording();
    soundOut->stop();
    ficSuccess = 0;
    ficBlocks = 0;
    presetSelector->setCurrentIndex(0);

    // the service if any - is stopped by halting the DAB processor
    DABprocessor->stop();
    inputDevice->stopReader();
    usleep(1000);
    currentService.valid = false;
    nextService.valid = false;
    serviceList.clear();
    ensembleModel.clear();
    ensembleDisplay->setModel(&ensembleModel);
    cleanScreen();
}

void RadioInterface::handleSelectChannel(const QString &channel) {
    stopDAB();
    startDAB(channel);
}

void RadioInterface::handleNextChanButton() {
    int i;
    int currentChannel = channelSelector->currentIndex();

    // if there's a current service and it's not the last, jump service
    if (currentService.serviceName != "")
	for (i = 0; i < (int)(serviceList.size()-1); i++)
	    if (serviceList.at(i).name == currentService.serviceName) {
		dabService s;

		i++;
		if (playing) {
		    s.serviceName = serviceList.at(i).name;
		    DABprocessor->getParameters(s.serviceName, &s.SId, &s.SCIds);
	 	    if (s.SId == 0)
		    	warning(this, tr(BAD_SERVICE));
	       	    else
		    	startDABService(&s);
		} else {
		    currentService.serviceName = serviceList.at(i).name;
		    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
		}
		return;
	    }

    // no dice, switch channel
    stopDAB();
    currentChannel++;
    if (currentChannel >= channelSelector->count())
	currentChannel = 0;
    disconnect (channelSelector, SIGNAL(activated (const QString &)),
		this, SLOT(handleSelectChannel(const QString &)));
    channelSelector->setCurrentIndex(currentChannel);
    connect (channelSelector, SIGNAL(activated (const QString &)),
		this, SLOT(handleSelectChannel(const QString &)));
    startDAB(channelSelector->currentText());
    currentService.valid = false;
}

void RadioInterface::handlePrevChanButton() {
    int i;
    int currentChannel = channelSelector->currentIndex();

    // if there's a current service and it's not the last, jump service
    if (currentService.serviceName != "")
	for (i = 1; i < (int)(serviceList.size()); i++)
	    if (serviceList.at(i).name == currentService.serviceName) {
		dabService s;

		i--;
		if (playing) {
		    s.serviceName = serviceList.at(i).name;
		    DABprocessor->getParameters(s.serviceName, &s.SId, &s.SCIds);
	 	    if (s.SId == 0)
		    	warning(this, tr(BAD_SERVICE));
	       	    else
		    	startDABService(&s);
		} else {
		    currentService.serviceName = serviceList.at(i).name;
		    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
		}
		return;
	    }

    // no dice, switch channel
    stopDAB();
    currentChannel--;
    if (currentChannel < 0)
	currentChannel = channelSelector->count()-1;
    disconnect(channelSelector, SIGNAL (activated(const QString &)),
		this, SLOT(handleSelectChannel(const QString &)));
    channelSelector->setCurrentIndex(currentChannel);
    connect(channelSelector, SIGNAL(activated(const QString &)),
		this, SLOT(handleSelectChannel(const QString &)));
    startDAB(channelSelector->currentText());
    currentService.valid = false;
}

//	FM ops

void RadioInterface::startFM(int32_t freq) {
    if (inputDevice == nullptr || FMprocessor == nullptr)
	return;
    ficBlocks = 0;
    ficSuccess = 0;
    FMprocessor->resetRDS();
    inputDevice->restartReader(freq);
    soundOut->restart();
    FMprocessor->start();
    playing = true;
    recording = false;
    setPlaying();
    setRecording();
}

void RadioInterface::stopFM() {
    if (inputDevice == nullptr || FMprocessor == nullptr)
	return;
    if (scanning)
	stopFMscan();
    ficBlocks = 0;
    ficSuccess = 0;
    soundOut->stop();
    inputDevice->stopReader();
    FMprocessor->stop();
    playing = false;
    scanning = false;
    stopRecording();
    setPlaying();
    setRecording();
    setScanning();
    cleanScreen();
}

void RadioInterface::handleScanDown() {
    startFMscan(true);
}

void RadioInterface::handleScanUp() {
    startFMscan(false);
}

void RadioInterface::handleStopScan() {
    stopFMscan();
}

void RadioInterface::setScanning() {
    if (scanning) {
	scanBackButton->setEnabled(false);
	scanForwardButton->setEnabled(false);
	addPresetButton->setEnabled(false);
	deletePresetButton->setEnabled(false);
	frequencyKnob->setEnabled(false);
	stopScanButton->setEnabled(true);
    } else {
	scanBackButton->setEnabled(true);
	scanForwardButton->setEnabled(true);
	addPresetButton->setEnabled(true);
	deletePresetButton->setEnabled(true);
	frequencyKnob->setEnabled(true);
	stopScanButton->setEnabled(false);
    }
}

void RadioInterface::handleFMfrequency(double freq) {
    FMfreq = freq;
    frequencyLCD->display(int(FMfreq*1000));
    if (playing) {
	stopFM();
	startFM(int(FMfreq*1000000));
    }
}

//	play, pause, record, stop record

void RadioInterface::handlePlayButton() {
    disconnect(playButton, SIGNAL (clicked (void)),
                 this, SLOT (handlePlayButton(void)));
    if (isFM)
	startFM(int(FMfreq*1000000));
    else
	startDABService(&currentService);
}

void RadioInterface::handlePauseButton() {
    disconnect(playButton, SIGNAL(clicked(void)),
	this, SLOT(handlePauseButton(void)));
    if (isFM)
	stopFM();
    else
	stopDABService();
}

void RadioInterface::setPlaying() {
    if (inputDevice != nullptr && ((isFM && !scanning) || (!isFM && currentService.valid))) {
	playButton->setEnabled(true);
	if (playing) {
	    connect(playButton, SIGNAL(clicked (void)),
			this, SLOT(handlePauseButton(void)));
	    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	    playButton->setToolTip("Stop playback");
	} else {
	    connect(playButton, SIGNAL(clicked (void)),
			this, SLOT(handlePlayButton(void)));
	    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	    playButton->setToolTip("Start playback");
	}
    } else {
	playButton->setToolTip("");
	playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	playButton->setEnabled(false);
    }
}

void RadioInterface::handleRecordButton() {
    QString outputFileName;
    QString baseFileName;
    QString ext;
    SF_INFO sfInfo;

    if (isFM)
	baseFileName = "FM:" + QString::number(FMfreq*1000);
    else if (currentService.valid)
	baseFileName = currentService.serviceName;
    else
	baseFileName = "";
    if ((outputFileName = chooseFileName(this, settings, tr("Record to"),
		tr("Waveform Audio File (*.wav);; Free Lossless Audio Codec (*.flac);; Ogg Vorbis (*.ogg)"),
		DIALOGS_RECORDING, baseFileName.trimmed())) != "") {
	memset(&sfInfo, 0, sizeof(sfInfo));
	sfInfo.samplerate = audioRate;
	sfInfo.channels = 2;
	ext = QFileInfo(outputFileName).suffix().toLower();
	if (ext == "wav")
	    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	else if (ext == "flac")
	    sfInfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
	else if (ext == "ogg")
	    sfInfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

	// sf_open fails if the format wasn't set properly
	// we use this for unknown extension types
	recordingFile = sf_open(outputFileName.toUtf8().data(), SFM_WRITE, &sfInfo);
	if (recordingFile != nullptr) {
	    if (!playing)
		handlePlayButton();
	    recording = true;
	    soundOut->startDumping(recordingFile);
	} else
	    warning(this, tr("could not start recording"));
    }
    disconnect(recordButton, SIGNAL(clicked(void)),
		this, SLOT(handleRecordButton(void)));
    setRecording();
}

void RadioInterface::handleStopRecordButton() {
    disconnect(recordButton, SIGNAL(clicked(void)),
		this, SLOT(handleStopRecordButton(void)));
    stopRecording();
    setRecording();
}

void RadioInterface::stopRecording() {
    if (recording) {
	soundOut->stopDumping();
	sf_close(recordingFile);
	recordingFile = nullptr;
	recording = false;
    }
}

void RadioInterface::setRecording() {
    if (inputDevice != nullptr && ((isFM && !scanning) || (!isFM && currentService.valid))) {
	recordButton->setEnabled(true);
	if (recording) {
	    connect(recordButton, SIGNAL(clicked(void)),
			this, SLOT(handleStopRecordButton(void)));
	    recordButton->setIcon(QIcon(":/recordbutton.png"));
	    recordButton->setToolTip ("Stop recording");
    	} else {
	    connect(recordButton, SIGNAL (clicked (void)),
			this, SLOT (handleRecordButton(void)));
	    recordButton->setIcon(QIcon(":/recordbuttonred.png"));
	    recordButton->setToolTip("Start recording");
    	}
    } else {
    	recordButton->setIcon(QIcon(":/recordbutton.png"));
    	recordButton->setEnabled(false);
    	recordButton->setToolTip("");
    }
}

//	dab and fm buttons

void RadioInterface::handleDABButton() {
    if (!isFM)
	return;
    stopFM();
    stopRecording();
    playing = false;
    scanning = false;
    toDAB();
    currentService.valid = (currentService.serviceName != "");
    startDAB(channelSelector->currentText());
    setPlaying();
    setRecording();
    setScanning();
}

void RadioInterface::toDAB() {
    isFM = false;
    disconnect(addPresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleAddFMPreset(void)));
    disconnect(deletePresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleDeleteFMPreset(void)));
    fmWidget->hide();
    dabWidget->show();
    squelchKnob->setEnabled(false);
    connect(addPresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleAddDABPreset(void)));
    connect(deletePresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleDeleteDABPreset(void)));
    FMButton->setChecked(false);
    DABButton->setChecked(true);
}

void RadioInterface::handleFMButton() {
    if (isFM)
	return;
    stopDAB();
    stopRecording();
    playing = false;
    toFM();
    setPlaying();
    setRecording();
}

void RadioInterface::toFM() {
    isFM = true;
    disconnect(addPresetButton, SIGNAL(clicked(void)),
		this, SLOT (handleAddDABPreset(void)));
    disconnect(deletePresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleDeleteDABPreset(void)));
    dabWidget->hide();
    fmWidget->show();
    squelchKnob->setEnabled(true);
    connect(addPresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleAddFMPreset(void)));
    connect(deletePresetButton, SIGNAL(clicked (void)),
		this, SLOT(handleDeleteFMPreset(void)));
    DABButton->setChecked(false);
    FMButton->setChecked(true);
}

// utility

void RadioInterface::cleanScreen() {
    serviceLabel->clear();
    ensembleId->clear();
    dynamicLabel->clear();
    presetSelector->setCurrentIndex(0);
    stereoLabel->setStyleSheet(stereoStyle);
    stereoLabel->clear();
    signalQuality->setValue(0);
    signalStrength->setValue(0);
}

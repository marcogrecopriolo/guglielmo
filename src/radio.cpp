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
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QPainter>
#include <fstream>
#include "constants.h"
#include "settings.h"
#include "fm-processor.h"
#include "fm-demodulator.h"
#include "rds-decoder.h"
#include "dab-tables.h"
#include <iostream>
#include <numeric>
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
#ifdef	HAVE_SDRPLAY_V3
#include "sdrplay-handler-v3.h"
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
#include "logging.h"

// A few repeated Ui constants
#define BAD_SERVICE	"cannot run this service"
#define BAD_PRESET	"this preset is not valid"

// Some software AGC constants
#define SW_AGC_SKIP_COUNT 16
#define SW_AGC_BYTES 65536

// Text buffers
#define INFOBUFLEN 100

// most buffer not used locally, but within the DAB processor
RadioInterface::RadioInterface(QSettings *Si, QWidget *parent):
	QWidget(parent),
	iqBuffer(2 * 1536),
	tiiBuffer(32768),
	responseBuffer(32768),
	frameBuffer(2 * 32768),
	audioBuffer(8 * 32768),
#ifdef USE_SPI
	dataBuffer(32768),
#endif
	DABband() {

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
    originalTitle = windowTitle();

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
    stationsAction = new QAction(QIcon(":/stations.png"), "sTations");
    stationsAction->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_T));
    slidesAction = new QAction(QIcon(":/slides.png"), "sLides");
    slidesAction->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_L));
    aboutAction = new QAction(QIcon(":/copyright.png"), "About");
    aboutAction->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_A));
    settingsAction = new QAction(QIcon(":/settings.png"), "Settings");
    settingsAction->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_S));
    menu->addAction(stationsAction);
    menu->addAction(slidesAction);
    menu->addAction(settingsAction);
    menu->addAction(aboutAction);
    menu->setLayoutDirection(Qt::LeftToRight);
    menuButton->setMenu(menu);
    stopScanButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    DABButton->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_D));
    FMButton->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_F));
    playButton->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_P));
    recordButton->setShortcut(QKeySequence(KEY_MODIFIER | Qt::Key_R));
    presetSelector->view()->setMinimumWidth(ensembleDisplay->width());
    stationSelector->view()->setMinimumWidth(ensembleDisplay->width());

    // setting the background to transparent hides the label, so we 
    // store the initial style instead
    stereoStyle = stereoLabel->styleSheet();

    serviceList.clear();
    ensembleModel.clear();
    ensembleDisplay->setModel(&ensembleModel);
    stationSelector->setModel(&ensembleModel);

#ifdef HAVE_MPRIS
    player.setServiceName(QString(TARGET));
    player.setIdentity(QString(TARGET));
#endif

    // Settings
    // UI
    settings->beginGroup(GROUP_UI);
    theme = settings->value(UI_THEME, UI_DEF_THEME).toString().toLower();
    if (theme != "") {
	log(LOG_UI, LOG_MIN, "applying theme %s", qPrintable(theme));
	QApplication::setStyle(theme);
    }
    skin = settings->value(UI_SKIN, UI_DEF_SKIN).toString();
    skinIsLocal = settings->value(UI_SKIN_LOCAL, UI_DEF_SKIN_LOCAL).toInt() != 0;
    settings->endGroup();
    qApp->setStyleSheet(loadSkin());

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
	if (!((audioSink *) soundOut)->selectDevice(soundChannel)) {
	    soundChannel = ((audioSink *) soundOut)->defaultDevice();
	    ((audioSink *) soundOut)->selectDevice(soundChannel);
	}
    }
    settings->endGroup();

    // base settings
    lastVolume = settings->value(GEN_VOLUME, GEN_DEF_VOLUME).toInt();
    if (lastVolume < 0)
	lastVolume = 0;
    else if (lastVolume > AUDIO_SCALE)
	lastVolume = AUDIO_SCALE;
    soundOut->setVolume(qreal(lastVolume)/AUDIO_SCALE);
    volumeKnob->setValue(double(lastVolume));
#ifdef HAVE_MPRIS
    lastPreset = settings->value(GEN_LAST_PRESET, 1).toInt();
    skipPresetMode = settings->value(GEN_SKIP_PRESET_MODE, true).toBool();
#endif
    deviceType = settings->value(GEN_DEVICE_TYPE, "").toString();
    scanInterval = settings->value(GEN_SCAN_INTERVAL, GEN_DEF_SCAN_INTERVAL).toInt();
    scanRetry = settings->value(GEN_SCAN_RETRY, GEN_DEF_SCAN_RETRY).toInt();

    findDevices();
    DABband.setupChannels(BAND_III);
    for (;;) {
	std::string channel = DABband.nextChannel();

	if (channel.length() == 0)
	    break;
	channelSelector->addItem(QString::fromStdString(channel));
    }
    nextService.valid = false;
    dataService.valid = false;
    currentService.serviceName = settings->value(GEN_SERVICE_NAME, "").toString();

    // FIXME we don't know that it's valid yet
    currentService.valid = currentService.serviceName != "";
    FMfreq = settings->value(GEN_FM_FREQUENCY, DEF_FM).toDouble();
    if (FMfreq < MIN_FM)
	FMfreq = MIN_FM;
    else if (FMfreq > MAX_FM)
	FMfreq = MAX_FM;
    frequencyKnob->setValue(double(FMfreq));
    frequencyLCD->display(int(FMfreq*1000));

    // presets
    presetSelector->addItem("Presets");
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
    else if (lastSquelch > AUDIO_SCALE)
	lastSquelch = AUDIO_SCALE;
    squelchKnob->setValue(double(lastSquelch));
    if (FMprocessor != nullptr) {
	FMprocessor->setSquelchValue(AUDIO_SCALE-int(lastSquelch));
    }

    playing = false;
    recording = false;
    scanning = false;
    isFM = (settings->value(GEN_TUNER_MODE, GEN_DEF_TUNER_MODE).toString() == GEN_FM);
    if (settings->value(GEN_DAB_MODE, GEN_DEF_DAB_MODE).toString() == GEN_DAB_SLIDES)
	dabDisplay = DD_SLIDES;
    else
	dabDisplay = DD_STATIONS;
    dabDisplayOff();
    if (isFM)
	toFM();
    else
	toDAB();

    qApp->installEventFilter(this);
    connect(aboutAction, SIGNAL(triggered(bool)),
		this, SLOT(handleAboutAction()));
    connect(settingsAction, SIGNAL(triggered(bool)),
		this, SLOT(handleSettingsAction()));
    connect(stationsAction, SIGNAL(triggered(bool)),
		this, SLOT(handleStationsAction()));
    connect(slidesAction, SIGNAL(triggered(bool)),
		this, SLOT(handleSlidesAction()));
    connect(leftDisplayButton, SIGNAL(clicked(void)),
		this, SLOT(handleLeftDisplayButton()));
    connect(rightDisplayButton, SIGNAL(clicked(void)),
		this, SLOT(handleRightDisplayButton()));
    connect(presetSelector, SIGNAL(activated(int)),
		this, SLOT(handlePresetSelector(int)));
    connect(ensembleDisplay, SIGNAL(clicked(QModelIndex)),
		this, SLOT(handleSelectService(QModelIndex)));
    connect(stationSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectService(int)));
    connect(channelSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectChannel(int)));
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
    connect(volumeKnob, SIGNAL(valueChanged(double)),
		this, SLOT(handleVolume(double)));
    connect(squelchKnob, SIGNAL(valueChanged(double)),
		this, SLOT(handleSquelch(double)));
    connect(this, SIGNAL(newSkin(QString)),
		this, SLOT(changeSkin(QString)));

#ifdef HAVE_MPRIS
    player.setCanControl(true);
    player.setVolume(double(lastVolume) / AUDIO_SCALE);
    player.setCanQuit(true);
    player.setCanPlay(true);
    player.setCanPause(true);
    player.setCanGoNext(true);
    player.setCanGoPrevious(true);
    player.setPlaybackStatus(Mpris::Stopped);
    connect(&player, SIGNAL(volumeRequested(double)), this, SLOT(mprisVolume(double)));
    connect(&player, SIGNAL(quitRequested()), this, SLOT(mprisClose()));
    connect(&player, SIGNAL(playRequested()), this, SLOT(mprisPlayButton()));
    connect(&player, SIGNAL(pauseRequested()), this, SLOT(mprisPauseButton()));
    connect(&player, SIGNAL(playPauseRequested()), this, SLOT(mprisPlayPause()));
    connect(&player, SIGNAL(stopRequested()), this, SLOT(mprisPauseButton()));
    connect(&player, SIGNAL(nextRequested()), this, SLOT(mprisNextButton()));
    connect(&player, SIGNAL(previousRequested()), this, SLOT(mprisPreviousButton()));
    player.setMetadata(metadata);
#endif

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
	startDAB();
}

RadioInterface::~RadioInterface() {

    // clear the model before deleting listview and combobox
    // to avoid double references and double frees
    ensembleModel.clear();
    ensembleDisplay->setModel(&ensembleModel);
    stationSelector->setModel(&ensembleModel);
    delete soundOut;
    if (DABprocessor != nullptr)
	delete DABprocessor;
    if (FMprocessor != nullptr)
	delete FMprocessor;
    if (scanTimer != nullptr)
	delete scanTimer;
    if (settingsDialog != nullptr)
	delete settingsDialog;
    for (const auto &dev: deviceList)
	delete dev.device;
}

QString extractStyleName(const QString& text)
{
    static const QRegularExpression re(
	R"(/\*\+\s*style\s*:\s*([^\*\s]+)\s*\*/)",
	QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch())
	return match.captured(1).toLower();

    return QString();
}

QString RadioInterface::loadSkin() {
    if (skin == "") {
	log(LOG_UI, LOG_MIN, "no skin in use");
	this->setAttribute(Qt::WA_StyledBackground, false);
	return QString("");
    }
    QString filename = QString("%1/skins/%2/%3.qss").arg(
	skinIsLocal? ":": QDir::home().absoluteFilePath(LOCAL_STORAGE), skin, skin);
    QFile file(filename);
    QString styleSheet;
    QString embeddedStyle;

    log(LOG_UI, LOG_MIN, "loading skin %s", qPrintable(filename));
    if (file.open(QFile::ReadOnly)) {
	QString storageDir = QDir::home().absoluteFilePath(LOCAL_STORAGE);
	this->setAttribute(Qt::WA_StyledBackground, true);
	styleSheet = QLatin1String(file.readAll());
	styleSheet.replace("${APP_DATA_DIR}", storageDir);
	embeddedStyle = extractStyleName(styleSheet);
	if (embeddedStyle != "") {
	    log(LOG_UI, LOG_MIN, "switching theme to %s", qPrintable(embeddedStyle));
	    theme = embeddedStyle;
	    QApplication::setStyle(theme);
	    settings->beginGroup(GROUP_UI);
	    settings->setValue(UI_THEME, theme);
	    settings->endGroup();
	}
    } else {
	this->setAttribute(Qt::WA_StyledBackground, false);
	log(LOG_UI, LOG_MIN, "loading skin %s failed, switchng to none", qPrintable(filename));
	skin = QString("");
	skinIsLocal = true;
	settings->beginGroup(GROUP_UI);
	settings->setValue(UI_SKIN, skin);
	settings->setValue(UI_SKIN_LOCAL, skinIsLocal);
	settings->endGroup();
	return QString("");
    }
    return styleSheet;
}

void RadioInterface::changeSkin(QString skin) {
    log(LOG_UI, LOG_MIN, "received newSkin");
    qApp->setStyleSheet(skin);
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}

void RadioInterface::processGain(agcStats *newStats, int amount) {
    int oldAgc = swAgc;

    if ((agc != AGC_SOFTWARE && agc != AGC_COMBINED) && !log_active(LOG_AGC, LOG_VERBOSE))
	return;
    swAgcAmount += amount;
    if (swAgcAmount < SW_AGC_BYTES)
	return;
    log(LOG_AGC, LOG_VERBOSE, "skip %i amount %i min %i max %i overflows %i",
	swAgcSkip+swAgcAccrue, swAgcAmount, stats.min, stats.max, stats.overflows);

    // we most likely already have data in the buffer, which will not be yet
    // affected by our gain change, so skip some buffers before retesting:
    // this avoids AGC induced signal oscillations
    if (swAgcSkip > 0) {
	swAgcAmount = 0;
	swAgcSkip--;
	return;
    }
    if (newStats->min < stats.min)
	stats.min = newStats->min;
    if (newStats->max > stats.max)
	stats.max = newStats->max;
    stats.overflows += newStats->overflows;

    if (swAgcAccrue > 0) {
	swAgcAmount = 0;
	swAgcAccrue--;
	return;
    }
    if (agc != AGC_SOFTWARE && agc != AGC_COMBINED) {
	resetAgcStats();
	return;
    }

    // if we have overflows, the signal is too strong
    // otherwise we change the gain to try to land the signal strength
    // between a high enough percentile and one near the top: after that,
    // empirically, AGC settles
    if ((stats.overflows > 0 || stats.max-stats.min > maxSignal) && swAgc > minIfGain)
	swAgc--;
    else if (stats.max-stats.min < minSignal && swAgc < maxIfGain)
	swAgc++;
    if (swAgc != oldAgc) {
	log(LOG_AGC, LOG_MIN, "switching gain to %i (min %i max %i overflows %i)",
			swAgc, stats.min, stats.max, stats.overflows);
	inputDevice->setIfGain(swAgc);
	swAgcSkip = SW_AGC_SKIP_COUNT;
    }
    resetAgcStats();
}

void RadioInterface::resetAgcStats() {
    swAgcAmount = 0;
    swAgcAccrue = SW_AGC_SKIP_COUNT;
    stats.min = INT_MAX;
    stats.max = 0;
    stats.overflows = 0;
}

void RadioInterface::resetSwAgc() {
    swAgc = ifGain;
    swAgcSkip = SW_AGC_SKIP_COUNT;
    if (inputDevice)
	 inputDevice->getSwAGCRange(&minSignal, &maxSignal);
    else {
	minSignal = 0;
	maxSignal = 0;
    }
}

void RadioInterface::makeDABprocessor() {
    if (inputDevice == nullptr)
	return;
    DABglobals.spectrumBuffer = nullptr;
    DABglobals.iqBuffer = &iqBuffer;
    DABglobals.responseBuffer = &responseBuffer;
    DABglobals.tiiBuffer = &tiiBuffer;

    // FIXME frameBuffer is used by aac frame dumping and is currentlky unused
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
    int32_t fmRate;

    if (inputDevice == nullptr)
	return;
    fmRate = mapRates(INPUT_RATE);
    if (FMfilter <= 0)
	FMfilter = 0.95*fmRate;
    FMprocessor = new fmProcessor(inputDevice, this, INPUT_RATE, fmRate,
				  workingRate, audioRate, FMthreshold);
    FMprocessor->setSink(soundOut);
    FMprocessor->setFMRDSSelector(rdsDecoder::RDS2);
    FMprocessor->setFMRDSDemod(fmProcessor::FM_RDS_AUTO);
    FMprocessor->setFMMode(true);
    FMprocessor->setBandwidth(FMfilter);
    FMprocessor->setBandFilterDegree(FMdegree);
    FMprocessor->setFMDecoder(fmDemodulator::fm_demod(FMdecoder));
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
    settings->setValue(GEN_DAB_MODE, dabDisplay == DD_SLIDES? GEN_DAB_SLIDES: GEN_DAB_STATIONS);
    settings->setValue(GEN_VOLUME, int(volumeKnob->value()));
    settings->setValue(GEN_SQUELCH, int(squelchKnob->value()));
    settings->setValue(GEN_DEVICE_TYPE, deviceType);
#ifdef HAVE_MPRIS
    settings->setValue(GEN_LAST_PRESET, int(lastPreset));
    settings->setValue(GEN_SKIP_PRESET_MODE, bool(skipPresetMode));
    if (currentPicFile != "")
	QFile::remove(currentPicFile);
#endif
    settings->beginWriteArray(GROUP_PRESETS);

    // skip 'presets'
    for (int i = 1; i < presetSelector->count(); i++) {
	settings->setArrayIndex(i-1);
	settings->setValue(PRESETS_NAME, presetSelector->itemText(i));
    }
    settings->endArray();
    settings->sync();
}

void RadioInterface::findDevices() {
    deviceDescriptor discoveredDevice;

#ifdef HAVE_SDRPLAY_V3
    bool foundV3 = false;
    try {
	    discoveredDevice.device = new sdrplayHandler_v3();
	    discoveredDevice.deviceType = "Sdrplay V3";
	    discoveredDevice.controls = HW_AGC|IF_GAIN|LNA_GAIN;
	    deviceList.push_back(discoveredDevice);
	    foundV3 = true;
    } catch (int e) {}
#endif
#ifdef HAVE_SDRPLAY
#ifdef HAVE_SDRPLAY_V3

    // rdsplay v2 is a fallback
    if (!foundV3)
#endif
	try {
		discoveredDevice.device = new sdrplayHandler();
		discoveredDevice.deviceType = "Sdrplay";
		discoveredDevice.controls = HW_AGC|IF_GAIN|LNA_GAIN;
		deviceList.push_back(discoveredDevice);
	} catch (int e) {}
#endif
#ifdef HAVE_RTLSDR
// no LNA
    try {
	    discoveredDevice.device = new rtlsdrHandler();
	    discoveredDevice.deviceType = "RtlSdr";
	    discoveredDevice.controls = SW_AGC|HW_AGC|COMBO_AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef HAVE_AIRSPY
// no LNA
    try {
	    discoveredDevice.device = new airspyHandler();
	    discoveredDevice.deviceType = "AirSpy";
	    discoveredDevice.controls = SW_AGC|HW_AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef HAVE_LIME
// no LNA
    try {
	    discoveredDevice.device = new limeHandler();
	    discoveredDevice.deviceType = "Lime";
	    discoveredDevice.controls = HW_AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef HAVE_PLUTO
// no LNA
    try {
	    discoveredDevice.device = new plutoHandler();
	    discoveredDevice.deviceType = "Pluto";
	    discoveredDevice.controls = HW_AGC|IF_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
#ifdef HAVE_HACKRF
// no AGC
    try {
	    discoveredDevice.device = new hackrfHandler();
	    discoveredDevice.deviceType = "HackRF";
	    discoveredDevice.controls = IF_GAIN|LNA_GAIN;
	    deviceList.push_back(discoveredDevice);
    } catch (int e) {}
#endif
    if (deviceList.size() == 0) {
	inputDevice = nullptr;
	deviceType = "";
    } else {
	QString saveType = deviceType;
	deviceStrings devNames[MAX_DEVICES];

	inputDevice = deviceList[0].device;
	deviceUiControls = deviceList[0].controls;
	deviceType = deviceList[0].deviceType;
	if (QString::compare(saveType, ""))
	    for (uint i = 0; i < deviceList.size(); ++i)
		if (!QString::compare(deviceList[i].deviceType, saveType)) {
		    inputDevice = deviceList[i].device;
		    deviceUiControls = deviceList[i].controls;
		    deviceType = deviceList[i].deviceType;
		    break;
		}
	log(LOG_UI, LOG_CHATTY, "using device %s", qPrintable(deviceType));
	settings->beginGroup(deviceType);
	uint dc = inputDevice->devices((deviceStrings *) &devNames, MAX_DEVICES);
	bool found = false;
	if (dc > 0) {
	    deviceId = settings->value(DEV_ID, "").toString();
	    for (uint i = 0; i < dc; i++)
		if (QString::compare(devNames[i].id, deviceId) == 0) {
		    found = true;
		    break;
		}
	    if (!found)
		deviceId = devNames[0].id;
	    inputDevice->setDevice(qPrintable(deviceId));
	    log(LOG_UI, LOG_CHATTY, "device %s using %s", qPrintable(deviceType), qPrintable(deviceId));
	} else
	    log(LOG_UI, LOG_CHATTY, "device name selection disabled for %s", qPrintable(deviceType));
	inputDevice->getIfRange(&minIfGain, &maxIfGain);
	inputDevice->getLnaRange(&minLnaGain, &maxLnaGain);
	agc = settings->value(DEV_AGC, DEV_DEF_AGC).toInt();
	ifGain = settings->value(DEV_IF_GAIN, (minIfGain + maxIfGain) / 2).toInt();
	lnaGain = settings->value(DEV_LNA_GAIN, minLnaGain).toInt();
	settings->endGroup();

	if (deviceUiControls & (HW_AGC | SW_AGC | COMBO_AGC)) {
	    inputDevice->setAgcControl((agc == AGC_ON || agc == AGC_COMBINED));
	}
	if (deviceUiControls & IF_GAIN) {
	    checkIfGain();
	    inputDevice->setIfGain(ifGain);
	}
	if (deviceUiControls & LNA_GAIN) {
	    checkLnaGain();
	    inputDevice->setLnaGain(lnaGain);
	}
    }

    // setup software agc
    resetSwAgc();
    resetAgcStats();
}

void RadioInterface::checkIfGain() {
   if (ifGain > maxIfGain)
	ifGain = maxIfGain;
   if (ifGain < minIfGain)
	ifGain = minIfGain;
}

void RadioInterface::checkLnaGain() {
   if (lnaGain > maxLnaGain)
	lnaGain = maxLnaGain;
   if (lnaGain < minLnaGain)
	lnaGain = minLnaGain;
}

// If a change is detected, we have to restart the selected
// service - if any. If the service is a secondary service,
// it might be the case that we have to start the main service
// how do we find that?
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
    for (const auto &serv: serviceList) {
	ensembleModel.appendRow(new QStandardItem(serv.name));
	for (int i = 0; i < ensembleModel.rowCount(); i ++) {
	    ensembleModel.setData(ensembleModel.index(i, 0),
			QFont("Cantarell", 11), Qt::FontRole);
	}
	ensembleDisplay->setModel(&ensembleModel);
	stationSelector->setModel(&ensembleModel);
    }

    // and restart the one that was running
    if (s.valid) {

	// secondary service may be gone
	if (s.SCIds != 0) {
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
	startDABService(&s);
    }
}

// In order to not overload with an enormous amount of
// signals, we trigger this function at most 10 times a second
void RadioInterface::newAudio(int amount, int rate) {
    _VLA(int16_t, vec, amount);

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
    stopFM();
    if (scanTimer == nullptr) {
	scanTimer = new QTimer();
	scanTimer->setInterval(scanInterval);
    }
    connect(scanTimer, SIGNAL(timeout()),
	this, SLOT(nextFrequency()));
    playing = false;
    scanning = true;
    cleanScreen();
    setPlaying();
    setRecording();
    setScanning();
#ifdef HAVE_MPRIS
    mprisLabelAndText("FM", "Scanning");
    player.setPlaybackStatus(Mpris::Stopped);
#endif
    FMfreq = (FMfreq*10+scanIncrement)/10;
    frequencyKnob->setValue(double(FMfreq));
    frequencyLCD->display(int(FMfreq*1000));
    inputDevice->restartReader(int(FMfreq*1000000));
    FMprocessor->start();
    FMprocessor->startScan();
    scanTimer->start();
}

// FIXME ideally stopping a scan and switching frequency should be serialized
void RadioInterface::stopFMscan() {
    if (scanTimer != nullptr && scanTimer->isActive())
	scanTimer->stop();
    disconnect(scanTimer, SIGNAL(timeout()),
	this, SLOT(nextFrequency()));
    scanning = false;
    FMprocessor->stopScan();
    stopFM();
    setPlaying();
    setRecording();
    setScanning();
}

//	slots

void RadioInterface::nextFrequency(void) {
    log(LOG_EVENT, LOG_MIN, "fm scan timer signal");
    if ((scanIncrement < 0 && FMfreq <= MIN_FM) || (scanIncrement > 0 && FMfreq >= MAX_FM)) {
	stopFMscan();
    } else {
	FMprocessor->stopScan();
	FMprocessor->stop();
	inputDevice->stopReader();
	FMfreq = (FMfreq*10+scanIncrement)/10;
	frequencyKnob->setValue(double(FMfreq));
	frequencyLCD->display(int(FMfreq*1000));
	inputDevice->restartReader(int(FMfreq*1000000));
	FMprocessor->start();
	FMprocessor->startScan();
	scanTimer->start();
    }
}

void RadioInterface::scanDone(void) {
    log(LOG_EVENT, LOG_MIN, "fm station found");
    stopFMscan();
    playing = true;
    startFM(int(FMfreq*1000000));
    setPlaying();
    setRecording();
    setScanning();
}

//	visual elements

//	slots exercised by the processors

QPixmap RadioInterface::getIcon(uint SId) {
    QPixmap p = QPixmap();
    QList<ImageInfo> info = cache->getImagesForChannel(SId);

    if (info.size() > 0) {
	int w = 0, h = 0;
	QString path = "";

	for (const ImageInfo& entry : info) {
	    log(LOG_CACHE, LOG_CHATTY, "found %s %i %i", qPrintable(entry.path), entry.width, entry.height);
	    if (entry.width >= w && entry.height >= h &&
		entry.width >= ICON_MIN_SIZE && entry.height >= ICON_MIN_SIZE &&
		entry.width <= ICON_MAX_SIZE && entry.height <= ICON_MAX_SIZE) {
		    w = entry.width;
		    h = entry.height;
		    path = entry.path;
	    }
	}
	if (path != "")
	    p = cache->loadImage(path);
    }
    if (p.isNull()) {
	p = QPixmap(ICON_LISTVIEW_SIZE);
	p.fill(Qt::transparent);
    }
    return p;
}

void RadioInterface::addToEnsemble(const QString &serviceName, uint SId) {
    serviceId ed;
    uint32_t lastId = 0;
    QString lastName = "";
    bool inserted = false;
    std::vector<serviceId>::iterator i;

    log(LOG_EVENT, LOG_MIN, "received service %s %i", qPrintable(serviceName), SId);
    if (!DABprocessor->is_audioService(serviceName)) {
	startDataService(serviceName, SId);
	return;
    }
    for (const auto &serv: serviceList)
	if (serv.name == serviceName)
	    return;

    ed.name = serviceName;
    ed.SId = SId;

    if (serviceList.size() == 0)
	ensembleModel.clear();
    else if (serviceOrder == ID_BASED) {
	for (i=serviceList.begin(); i<serviceList.end(); i++) {
	    if (lastId < SId && SId <= i->SId) {
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
    if (!inserted) {
	int row = ensembleModel.rowCount();
	serviceList.push_back(ed);
	ensembleModel.appendRow(new QStandardItem(ed.name));
	ensembleModel.setData(ensembleModel.index(row, 0),
			QFont("Cantarell", 11), Qt::FontRole);
	if (cache->imageCount() > 0)
	    ensembleModel.item(row)->setIcon(getIcon(ed.SId));
    } else {
	int row = 0;
	bool hasIcons = (cache->imageCount() > 0);

	ensembleModel.clear();
	for (const auto &serv: serviceList) {
	    ensembleModel.appendRow(new QStandardItem(serv.name));
	    ensembleModel.setData(ensembleModel.index(row, 0),
			    QFont("Cantarell", 11), Qt::FontRole);
	    if (hasIcons)
		ensembleModel.item(row)->setIcon(getIcon(serv.SId));
	    row++;
	}
    }
    for (int i = 0; i < ensembleModel.rowCount(); i++)
	if (currentService.valid && serviceList.at(i).name == currentService.serviceName) {
	    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
	    stationSelector->setCurrentIndex(i);
	}

    ensembleDisplay->setModel(&ensembleModel);
    stationSelector->setModel(&ensembleModel);
    if (nextService.valid && nextService.serviceName == serviceName) {
#ifdef HAVE_MPRIS
	mprisLabelAndText("DAB", serviceName.trimmed());
#endif
	startDABService(&nextService);
	nextService.valid = false;
    }
#ifdef HAVE_MPRIS
    else if (currentService.valid && currentService.serviceName == serviceName)
	mprisLabelAndText("DAB", serviceName.trimmed());
#endif
}

void RadioInterface::nameOfEnsemble(int id, const QString &v) {
    log(LOG_EVENT, LOG_CHATTY, "station name %s %i", qPrintable(v), id);
    ensembleId->setText(v + " (" + QString::number(id, 16) + ")");
}

void RadioInterface::ensembleLoaded(int count) {
    int i = 0;

    // we are loading a scan list, no need to start a service
    if (scanning) {
	if (count >= 0)
	    log(LOG_EVENT, LOG_MIN, "dab full scan ensemble loaded with %i services", count);
	scanEnsembleLoaded();
	return;
    }
    log(LOG_EVENT, LOG_MIN, "ensemble complete with %i services", count);
    if (nextService.valid && nextService.serviceName == "" && count > 0) {
	if (nextService.fromEnd)
	    i = serviceList.size() - 1;
	
	currentService.serviceName = serviceList.at(i).name;
	currentService.valid = true;
	setPlaying();
	setRecording();
	setScanning();
	if (nextService.autoPlay) {
#ifdef HAVE_MPRIS
	    mprisLabelAndText("DAB", currentService.serviceName.trimmed());
#endif
	    startDABService(&currentService);
	} else {
	    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
	    stationSelector->setCurrentIndex(i);
	}
    }
    nextService.serviceName = "";
    nextService.valid = false;
    nextService.fromEnd = false;
    nextService.autoPlay = false;
}

void RadioInterface::showStrength(float strength) {
    log(LOG_EVENT, LOG_VERBOSE, "signal strength %f", strength);
    if (strength < MEDIAN_QUALITY)
	signalStrength->setFillBrush(QBrush(Qt::red));
    else
	signalStrength->setFillBrush(QBrush(Qt::green));
    signalStrength->setValue(strength);
}

void RadioInterface::showQuality(bool b) {
    log(LOG_EVENT, LOG_VERBOSE, "quality %i", b);
    if (b) 
	ficSuccess += FIC_SUCCESS_INCREMENT;
    if (++ficBlocks >= FIC_BLOCKS_MAX) {
	if (ficSuccess > MEDIAN_QUALITY) {
	    signalQuality->setFillBrush(QBrush(Qt::green));
	} else {
	    signalQuality->setFillBrush(QBrush(Qt::red));
	}
	signalQuality->setValue(ficSuccess);
	ficBlocks = 0;
	ficSuccess = 0;
    }
}

void RadioInterface::showText(QString s) {
    log(LOG_EVENT, LOG_CHATTY, "radio text %s", qPrintable(s));
    dynamicLabel->setText(s);
#ifdef HAVE_MPRIS

    // some MPRIS mistakenly report "no track playing" if the title is empty
    // note that blank but not empty does the trick
    if (s != "") {
	metadata["xesam:title"] = s;
	player.setMetadata(metadata);
    }
#endif
}

void RadioInterface::showSoundMode(bool s) {
    log(LOG_EVENT, LOG_VERBOSE, "stereo mode %i", s);
    stereoLabel->setStyleSheet(s?  "QLabel {background-color: green; color: white}":
			 "QLabel {background-color: red; color: white}");
    stereoLabel->setText(s? "stereo" : "mono");
}

void RadioInterface::showLabel(const QString s) {
    log(LOG_EVENT, LOG_CHATTY, "radio label %s", qPrintable(s));
    serviceLabel->setAlignment(Qt::AlignLeft);
    serviceLabel->setText(s);
#ifdef HAVE_MPRIS
    metadata["xesam:artist"] = s;
    player.setMetadata(metadata);
#endif
}

#define BLACK_THRESHOLD 10

bool isBlack(QRgb pixel) {
    if (qAlpha(pixel) == 0)
	return true;
    return qRed(pixel) <= BLACK_THRESHOLD &&
	qGreen(pixel) <= BLACK_THRESHOLD &&
	qBlue(pixel) <= BLACK_THRESHOLD;
};

bool rowHasContent(QImage img, QRgb pixel, int width, int y) {
    const QRgb* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
    return (line[0] == pixel) &&
	(line[width/2] == pixel) &&
	(line[width-1] == pixel);
};

bool colHasContent(QImage img, QRgb pixel, int x, int top, int bottom) {
    const QRgb* lineTop = reinterpret_cast<const QRgb*>(img.constScanLine(top));
    const QRgb* lineMid = reinterpret_cast<const QRgb*>(img.constScanLine((top+bottom)/2));
    const QRgb* lineBot = reinterpret_cast<const QRgb*>(img.constScanLine(bottom));
    return (lineTop[x] == pixel) &&
	(lineMid[x] == pixel) &&
	(lineBot[x] == pixel);
};

QPixmap trimBorders(const QImage& image) {
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    int width = img.width();
    int height = img.height();
    int top = 0;
    int bottom = height - 1;
    int left = 0;
    int right = width - 1;
    const QRgb* topLine = reinterpret_cast<const QRgb*>(img.constScanLine(top));
    const QRgb* bottomLine = reinterpret_cast<const QRgb*>(img.constScanLine(bottom));
    QRgb topLeft = topLine[0];
    QRgb bottomRight = bottomLine[right];

    if (isBlack(topLeft)) {
	for (; top < height && !rowHasContent(img, topLeft, width, top); ++top);
	for (; left < width && !colHasContent(img, topLeft, left, top, bottom); ++left);
    }

    if (isBlack(bottomRight)) {
	for (; bottom > top && !rowHasContent(img, bottomRight, width, bottom); --bottom);
	for (; right > left && !colHasContent(img, bottomRight, right, top, bottom); --right);
    }

    int doTop = (top > 0 || bottom < height - 1);
    int doSides = (left > 0 || right < width - 1);
    if (left > right || top > bottom)
	return QPixmap();
    else if (!doTop && !doSides)
	return QPixmap::fromImage(img);

    // if only one pair of sides, we can just trim the image,
    // as the content scaling won't change
    if (doTop != doSides)
	return QPixmap::fromImage(img.copy(left, top, right - left + 1, bottom - top + 1));

    // otherwise crop top and bottom and fill left and right)
    img = img.copy(0, top, width - 1, bottom - top + 1);
    QPainter painter(&img);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if (left > 0)
	painter.fillRect(0, 0, left, img.height(), Qt::transparent);
    if (right < width - 1)
	painter.fillRect(right - left + 1, 0, width - right -1, img.height(), Qt::transparent);

    return QPixmap::fromImage(img);
}

void RadioInterface::handleSlides(QByteArray data, int contentType, QString pictureName, int dirs) {
    const char *type;

    log(LOG_EVENT, LOG_MIN, "slide %s 0x%x", qPrintable(pictureName), contentType);
    if (pictureName == QString(""))
	return;
    switch (static_cast<MOTContentType>(contentType)) {
    case MOTCTImageGIF:
	type = "gif";
	break;
    case MOTCTImageJFIF:
	type = "jpg";
	break;
    case MOTCTImageBMP:
	type = "bmp";
	break;
    case MOTCTImagePNG:
	type = "png";
	break;
    default:
	return;
    }

    if (dirs != 0)
	handleEPGPicture(data, type, pictureName);
    else {
	QImage i;

	i.loadFromData(data, type);
	currentService.slidePriority = INT_MAX;
	showSlides(trimBorders(i));
    }
}

int32_t extractServiceIdFromFilename(const QString &filename) {
    QString serviceIdHex;
    QString name = filename.split('/').last().split('\\').last();
    int dotPos = name.lastIndexOf('.');

    if (dotPos > 0)
	name = name.left(dotPos);

    // Pattern 1: siXXXX## format (e.g., siC22C03, siD31701)
    QRegularExpression pattern1("^si([0-9A-Fa-f]+)\\d{2}$");
    QRegularExpressionMatch match1 = pattern1.match(name);
    if (match1.hasMatch())
	serviceIdHex = match1.captured(1);
    else {

	// Pattern 2: XXXX_#_### format (e.g., C22C_1_000, D317_0_000)
	QRegularExpression pattern2("^([0-9A-Fa-f]+)_\\d+_\\d+$");
	QRegularExpressionMatch match2 = pattern2.match(name);
	if (match2.hasMatch())
	    serviceIdHex = match2.captured(1);
	else {

	    // Pattern 3: YYYYMMDD_XXXX_SI format (e.g., 20241213_C22C_SI)
	    QRegularExpression pattern3("^\\d{8}_([0-9A-Fa-f]+)_SI$");
	    QRegularExpressionMatch match3 = pattern3.match(name);
	    if (match3.hasMatch())
		serviceIdHex = match3.captured(1);
	}
    }

    if (!serviceIdHex.isEmpty()) {
	bool ok;
	int serviceId = serviceIdHex.toInt(&ok, 16);  // Base 16 for hex
	if (ok)
	    return serviceId;
    }
    return -1;
}

void RadioInterface::handleEPGPicture(QByteArray data, const char *type, QString pictureName) {
    QPixmap p;
    QString serviceName;
    int32_t SId;
    int priority;

    SId = extractServiceIdFromFilename(pictureName);
    if (SId < 0)
	return;
    p.loadFromData(data, type);
    priority = p.width() * p.height();
    log(LOG_EVENT, LOG_MIN, "slide SId %x %s %i %i", SId, type, p.width(), p.height());
    cache->addImage(SId, p.width(), p.height(), type, data);
    if (p.width() == p.height() && p.height() >= ICON_MIN_SIZE && p.height() <= ICON_MAX_SIZE) {
	QPixmap empty(ICON_LISTVIEW_SIZE);

	// there's no other way to align text when some icons are missing
	// than adding an empty icon
	// it will be overwritten by the logo when it arrives
	empty.fill(Qt::transparent);
	ensembleDisplay->setIconSize(QSize(ICON_LISTVIEW_SIZE));
	for (int i = 0; i < ensembleModel.rowCount(); i++) {
	    if (serviceList.at(i).SId == (uint32_t) SId) {
		ensembleModel.item(i)->setIcon(p);
	    } else if (ensembleModel.item(i)->icon().isNull())
		ensembleModel.item(i)->setIcon(QIcon(empty));
	}
    } else if (playing && currentService.SId == (uint32_t) SId &&
	       priority > currentService.slidePriority &&
	       p.width() >= SLIDE_MIN_SIZE && p.height() >= SLIDE_MIN_SIZE) {
	currentService.slidePriority = priority;
	showSlides(p);
    }
}

// Slides should be desplayed with the following priorities
// MOT slideshows
// MOT logos (highest reolution preferred)
// default slides
// empty slides
void RadioInterface::showSlides(QPixmap p) {
#ifdef HAVE_MPRIS
    QString tmpPicFile = currentPicFile;
#endif
    int h = slidesLabel->height();
    int w = slidesLabel->width();

    slidesLabel->setPixmap(p.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (dabDisplay == DD_SLIDES)
	slidesLabel->show();
#ifdef HAVE_MPRIS
    if (tmpPicFile != "")
	QFile::remove(tmpPicFile);
    currentPicFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QString("/" TARGET "%1.png").arg(rand());
    p.save(currentPicFile);
    log(LOG_MPRIS, LOG_MIN, "slide file %s", qPrintable(currentPicFile));
    metadata["mpris:artUrl"] = currentPicFile;
    player.setMetadata(metadata);
#endif
}

void RadioInterface::handleMotObject(QByteArray result,
					  QString name,
					  int contentType, bool dirElement) {

    log(LOG_EVENT, LOG_MIN, "MOT name %s cont 0x%x dir %i", qPrintable(name),
	getContentBaseType((MOTContentType)contentType), dirElement);

    // currently we only handle images
    switch (getContentBaseType((MOTContentType)contentType)) {
    case MOTBaseTypeImage:
	handleSlides(result, contentType, name, dirElement);
	break;
    default:
	break;
    }
}

void RadioInterface::closeEvent(QCloseEvent *event) {
    log(LOG_UI, LOG_MIN, "close window received");
    if (yesNo(this)) {
	log(LOG_UI, LOG_MIN, "close window accepted");
	terminateProcess();
	event->accept();
    } else {
	log(LOG_UI, LOG_MIN, "close window denied");
	event->ignore();
    }
}

// program icon changes depending on whether window is minimised
// and radio is playing
void RadioInterface::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
	if (!isMinimized())
	    setWindowIcon(MAIN_ICON_PATH);
	setIconAndTitle();
    }
}


//	audio

void RadioInterface::handleVolume(double vol) {
    log(LOG_UI, LOG_MIN, "volume changed to %f", vol);
    soundOut->setVolume(qreal(vol)/AUDIO_SCALE);
#ifdef HAVE_MPRIS
    player.setVolume(vol / AUDIO_SCALE);
#endif
}

void RadioInterface::handleSquelch(double val) {
    log(LOG_UI, LOG_MIN, "squelch changed to %f", val);
    FMprocessor->setSquelchValue(AUDIO_SCALE-int(val));
}

//	preset selection

static
void addPreset(QComboBox *presetSelector, QString preset, QString comment) {
    log(LOG_UI, LOG_MIN, "add preset %s", qPrintable(preset));

    // don't add duplicates
    for (int i = 0; i < presetSelector->count(); i ++)
	if (presetSelector->itemText(i).contains(preset)) {
	    log(LOG_UI, LOG_MIN, "preset already there %s", qPrintable(preset));
	    return;
	}
    if (comment != "")
	preset = preset + ":" + comment;
    presetSelector->addItem(preset);
}

static
void deletePreset(QComboBox *presetSelector, QString preset) {
    log(LOG_UI, LOG_MIN, "delete preset %s", qPrintable(preset));

    for (int i = 0; i < presetSelector->count(); i ++)
	if (presetSelector->itemText(i).contains(preset)) {
	    presetSelector->removeItem(i);
	    return;
	}
    log(LOG_UI, LOG_MIN, "preset not found %s", qPrintable(preset));
}

void RadioInterface::handlePresetSelector(int index) {
    int newPreset;
    QString preset;

    log(LOG_UI, LOG_MIN, "play preset %i", index);
    if (inputDevice == NULL || index >= presetSelector->count() || presetSelector->count() <= 1)
	return;
    preset =  presetSelector->itemText(index);

    log(LOG_UI, LOG_MIN, "preset is %s", qPrintable(preset));
    if (preset == "Presets")
	return;

#if QT_VERSION >= 0x050e00
    QStringList list = preset.split(":", Qt::SkipEmptyParts);
#else
    QStringList list = preset.split(":", QString::SkipEmptyParts);
#endif
    if (list.length() != 2 && list.length() != 3) {
	warning(this, tr(BAD_PRESET));
	deletePreset(presetSelector, preset);
	return;
    }
    playing = false;
    stopRecording();
#ifdef HAVE_MPRIS
    newPreset = presetSelector->findText(preset, Qt::MatchExactly);
    if (newPreset > 0)
	lastPreset = newPreset;
#else
    (void) newPreset;
#endif

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
    disconnect(channelSelector, SIGNAL(activated(int)),
		    this, SLOT(handleSelectChannel(int)));
    int k = channelSelector->findText(channel);

    if (k != -1)
	channelSelector->setCurrentIndex(k);
    else {
	warning(this, tr(BAD_PRESET));
	deletePreset(presetSelector, preset);
    }
    connect(channelSelector, SIGNAL(activated(int)),
		 this, SLOT(handleSelectChannel(int)));
    if (k == -1)
	return;

    toDAB();
    stopDAB();
    nextService.valid = true;
    nextService.serviceName = service;
    nextService.SId = 0;
    nextService.SCIds = 0;

    // the preset service will be picked up by the addToEnsemble() slot
    startDAB();
}

void RadioInterface::handleAddDABPreset() {
    if (currentService.serviceName.at(1) == ' ') {
	log(LOG_UI, LOG_MIN, "add dab empty preset");
	return;
    }

    addPreset(presetSelector, channelSelector->currentText() + ":" + currentService.serviceName, "");
}

void RadioInterface::handleDeleteDABPreset() {
    if (currentService.serviceName.at(1) == ' ') {
	log(LOG_UI, LOG_MIN, "delete dab empty preset");
	return;
    }

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
    presetSelector->setCurrentIndex(0);
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
    stereoLabel->setToolTip("");
    serviceLabel->setToolTip("");
    currentService.valid = false;
    cleanScreen();
#ifdef HAVE_MPRIS
    mprisLabelAndText("DAB", currentService.serviceName.trimmed());
    player.setPlaybackStatus(Mpris::Stopped);
#endif
}

void RadioInterface::handleSelectService(QModelIndex ind) {
    dabService s;

    QString currentProgram = ind.data(Qt::DisplayRole).toString();
    if (currentProgram == "") {
	log(LOG_UI, LOG_MIN, "switching to an empty service");
	return;
    }
    log(LOG_UI, LOG_MIN, "switching to dab %s", qPrintable(currentProgram));

    stopDABService();
    DABprocessor->getParameters(currentProgram, &s. SId, &s.SCIds);
    if (s.SId == 0)
	   warning(this, tr(BAD_SERVICE));
    else {
	s.serviceName = currentProgram;
	startDABService(&s);
    }
}

void RadioInterface::handleSelectService(int ind) {
    dabService s;

    QString currentProgram = stationSelector->itemText(ind);
    if (currentProgram == "") {
	log(LOG_UI, LOG_MIN, "switching to an empty service");
	return;
    }
    log(LOG_UI, LOG_MIN, "switching to dab %s", qPrintable(currentProgram));

    stopDABService();
    DABprocessor->getParameters(currentProgram, &s. SId, &s.SCIds);
    if (s.SId == 0)
	   warning(this, tr(BAD_SERVICE));	
    else {
	s.serviceName = currentProgram;
	startDABService(&s);
    }
}

void RadioInterface::handleStationsAction() {
    log(LOG_UI, LOG_MIN, "switching to dab stations");
    dabDisplayOff();
    dabDisplay = DD_STATIONS;
    dabDisplayOn();
}

void RadioInterface::handleSlidesAction() {
    log(LOG_UI, LOG_MIN, "switching to dab slides");
    dabDisplayOff();
    dabDisplay = DD_SLIDES;
    dabDisplayOn();
}

void RadioInterface::handleLeftDisplayButton() {
    dabDisplayOff();
    dabDisplay = dabDisplay > 0? dabDisplay - 1: DD_COUNT - 1;
    log(LOG_UI, LOG_MIN, "switching to prior dab view %i", dabDisplay);
    dabDisplayOn();
}

void RadioInterface::handleRightDisplayButton() {
    dabDisplayOff();
    dabDisplay = (dabDisplay + 1) % DD_COUNT;
    log(LOG_UI, LOG_MIN, "switching to next dab view %i", dabDisplay);
    dabDisplayOn();
}

void RadioInterface::dabDisplayOff() {
    slidesAction->setVisible(false);
    stationsAction->setVisible(false);
    slidesLabel->setVisible(false);
    ensembleDisplay->setVisible(false);
    stationSelector->setVisible(false);
    channelSelector->setVisible(false);
}

void RadioInterface::dabDisplayOn() {
    switch (dabDisplay) {
      case DD_SLIDES:
	slidesLabel->setVisible(true);
	stationSelector->setVisible(true);
	stationsAction->setVisible(true);
	break;
      case DD_STATIONS:
	slidesAction->setVisible(true);
	channelSelector->setVisible(true);
	ensembleDisplay->setVisible(true);
	break;
    }
}

//	DAB OPs

void RadioInterface::startDABService(dabService *s) {
    QString serviceName = s->serviceName;
    QPixmap p;

    if (playing && !isFM && currentService.valid) {
	log(LOG_UI, LOG_MIN, "service %s is still valid", currentService.serviceName.toLatin1().data());
	stopDABService();
    }

    ficBlocks = 0;
    ficSuccess = 0;
    currentService = *s;
    currentService.valid = false;
    for (int i = 0; i < (int) ensembleModel.rowCount(); i ++) {
	QString itemText = ensembleModel.index(i, 0).data(Qt::DisplayRole).toString();
	if (itemText == serviceName) {
	    audiodata ad;
	    char buf[INFOBUFLEN];

	    ensembleDisplay->setCurrentIndex(ensembleModel.index(i, 0));
	    stationSelector->setCurrentIndex(i);
	    serviceLabel->setStyleSheet("QLabel {color: black}");
	    showLabel(serviceName);
	    if (DABprocessor->is_audioService(serviceName)) {
		DABprocessor->dataforAudioService(serviceName, &ad);
		currentService.valid = true;
		currentService.serviceName = serviceName;
		currentService.SId = serviceList.at(i).SId;
		if (!ad.defined)
		    warning(this, tr(BAD_SERVICE));
		else {
		    ad.procMode = __ONLY_SOUND;
	 	    DABprocessor->set_audioChannel(&ad, &audioBuffer);
		    soundOut->restart();
		    ad.audioInfo((char *) &buf, INFOBUFLEN);
		    stereoLabel->setToolTip((char *) &buf);
		    ad.serviceInfo((char *) &buf, INFOBUFLEN);
		    serviceLabel->setToolTip((char *) &buf);
		    currentService.slidePriority = 0;
		    QList<ImageInfo> info = cache->getImagesForChannel(currentService.SId);
		    if (info.size() > 0) {
			int w = 0, h = 0;
			QString path = "";

			for (const ImageInfo& entry : info) {
			    log(LOG_CACHE, LOG_CHATTY, "found %s %i %i", qPrintable(entry.path), entry.width, entry.height);
			    if (entry.width >= w && entry.height >= h &&
				entry.width >= SLIDE_MIN_SIZE && entry.height >= SLIDE_MIN_SIZE) {
				    w = entry.width;
				    h = entry.height;
				    path = entry.path;
			    }
			}
			if (path != "") {
			    p = cache->loadImage(path);
			    currentService.slidePriority = p.width() * p.height();
			}
		    }
		    if (currentService.slidePriority == 0) {
			if (ad.isDABplus())
			    p.load(":/empty_plus.png");
			else
			    p.load(":/empty.png");
		    }
		    showSlides(p);
		}
		playing = true;
		setPlaying();
		setRecording();
#ifdef HAVE_MPRIS
		mprisLabelAndText(serviceName.trimmed(), " ");
		player.setPlaybackStatus(Mpris::Playing);
#endif
	    } else
		warning(this, tr(BAD_SERVICE));
	    return;
	}
    }
}

void RadioInterface::startDAB() {
    QString channel = channelSelector->currentText();
    int tunedFrequency = DABband.frequency(channel.toStdString());

    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    cache = new ImageCache(QString("%1/%2").arg(LOCAL_CACHE).arg(channel));
    emptyArt(true);
#ifdef HAVE_MPRIS
    mprisLabelAndText("DAB", channel);
    player.setPlaybackStatus(Mpris::Stopped);
#endif
    inputDevice->restartReader(tunedFrequency);
    DABprocessor->start(tunedFrequency);
}

void RadioInterface::startDataService(QString serviceName, uint SId) {
#ifdef USE_SPI
    packetdata pd;

    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    if (scanning || dataService.valid)
	return;
    DABprocessor->dataforPacketService(serviceName, &pd, 0);
    if (!pd.defined) {
	log(LOG_SPI, LOG_MIN, "cound not find background service %s %d", qPrintable(serviceName), SId);
	return;
    }
    log(LOG_SPI, LOG_MIN, "starting background service %s %d", qPrintable(serviceName), SId);
    dataService.serviceName = serviceName;
    dataService.SId = SId;
    dataService.valid = true;
    DABprocessor->set_dataChannel(&pd, &dataBuffer);
#else
    (void) serviceName;
    (void) SId;
    (void) SCIDs;
#endif
}

void RadioInterface::stopDataServices() {
#ifdef USE_SPI
    packetdata pd;

    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    if (scanning || dataService.serviceName == "" || !dataService.valid)
	return;
    log(LOG_SPI, LOG_MIN, "stopping background service %s", qPrintable(dataService.serviceName));
    DABprocessor->dataforPacketService(dataService.serviceName, &pd, 0);
    DABprocessor->stopService(&pd);
    dataService.valid = false;
#endif
}

void RadioInterface::stopDAB() {
    if (inputDevice == nullptr || DABprocessor == nullptr)
	return;
    if (currentService.valid) {
	audiodata ad;

	DABprocessor->dataforAudioService(currentService.serviceName, &ad);
	DABprocessor->stopService(&ad);
	usleep(1000);
	soundOut->stop();
    }
    stopDataServices();
    playing = false;
    stopRecording();
    setPlaying();
    setRecording();
    ficSuccess = 0;
    ficBlocks = 0;
    presetSelector->setCurrentIndex(0);

    currentService.valid = false;
    DABprocessor->stop();
    inputDevice->stopReader();
    usleep(1000);
    nextService.valid = false;
    nextService.autoPlay = false;
    serviceList.clear();
    ensembleModel.clear();
    ensembleDisplay->setModel(&ensembleModel);
    stationSelector->setModel(&ensembleModel);
    ensembleId->clear();
    cleanScreen();
#ifdef HAVE_MPRIS
    mprisLabelAndText("DAB", " ");
    player.setPlaybackStatus(Mpris::Stopped);
#endif

    if (cache != nullptr) {
	delete cache;
	cache = nullptr;
    }
}

void RadioInterface::handleSelectChannel(int index) {
    log(LOG_UI, LOG_MIN, "switching to dab channel %i", index);
    if (index < 0 || index >=  channelSelector->count())
	return;

    channelSelector->setCurrentIndex(index);
    log(LOG_UI, LOG_MIN, "dab channel is %s", qPrintable(channelSelector->currentText()));
    stopDAB();
    currentService.valid = false;
    currentService.serviceName = "";
    nextService.valid = true;
    nextService.fromEnd = false;
    nextService.autoPlay = false;
    setPlaying();
    setRecording();
    setScanning();
    startDAB();
}

void RadioInterface::handleNextChanButton() {
    int i;
    int currentChannel = channelSelector->currentIndex();

    // if there's a current service and it's not the last, jump service
    if (currentService.serviceName != "")
	for (i = 0; i < (int)(serviceList.size()-1); i++)
	    if (serviceList.at(i).name == currentService.serviceName) {
		dabService s;

		log(LOG_UI, LOG_MIN, "switching to next station");
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
		    stationSelector->setCurrentIndex(i);
		}
		return;
	    }

    // no dice, switch channel
    log(LOG_UI, LOG_MIN, "switching to next channel");
    nextService.autoPlay = playing;
    stopDAB();
    currentChannel++;
    if (currentChannel >= channelSelector->count())
	currentChannel = 0;
    disconnect(channelSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectChannel(int)));
    channelSelector->setCurrentIndex(currentChannel);
    connect(channelSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectChannel(int)));
    currentService.valid = false;
    currentService.serviceName = "";
    nextService.valid = true;
    nextService.fromEnd = false;
    setPlaying();
    setRecording();
    setScanning();
    startDAB();
}

void RadioInterface::handlePrevChanButton() {
    int i;
    int currentChannel = channelSelector->currentIndex();

    // if there's a current service and it's not the last, jump service
    if (currentService.serviceName != "")
	for (i = 1; i < (int)(serviceList.size()); i++)
	    if (serviceList.at(i).name == currentService.serviceName) {
		dabService s;

		log(LOG_UI, LOG_MIN, "switching to previous station");
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
		    stationSelector->setCurrentIndex(i);
		}
		return;
	    }

    // no dice, switch channel
    log(LOG_UI, LOG_MIN, "switching to previous channel");
    nextService.autoPlay = playing;
    stopDAB();
    currentChannel--;
    if (currentChannel < 0)
	currentChannel = channelSelector->count()-1;
    disconnect(channelSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectChannel(int)));
    channelSelector->setCurrentIndex(currentChannel);
    connect(channelSelector, SIGNAL(activated(int)),
		this, SLOT(handleSelectChannel(int)));
    currentService.valid = false;
    currentService.serviceName = "";
    nextService.valid = true;
    nextService.fromEnd = true;
    setPlaying();
    setRecording();
    setScanning();
    startDAB();
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
    emptyArt(false);
#ifdef HAVE_MPRIS
    mprisLabelAndText("FM", QString().asprintf("%3.3f", FMfreq));
    player.setPlaybackStatus(Mpris::Playing);
#endif
}

void RadioInterface::stopFM() {
    if (inputDevice == nullptr || FMprocessor == nullptr)
	return;
    if (scanning)
	stopFMscan();
    ficBlocks = 0;
    ficSuccess = 0;
    FMprocessor->stop();
    soundOut->stop();
    inputDevice->stopReader();
    playing = false;
    scanning = false;
    stopRecording();
    setPlaying();
    setRecording();
    setScanning();
    cleanScreen();
    emptyArt(true);
#ifdef HAVE_MPRIS
    mprisLabelAndText("FM", QString().asprintf("%3.3f", FMfreq));
    player.setPlaybackStatus(Mpris::Stopped);
#endif
}

void RadioInterface::handleScanDown() {
    log(LOG_UI, LOG_MIN, "starting previous station scan");
    startFMscan(true);
}

void RadioInterface::handleScanUp() {
    log(LOG_UI, LOG_MIN, "starting next station scan");
    startFMscan(false);
}

void RadioInterface::handleStopScan() {
    log(LOG_UI, LOG_MIN, "stopping station scan");
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
    log(LOG_UI, LOG_MIN, "new fm frequency %f", freq);
    FMfreq = freq;
    frequencyLCD->display(int(FMfreq*1000));
    if (playing) {
	stopFM();
	startFM(int(FMfreq*1000000));
    }
}

//	play, pause, record, stop record

void RadioInterface::handlePlayButton() {
    log(LOG_UI, LOG_MIN, "play");
    disconnect(playButton, SIGNAL(clicked(void)),
		 this, SLOT(handlePlayButton(void)));
    if (isFM)
	startFM(int(FMfreq*1000000));
    else
	startDABService(&currentService);
}

void RadioInterface::handlePauseButton() {
    log(LOG_UI, LOG_MIN, "pause");
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
	    connect(playButton, SIGNAL(clicked(void)),
			this, SLOT(handlePauseButton(void)));
	    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	    playButton->setToolTip("Pause playback");
	} else {
	    connect(playButton, SIGNAL(clicked(void)),
			this, SLOT(handlePlayButton(void)));
	    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	    playButton->setToolTip("Start playback");
	}
    } else {
	playButton->setToolTip("");
	playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	playButton->setEnabled(false);
    }
    setIconAndTitle();
}

#define PAUSED "paused - "

void RadioInterface::setIconAndTitle() {
    QString title;

    if (isMinimized()) {
	if (playing)
	    setWindowIcon(PLAY_ICON_PATH);
	else
	    setWindowIcon(PAUSE_ICON_PATH);
	if (isFM)
	    title = QString().asprintf("%3.3f", FMfreq);
	else if (currentService.serviceName != "")
	    title = currentService.serviceName;
	else
	    title = originalTitle;
    } else {
	title = originalTitle;
	if (!playing)
	    title.prepend(PAUSED);
    }
    setWindowTitle(title);
}

void RadioInterface::handleRecordButton() {
    log(LOG_UI, LOG_MIN, "record");
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
    log(LOG_UI, LOG_MIN, "stop recording");
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
	    recordButton->setToolTip("Stop recording");
    	} else {
	    connect(recordButton, SIGNAL(clicked (void)),
			this, SLOT(handleRecordButton(void)));
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
    log(LOG_UI, LOG_MIN, "to dab");
    if (!isFM)
	return;
    stopFM();
    stopRecording();
    playing = false;
    scanning = false;
    toDAB();
    currentService.valid = (currentService.serviceName != "");
    startDAB();
    setPlaying();
    setRecording();
    setScanning();
}

void RadioInterface::toDAB() {
    isFM = false;
    disconnect(addPresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleAddFMPreset(void)));
    disconnect(deletePresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleDeleteFMPreset(void)));
    fmWidget->hide();
    dabWidget->show();
    squelchKnob->setEnabled(false);
    connect(addPresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleAddDABPreset(void)));
    connect(deletePresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleDeleteDABPreset(void)));
    FMButton->setChecked(false);
    DABButton->setChecked(true);
    dabDisplayOn();
}

void RadioInterface::handleFMButton() {
    log(LOG_UI, LOG_MIN, "to fm");
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
		this, SLOT(handleAddDABPreset(void)));
    disconnect(deletePresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleDeleteDABPreset(void)));
    dabWidget->hide();
    fmWidget->show();
    squelchKnob->setEnabled(true);
    connect(addPresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleAddFMPreset(void)));
    connect(deletePresetButton, SIGNAL(clicked(void)),
		this, SLOT(handleDeleteFMPreset(void)));
    DABButton->setChecked(false);
    FMButton->setChecked(true);
    slidesAction->setVisible(false);
    stationsAction->setVisible(false);
    emptyArt(true);
#ifdef HAVE_MPRIS
    mprisLabelAndText("FM", QString().asprintf("%3.3f", FMfreq));
    player.setPlaybackStatus(Mpris::Stopped);
#endif
}

// utility

void RadioInterface::emptyArt(bool dimmed) {
    QPixmap p;

    if (dimmed)
	p.load(":/" TARGET "_dimmed.png");
    else
	p.load(":/" TARGET ".ico");
    currentService.slidePriority = 0;
    showSlides(p);
}

void RadioInterface::cleanScreen() {
    QPixmap p;

    if (isFM)
	ensembleId->clear();
    serviceLabel->clear();
    dynamicLabel->clear();
    emptyArt(true);
    presetSelector->setCurrentIndex(0);
    stereoLabel->setStyleSheet(stereoStyle);
    stereoLabel->clear();
    stereoLabel->setToolTip("");
    signalQuality->setValue(0);
    signalStrength->setValue(0);
    log(LOG_EVENT, LOG_CHATTY, "clean screen");
}

void RadioInterface::changePreset(int d) {
#ifdef HAVE_MPRIS
    if (presetSelector->count() <= 1)
	return;
    lastPreset += d;
    if (lastPreset >= presetSelector->count())
	lastPreset = 1;
    else if (lastPreset <= 1)
	lastPreset = presetSelector->count()-1;
    handlePresetSelector(lastPreset);
#else
    (void) d;
#endif
}

void RadioInterface::changeStation(int d) {
    if (isFM) {
	if (d < 0 && FMfreq <= MIN_FM) {
		handleDABButton();
		channelSelector->setCurrentIndex(channelSelector->count()-1);
		nextService.serviceName = "";
		nextService.valid = true;
		nextService.fromEnd = true;
		nextService.autoPlay = false;
		startDAB();
		return;
	} else if (d > 0 && FMfreq >= MAX_FM) {
		handleDABButton();
		channelSelector->setCurrentIndex(0);
		nextService.serviceName = "";
		nextService.valid = true;
		nextService.fromEnd = false;
		nextService.autoPlay = false;
		startDAB();
		return;
	}
    } else {
	if (d < 0 && channelSelector->currentIndex() == 0) {
		handleFMButton();
		FMfreq = MAX_FM;
	}
	else if (d >0 && channelSelector->currentIndex() == channelSelector->count()-1) {
		handleFMButton();
		FMfreq = MIN_FM;
	}
    }
    if (isFM)
	startFMscan((d < 0));
    else if (d < 0)
	handlePrevChanButton();
    else
	handleNextChanButton();
}

// MPRIS

#ifdef HAVE_MPRIS
void RadioInterface::mprisLabelAndText(QString l, QString t) {
    log(LOG_MPRIS, LOG_MIN, "mpris artist %s title %s", qPrintable(l), qPrintable(t));
    metadata["xesam:artist"] = l;
    metadata["xesam:title"] = t;
    player.setMetadata(metadata);
}

void RadioInterface::mprisClose() {
	log(LOG_MPRIS, LOG_MIN, "mpris close");
	terminateProcess();
	exit(0);
}

void RadioInterface::mprisVolume(double vol) {
    log(LOG_MPRIS, LOG_MIN, "mpris volume %f", vol);
    volumeKnob->setValue(vol * AUDIO_SCALE);
    soundOut->setVolume(qreal(vol));
}

void RadioInterface::mprisPlayButton() {
    log(LOG_MPRIS, LOG_MIN, "mpris play");
    handlePlayButton();
}

void RadioInterface::mprisPauseButton() {
    log(LOG_MPRIS, LOG_MIN, "mpris pause");
    handlePauseButton();
}

void RadioInterface::mprisPlayPause() {
    log(LOG_MPRIS, LOG_MIN, "mpris play/pause");
    if (playing)
	handlePauseButton();
    else
	handlePlayButton();
}

void RadioInterface::mprisNextButton() {
    log(LOG_MPRIS, LOG_MIN, "mpris next");
    if (skipPresetMode)
	changePreset(1);
    else
	changeStation(1);
}

void RadioInterface::mprisPreviousButton() {
    log(LOG_MPRIS, LOG_MIN, "mpris previous");
    if (skipPresetMode)
	changePreset(-1);
    else
	changeStation(-1);
}
#endif

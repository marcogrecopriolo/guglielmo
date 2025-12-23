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
#include <QApplication>
#include <QTranslator>
#include <QMessageBox>
#include <QWidget>
#include <QDialog>
#include <QStyle>
#include <QStyleFactory>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDropEvent>
#include <QMimeData>
#include <QString>
#include "Qt-audio.h"
#include "audiosink.h"
#include "settings.h"
#include "radio.h"
#include "fm-demodulator.h"
#include "ui_about.h"
#include "listwidget.h"
#include "logging.h"
#define STRBUFLEN 256

// yes / no confirmation
bool yesNo(QWidget *parent) {
    log(LOG_UI, LOG_MIN, "yes / no dialog");
    QMessageBox::StandardButton resultButton = QMessageBox::question(parent, TARGET,
		QWidget::tr("Are you sure?"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    return resultButton == QMessageBox::Yes;
}

// generic warning
void warning(QWidget *parent, QString what) {
    log(LOG_UI, LOG_MIN, "warning %s", qPrintable(what));
    QMessageBox::warning(parent, QWidget::tr("Warning"), what);
}

// open write file
QString chooseFileName(QWidget *parent, QSettings *settings, QString what, QString filters, QString state, QString fileName ) {
    QFileDialog *saveDialog = new QFileDialog(parent);

    log(LOG_UI, LOG_MIN, "choose file dialog");
    settings->beginGroup(GROUP_DIALOGS);
    QByteArray saveState = settings->value(state, "").toByteArray();
    settings->endGroup();
    saveDialog->restoreState(saveState);
    QString selectedFilter = saveDialog->selectedNameFilter();
    QString out = saveDialog->getSaveFileName(parent, what, saveDialog->directory().filePath(fileName), filters, &selectedFilter);

    // FIXME the current filter is not saved
    settings->beginGroup(GROUP_DIALOGS);
    saveState = saveDialog->saveState();
    settings->setValue(state, saveState);
    settings->endGroup();
    return out;
}

// "about" window
void RadioInterface::handleAboutAction() {
    QFile file(":/AUTHORS");
    QDialog *aboutDialog = new QDialog;
    Ui::aboutWindow aboutUi;

    log(LOG_UI, LOG_MIN, "about window");
    aboutUi.setupUi(aboutDialog);
//  aboutDialog->move(window()->frameGeometry().topLeft() +
//		window()->rect().center() - aboutDialog->rect().center());
    aboutUi.nameLabel->setText(TARGET);
    aboutUi.versionLabel->setText("Version: " CURRENT_VERSION);
    aboutUi.okButton->setIcon(aboutDialog->style()->standardIcon(QStyle::SP_DialogOkButton));
    aboutDialog->connect(aboutUi.okButton, SIGNAL(clicked(void)),
		aboutDialog, SLOT(close()));
    file.open(QIODevice::ReadOnly | QFile::Text);
    QTextStream f(&file);
    aboutUi.aboutLabel->setText(f.readAll());

    // avoid widget leaks
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

// "settings" window
void RadioInterface::handleSettingsAction() {
    log(LOG_UI, LOG_MIN, "settings window");
    if (settingsDialog == nullptr) {
	settingsDialog = new QDialog;

	settingsUi.setupUi(settingsDialog);
//	settingsDialog->move(window()->frameGeometry().topLeft() +
//		window()->rect().center() - settingsDialog->rect().center());
	settingsUi.closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	settingsDialog->connect(settingsUi.closeButton, SIGNAL(clicked(void)),
		settingsDialog, SLOT(close()));
	settingsDialog->connect(settingsDialog, SIGNAL(rejected()),
		this, SLOT(settingsClose(void)));

	// Presets tab
	settingsUi.presetList->setDragDropMode(QAbstractItemView::DragDrop);
	settingsUi.presetList->setAcceptDrops(true);
	settingsUi.presetList->setDefaultDropAction(Qt::MoveAction);
	settingsUi.scanList->setDragEnabled(true);
	settingsUi.stopScanButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	settingsUi.sortButton->setIcon(QIcon(":/sort.png"));
	descendingOrder = false;
	settingsDialog->connect(settingsUi.scanComboBox, SIGNAL(activated(int)),
		this, SLOT(startFullScan()));
	settingsDialog->connect(settingsUi.stopScanButton, SIGNAL(clicked()),
		this, SLOT(stopFullScan()));
	settingsDialog->connect(settingsUi.copyButton, SIGNAL(clicked()),
		this, SLOT(copyStation()));
	settingsDialog->connect(settingsUi.sortButton, SIGNAL(clicked()),
		this, SLOT(sortPresets()));
	settingsDialog->connect(settingsUi.minusButton, SIGNAL(clicked()),
		this, SLOT(dropPreset()));
	settingsDialog->connect(settingsUi.downButton, SIGNAL(clicked()),
		this, SLOT(lowerPreset()));
	settingsDialog->connect(settingsUi.upButton, SIGNAL(clicked()),
		this, SLOT(liftPreset()));
	settingsDialog->connect(settingsUi.clearButton, SIGNAL(clicked()),
		this, SLOT(clearScanList()));

	// UI tab
	QStringList list = QStyleFactory::keys();
	for (int i = 0; i < list.size(); i++) {
	    settingsUi.styleComboBox->insertItem(i, list[i]);
	    log(LOG_UI, LOG_CHATTY, "found style %s", qPrintable(list[i].toLower()));
	    if (list[i].toLower() == theme)
		settingsUi.styleComboBox->setCurrentIndex(i);
	}
	settingsDialog->connect(settingsUi.styleComboBox, SIGNAL(activated(int)),
		this, SLOT(setUiStyle(int)));
	QDir local(QString(":/skins"));
	QDir external(QString("%1/skins").arg(QDir::home().absoluteFilePath(LOCAL_STORAGE)));
	QStringList skins = local.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	int count;

	settingsUi.skinComboBox->addItem("none");
	for (count = 0; count < skins.size(); count++) {
	    settingsUi.skinComboBox->addItem(skins[count]);
	    settingsUi.skinComboBox->setItemData(count, true);
	    if (skins[count].toLower() == skin && skinIsLocal)
		settingsUi.skinComboBox->setCurrentIndex(count+1);
	}
	count++;
	skins = external.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int i = 0; i < skins.size(); i++) {
	    settingsUi.skinComboBox->addItem(skins[i]);
	    settingsUi.skinComboBox->setItemData(i+count, false);
	    if (skins[i].toLower() == skin && !skinIsLocal)
		settingsUi.skinComboBox->setCurrentIndex(i+count);
	}
	settingsDialog->connect(settingsUi.skinComboBox, SIGNAL(activated(int)),
		this, SLOT(setUiSkin(int)));

	// MPRIS
#ifndef HAVE_MPRIS

	// setTabVisible only exists in 5.15
	if (settingsUi.tabWidget->tabText(2) == "Remote control")
	    settingsUi.tabWidget->removeTab(2);
#else
	settingsDialog->connect(settingsUi.remoteComboBox, SIGNAL(activated(int)),
		this, SLOT(setRemoteMode(int)));
    	if (skipPresetMode)
	    settingsUi.remoteComboBox->setCurrentIndex(0);
	else
	    settingsUi.remoteComboBox->setCurrentIndex(1);
#endif

	// Sound tab
    	if (isQtAudio) {
	    settingsUi.modeComboBox->setCurrentIndex(0);
	    settingsUi.alsaWidget->hide();
	} else {

	    // channels are already set up
	    setSoundChannels();
	    settingsUi.outputComboBox->setCurrentIndex(soundChannel);
	    settingsUi.modeComboBox->setCurrentIndex(1);
	}
	settingsUi.latencySpinBox->setValue(latency);
	settingsDialog->connect(settingsUi.modeComboBox, SIGNAL(activated(int)),
		this, SLOT(setSoundMode(int)));
	settingsDialog->connect(settingsUi.outputComboBox, SIGNAL(activated(int)),
		this, SLOT(setSoundOutput(int)));
	settingsDialog->connect(settingsUi.latencySpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(setLatency(int)));

	// FM tab
	settingsUi.decoderComboBox->setCurrentIndex(FMdecoder);
	if (deemphasis == 1)
	    settingsUi.deemphasisComboBox->setCurrentIndex(0);
	else
	    settingsUi.deemphasisComboBox->setCurrentText(QString::number(deemphasis));
	if (FMfilter <= 0)
	    settingsUi.fmFilterComboBox->setCurrentIndex(0);
	else
	    settingsUi.fmFilterComboBox->setCurrentText(QString::number(FMfilter/1000));
	settingsUi.fmDegreeFilterSpinBox->setValue(FMdegree);
	if (lowPassFilter <= 0)
	    settingsUi.lowPassComboBox->setCurrentIndex(0);
	else
	    settingsUi.lowPassComboBox->setCurrentText(QString::number(lowPassFilter));
	settingsUi.fmAudioGainSpinBox->setValue(FMaudioGain);
	settingsDialog->connect(settingsUi.decoderComboBox, SIGNAL(activated(int)),
		this, SLOT(setDecoder(int)));
	settingsDialog->connect(settingsUi.deemphasisComboBox, SIGNAL(activated(int)),
		this, SLOT(setDeemphasis(int)));
	settingsDialog->connect(settingsUi.fmFilterComboBox, SIGNAL(activated(int)),
		this, SLOT(setFMFilter(int)));
	settingsDialog->connect(settingsUi.fmDegreeFilterSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(setFMDegree(int)));
	settingsDialog->connect(settingsUi.lowPassComboBox, SIGNAL(activated(int)),
		this, SLOT(setLowPassFilter(int)));
	settingsDialog->connect(settingsUi.fmAudioGainSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(setFMaudioGain(int)));

	// Device tab
	for (const auto &dev: deviceList) {
	    settingsUi.deviceComboBox->addItem(dev.deviceType);
	    if (inputDevice == dev.device)
		settingsUi.deviceComboBox->setCurrentIndex(settingsUi.deviceComboBox->count()-1);
	}

	int dc;
	deviceStrings devNames[MAX_DEVICES];

	// show the end of the device name
	if (inputDevice == NULL || (dc = inputDevice->devices((deviceStrings *) &devNames, MAX_DEVICES)) == 0)
	    settingsUi.deviceNameComboBox->setEnabled(false);
	else 
	    for (int i = 0; i < dc; i++) {
		settingsUi.deviceNameComboBox->addItem(devNames[i].name);
		settingsUi.deviceNameComboBox->setItemData(i, QString(devNames[i].id), Qt::UserRole);
		if (strlen(devNames[i].description) > 0)
		    settingsUi.deviceNameComboBox->setItemData(i, QString(devNames[i].description), Qt::ToolTipRole);
		if (devNames[i].id == deviceId)
		    settingsUi.deviceNameComboBox->setCurrentIndex(i);
	    }
	settingsUi.deviceNameComboBox->view()->setTextElideMode(Qt::ElideLeft);
	if (inputDevice != nullptr) {
	    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_ON, !(deviceUiControls & HW_AGC));
	    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_SOFTWARE, !(deviceUiControls & SW_AGC));
	    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_COMBINED, !(deviceUiControls & COMBO_AGC));
	    if (qobject_cast<QListView*> (settingsUi.agcComboBox->view())->isRowHidden(agc)) {
		inputDevice->setAgcControl(false);
		settingsUi.agcComboBox->setCurrentIndex(0);
	    } else
		settingsUi.agcComboBox->setCurrentIndex(agc);
	    if (deviceUiControls & IF_GAIN) {
		settingsUi.gainSpinBox->setMinimum(minIfGain);
		settingsUi.gainSpinBox->setMaximum(maxIfGain);
		settingsUi.gainSpinBox->setValue(ifGain);
	    } else {
		settingsUi.gainSpinBox->setEnabled(false);
		settingsUi.gainSpinBox->setValue(0);
	    }
	    if (deviceUiControls & LNA_GAIN) {
		int min, max;

		inputDevice->getLnaRange(&min, &max);
		settingsUi.lnaSpinBox->setMinimum(min);
		settingsUi.lnaSpinBox->setMaximum(max);
		settingsUi.lnaSpinBox->setValue(lnaGain);
	    } else {
		settingsUi.lnaSpinBox->setEnabled(false);
		settingsUi.lnaSpinBox->setValue(0);
	    }
	} else {
	    settingsUi.agcComboBox->setCurrentIndex(0);
	    settingsUi.agcComboBox->setEnabled(false);
	    settingsUi.gainSpinBox->setEnabled(false);
	    settingsUi.lnaSpinBox->setEnabled(false);
	}
	settingsDialog->connect(settingsUi.deviceComboBox, SIGNAL(activated(int)),
		this, SLOT(setDevice(int)));
	settingsDialog->connect(settingsUi.deviceNameComboBox, SIGNAL(activated(int)),
		this, SLOT(setDeviceName(int)));
	settingsDialog->connect(settingsUi.agcComboBox, SIGNAL(activated(int)),
		this, SLOT(setAgcControl(int)));
	settingsDialog->connect(settingsUi.gainSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(setIfGain(int)));
	settingsDialog->connect(settingsUi.lnaSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(setLnaGain(int)));
    }
    settingsDialog->setWindowTitle(originalTitle);
    settingsUi.presetList->clear();
    for (int i = 1; i < presetSelector->count(); i ++)
	settingsUi.presetList->addItem(presetSelector->itemText(i));
    settingsUi.decoderComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.deemphasisComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.lowPassComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.fmFilterComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.fmDegreeFilterSpinBox->setEnabled(FMprocessor != nullptr);
    settingsUi.lowPassComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.fmAudioGainSpinBox->setEnabled(FMprocessor != nullptr);

    settingsDialog->show();
}

// list widgets with duplicate check on drop
// used by the presets tab
void ListWidget::dropEvent(QDropEvent *event) {
    if (event->proposedAction() == Qt::CopyAction) {
	QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
	QDataStream stream(&encoded, QIODevice::ReadOnly);
	QStringList items;

	while (!stream.atEnd()) {
		int row, col;
		QMap<int,  QVariant> roleDataMap;
		stream >> row >> col >> roleDataMap;
		items << roleDataMap[Qt::DisplayRole].toString();
	}
	log(LOG_UI, LOG_MIN, "searching %s", qPrintable(items.join(",")));
	for (const auto &item: items)
	    if (findItems(item, Qt::MatchStartsWith).size() > 0) {
		log(LOG_UI, LOG_MIN, "entry %s already present", qPrintable(item));
		return;
	    }
    }
    QListWidget::dropEvent(event);
}

void RadioInterface::settingsClose(void) {
    log(LOG_UI, LOG_MIN, "close settings");
    if (scanning)
	stopFullScan();

    // Presets tab
    if (settingsUi.presetList->count() > 0) {

	// only reloaded if a scan is actually done because it is prone to segv
	// if a skin is changed
	QString p = presetSelector->itemText(0);

	presetSelector->clear();
	presetSelector->addItem(p);
	for (int i = 0; i < settingsUi.presetList->count(); i ++)
	    presetSelector->addItem(settingsUi.presetList->item(i)->text());
    }

    // Ui tab
    settings->beginGroup(GROUP_UI);
    settings->setValue(UI_SKIN, skin);
    settings->setValue(UI_SKIN_LOCAL, skinIsLocal);
    settings->setValue(UI_THEME, theme);
    settings->endGroup();

    // Sound tab
    settings->beginGroup(GROUP_SOUND);
    settings->setValue(SOUND_MODE, isQtAudio? SOUND_QT: SOUND_PORTAUDIO);
    settings->setValue(SOUND_LATENCY, latency);
    settings->setValue(SOUND_CHANNEL, soundChannel);
    settings->endGroup();

    // FM tab
    settings->beginGroup(GROUP_FM);
    settings->setValue(FM_FILTER, FMfilter);
    settings->setValue(FM_DEGREE, FMdegree);
    settings->setValue(FM_DECODER, FMdecoder);
    settings->setValue(FM_DEEMPHASIS, deemphasis);
    settings->setValue(FM_LOW_PASS_FILTER, lowPassFilter);
    settings->setValue(FM_AUDIO_GAIN, FMaudioGain);
    settings->endGroup();

    // Device tab
    settings->beginGroup(deviceType);
    if (settingsUi.agcComboBox->isEnabled())
	settings->setValue(DEV_AGC, agc);
    if (settingsUi.gainSpinBox->isEnabled())
	settings->setValue(DEV_IF_GAIN, ifGain);
    if (settingsUi.lnaSpinBox->isEnabled())
	settings->setValue(DEV_LNA_GAIN, lnaGain);
    settings->setValue(DEV_ID, deviceId);
    settings->endGroup();
}

void RadioInterface::startFullScan() {
    QString scanType = settingsUi.scanComboBox->currentText();

    log(LOG_UI, LOG_MIN, "start full %s scan", qPrintable(scanType));
    if (settingsUi.scanComboBox->currentIndex() == 0)
	return;
    settingsUi.scanComboBox->setVisible(false);
    settingsUi.scanComboBox->setCurrentIndex(0);
    saveIsFM = isFM;
    saveFMfreq = FMfreq;
    saveChannel = channelSelector->currentIndex();
    saveService = currentService;
    if (playing) {
	if (isFM)
	    stopFM();
	else
	    stopDAB();
	usleep(1000);
    }
    if (scanType == "FM" && FMprocessor != NULL) {
	handleFMButton();
	scanIncrement = 1;
	FMfreq = MIN_FM;
	stopFM();
	if (scanTimer == nullptr) {
	    scanTimer = new QTimer();
	    scanTimer->setInterval(scanInterval);
	}
	connect(scanTimer, SIGNAL(timeout()),
	    this, SLOT(nextFullScanFrequency()));
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
	frequencyKnob->setValue(double(FMfreq));
	frequencyLCD->display(int(FMfreq*1000));
	inputDevice->restartReader(int(FMfreq*1000000));
	FMprocessor->start();
	FMprocessor->startFullScan();
	scanTimer->start();
    } else if (scanType == "DAB") {
	handleDABButton();
	stopDAB();
	if (scanTimer == nullptr) {
	    scanTimer = new QTimer();
	    scanTimer->setInterval(scanInterval);
	}
	connect(scanTimer, SIGNAL(timeout()),
	    this, SLOT(nextFullDABScan()));
	connect(this, SIGNAL(advanceScan(int)),
	    this, SLOT(ensembleLoaded(int)));
	scanRetryCount = 0;
	playing = false;
	scanning = true;
	cleanScreen();
	setPlaying();
	setRecording();
	setScanning();
#ifdef HAVE_MPRIS
	mprisLabelAndText("DAB", "Scanning");
	player.setPlaybackStatus(Mpris::Stopped);
#endif
	scanIndex = 0;
	channelSelector->setCurrentIndex(scanIndex);
	startDAB();
	scanTimer->start();
    }
}

void RadioInterface::stopFullScan() {
    if (scanTimer != nullptr && scanTimer->isActive())
	scanTimer->stop();
    scanning = false;
    if (isFM) {
	log(LOG_UI, LOG_MIN, "stop full FM scan");
	disconnect(scanTimer, SIGNAL(timeout()),
	    this, SLOT(nextFullScanFrequency()));
	FMprocessor->stopFullScan();
	stopFM();
    } else {
	log(LOG_UI, LOG_MIN, "stop full DAB scan");
	stopDAB();
	disconnect(scanTimer, SIGNAL(timeout()),
	    this, SLOT(nextFullDABScan()));
	disconnect(this, SIGNAL(advanceScan(int)),
	    this, SLOT(ensembleLoaded(int)));
    }
    isFM = saveIsFM;
    FMfreq = saveFMfreq;
    channelSelector->setCurrentIndex(saveChannel);
    currentService = saveService;
    if (isFM) {
	toFM();
	frequencyLCD->display(int(FMfreq*1000));
    } else {
	toDAB();
	currentService.valid = (currentService.serviceName != "");
	startDAB();
    }
    setPlaying();
    setRecording();
    setScanning();
    settingsUi.scanComboBox->setVisible(true);
}

void RadioInterface::nextFullScanFrequency(void) {
    log(LOG_EVENT, LOG_MIN, "fm full scan timer signal");
    if (FMfreq >= MAX_FM) {
	stopFullScan();
    } else {
	FMprocessor->stopFullScan();
	FMprocessor->stop();
	inputDevice->stopReader();
	FMfreq = (FMfreq*10+scanIncrement)/10;
	frequencyKnob->setValue(double(FMfreq));
	frequencyLCD->display(int(FMfreq*1000));
	inputDevice->restartReader(int(FMfreq*1000000));
	log(LOG_EVENT, LOG_MIN, "fm full scan next frequency %f", FMfreq);
	FMprocessor->start();
	FMprocessor->startFullScan();
	scanTimer->start();
    }
}

void RadioInterface::scanFound(void) {
    QString station = "FM:" + QString::number(FMfreq);

    if (settingsUi.scanList->findItems(station, Qt::MatchStartsWith).size() > 0) {
	log(LOG_EVENT, LOG_MIN, "station %s already present", qPrintable(station));
    } else {
	log(LOG_EVENT, LOG_MIN, "station found %s", qPrintable(station));
	settingsUi.scanList->addItem(station);
    }
}

void RadioInterface::nextFullDABScan(void) {
    log(LOG_EVENT, LOG_CHATTY, "dab full scan timer signal attempt %i", scanRetryCount+1);

    // nothing, either we stop, or move on
    if (serviceList.size() == 0) {
	if (signalStrength->value() < MIN_SCAN_SIGNAL || ++scanRetryCount > scanRetry) {
	    scanRetryCount = 0;
	    if (scanIndex ==  channelSelector->count() - 1) {
		stopFullScan();
		return;
	    } else {
		scanIndex++;
		stopDAB();
		channelSelector->setCurrentIndex(scanIndex);
		log(LOG_EVENT, LOG_MIN, "dab full scan next frequency %s", qPrintable(channelSelector->currentText()));
		startDAB();
	    }
	}
    } else if (++scanRetryCount > scanRetry) {
        log(LOG_EVENT, LOG_CHATTY, "dab full scan timeout for %s", qPrintable(channelSelector->currentText()));
	emit advanceScan(-1);
	return;
    }
    scanTimer->start();
}

void RadioInterface::scanEnsembleLoaded() {
    scanRetryCount = 0;
    for (const auto &serv: serviceList) {
	QString station = channelSelector->itemText(scanIndex) + ":" +serv.name;

	if (settingsUi.scanList->findItems(station, Qt::MatchStartsWith).size() > 0) {
	    log(LOG_EVENT, LOG_MIN, "dab station %s already present", qPrintable(station));
	} else {
	    log(LOG_EVENT, LOG_MIN, "dab station found %s", qPrintable(station));
	    settingsUi.scanList->addItem(station);
	}
    }
    if (scanIndex ==  channelSelector->count() - 1) {
	stopFullScan();
    } else {
	scanIndex++;
	stopDAB();
	channelSelector->setCurrentIndex(scanIndex);
	log(LOG_EVENT, LOG_MIN, "dab full scan next frequency %s", qPrintable(channelSelector->currentText()));
	startDAB();
	scanTimer->start();
    }
}

void RadioInterface::clearScanList() {
    log(LOG_UI, LOG_MIN, "clear scan list");
    settingsUi.scanList->clear();
}

void RadioInterface::copyStation() {
    int row = settingsUi.scanList->currentRow();

    if (row < 0) {
	log(LOG_UI, LOG_MIN, "add station: no row selected");
    } else {
	QString station = settingsUi.scanList->currentItem()->text();

	if (settingsUi.presetList->findItems(station, Qt::MatchStartsWith).size() > 0) {
	    log(LOG_UI, LOG_MIN, "station %s already present", qPrintable(station));
	} else {
	    log(LOG_UI, LOG_MIN, "add station %s", qPrintable(station));
	    settingsUi.presetList->addItem(settingsUi.scanList->currentItem()->text());
	}
    }
}

void RadioInterface::dropPreset() {
    int row = settingsUi.presetList->currentRow();

    if (row < 0) {
	log(LOG_UI, LOG_MIN, "drop preset: no row selected");
    } else {
	log(LOG_UI, LOG_MIN, "drop preset %s", qPrintable(settingsUi.presetList->currentItem()->text()));
	settingsUi.presetList->takeItem(row);
    }
}

void RadioInterface::lowerPreset() {
    int row = settingsUi.presetList->currentRow();

    if (row < 0) {
	log(LOG_UI, LOG_MIN, "move preset down: no row selected");
    } else if (row == settingsUi.presetList->count()-1) {
	log(LOG_UI, LOG_MIN, "move preset down: item is last");
    } else {
	QListWidgetItem *item = settingsUi.presetList->takeItem(row);

	log(LOG_UI, LOG_MIN, "move preset down %s", qPrintable(item->text()));
	settingsUi.presetList->insertItem(row+1, item);
	settingsUi.presetList->setCurrentRow(row+1);
    }
}

void RadioInterface::liftPreset() {
    int row = settingsUi.presetList->currentRow();

    if (row < 0) {
	log(LOG_UI, LOG_MIN, "move preset up: item is first");
    } else if (row == 0) {
	log(LOG_UI, LOG_MIN, "move preset up: item is last");
    } else {
	QListWidgetItem *item = settingsUi.presetList->takeItem(row);

	log(LOG_UI, LOG_MIN, "move preset up %s", qPrintable(item->text()));
	settingsUi.presetList->insertItem(row-1, item);
	settingsUi.presetList->setCurrentRow(row-1);
    }
}

void RadioInterface::sortPresets() {
    log(LOG_UI, LOG_MIN, "sort entries %i", descendingOrder);
    if (descendingOrder)
	settingsUi.presetList->sortItems(Qt::DescendingOrder);
    else
	settingsUi.presetList->sortItems(Qt::AscendingOrder);
    descendingOrder = !descendingOrder;
}

void RadioInterface::setUiStyle(int index) {
    theme = settingsUi.styleComboBox->itemText(index).toLower();

    log(LOG_UI, LOG_MIN, "style %s", qPrintable(theme));
    QApplication::setStyle(theme);
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}

void RadioInterface::setUiSkin(int index) {
    QString skinText;

    if (index == 0)
	skin = QString("");
    else {
	skin = settingsUi.skinComboBox->itemText(index);
	skinIsLocal = settingsUi.skinComboBox->itemData(index).toBool();
    }

    skinText = loadSkin();
    if (theme != settingsUi.styleComboBox->itemText(settingsUi.styleComboBox->currentIndex()).toLower()) {
	for (int i = 0; i < settingsUi.styleComboBox->count(); i++) {
	    if (settingsUi.styleComboBox->itemText(i).toLower() == theme)
		settingsUi.styleComboBox->setCurrentIndex(i);
	}
    }

    // it should not make any difference since the object is the same
    // but there's some race condition here if the skin is loaded directly
    emit newSkin(skinText);
}

#ifdef HAVE_MPRIS
void RadioInterface::setRemoteMode(int index) {
    QString mode = settingsUi.remoteComboBox->itemText(index);

    log(LOG_UI, LOG_MIN, "mpris mode %s", qPrintable(mode));
    skipPresetMode = (mode == "presets");
}
#endif

void RadioInterface::setSoundChannels() {
    int d = ((audioSink *) soundOut)->numberOfDevices();

    settingsUi.outputComboBox->clear();
    for (int i = 0; i < d; i++)
	settingsUi.outputComboBox->insertItem(i, ((audioSink *) soundOut)->outputChannel(i), QVariant(i));
}

void RadioInterface::setSoundMode(int index) {
    QString mode = settingsUi.modeComboBox->itemText(index);
    bool nextIsQtAudio = (mode == "Qt");
    bool stop = playing;

    log(LOG_UI, LOG_MIN, "audio mode %s", qPrintable(mode));
    if (nextIsQtAudio == isQtAudio)
	return;
    isQtAudio = nextIsQtAudio;

    if (stop) {
	if (isFM)
	    stopFM();
	else
	    stopDABService();
    }
    soundOut->stop();
    delete soundOut;
    if (isQtAudio) {
	soundOut = new Qt_Audio;
	settingsUi.alsaWidget->hide();
    } else {
	soundOut = new audioSink(latency);

	// FIXME audiosink requires that the combobox be populated every time
//	if (settingsUi.outputComboBox->currentIndex() < 0) {
	    settingsUi.outputComboBox->clear();
	    setSoundChannels();
//	}
	if (!((audioSink *) soundOut)->selectDevice(soundChannel)) {
	    soundChannel = ((audioSink *) soundOut)->defaultDevice();
	    ((audioSink *) soundOut)->selectDevice(soundChannel);
	}
	settingsUi.outputComboBox->setCurrentIndex(soundChannel);
	settingsUi.alsaWidget->show();
    }
    if (FMprocessor != nullptr)
	FMprocessor->setSink(soundOut);
    soundOut->setVolume(volumeKnob->value()/100);
    if (stop) {
	if (isFM)
	    startFM(int(FMfreq*1000000));
	else
	    startDABService(&currentService);
    }
}

void RadioInterface::setSoundOutput(int c) {
    log(LOG_UI, LOG_MIN, "sound channel %i", c);
    soundChannel = c;
    if (!((audioSink *) soundOut)->selectDevice(soundChannel)) {
	soundChannel = ((audioSink *) soundOut)->defaultDevice();
	((audioSink *) soundOut)->selectDevice(soundChannel);
	settingsUi.outputComboBox->setCurrentIndex(soundChannel);
    }
}

void RadioInterface::setLatency(int newLatency) {
    bool stop = playing;

    log(LOG_UI, LOG_MIN, "sound latency %i", newLatency);
    if (latency == newLatency)
	return;
    if (stop) {
	if (isFM)
	    stopFM();
	else
	    stopDABService();
    }
    soundOut->stop();
    delete soundOut;
    latency = newLatency;
    soundOut = new audioSink(latency);
    setSoundChannels();
    soundOut->setVolume(volumeKnob->value()/100);
    if (!((audioSink *) soundOut)->selectDevice(soundChannel)) {
	soundChannel = ((audioSink *) soundOut)->defaultDevice();
	((audioSink *) soundOut)->selectDevice(soundChannel);
    }
    settingsUi.outputComboBox->setCurrentIndex(soundChannel);
    if (stop) {
	if (isFM)
	    startFM(int(FMfreq*1000000));
	else
	    startDABService(&currentService);
    }
}

void RadioInterface::setDecoder(int index) {
    QString decoder = settingsUi.decoderComboBox->itemText(index);

    log(LOG_UI, LOG_MIN, "fm decoder %i %s", index, qPrintable(decoder));
    FMprocessor->setFMDecoder(fmDemodulator::fm_demod(index));
    FMdecoder = index;
}

void RadioInterface::setDeemphasis(int index) {
    QString v = settingsUi.deemphasisComboBox->itemText(index);

    log(LOG_UI, LOG_MIN, "fm deemphasis %s", qPrintable(v));
    if (v == "None")
	deemphasis = 1;
    else
	deemphasis = v.toInt();
    FMprocessor->setDeemphasis((int32_t) deemphasis);
}

void RadioInterface::setFMFilter(int index) {
    QString v = settingsUi.fmFilterComboBox->itemText(index);

    log(LOG_UI, LOG_MIN, "fm filter %s", qPrintable(v));
    if (v == "None")
	FMfilter = 0;
    else
	FMfilter = v.toInt()*1000;
    FMprocessor->setBandwidth(FMfilter);
}

void RadioInterface::setFMDegree(int degree) {
    log(LOG_UI, LOG_MIN, "fm degree %i", degree);
    FMprocessor->setBandFilterDegree(degree);
    FMdegree = degree;
}

void RadioInterface::setLowPassFilter(int index) {
    QString v = settingsUi.lowPassComboBox->itemText(index);

    log(LOG_UI, LOG_MIN, "fm low pass filter %s", qPrintable(v));
    if (v == "None")
	lowPassFilter = 0;
    else
	lowPassFilter = v.toInt();
    FMprocessor->setAudioBandwidth((int32_t) lowPassFilter);
}

void RadioInterface::setFMaudioGain(int gain) {
    log(LOG_UI, LOG_MIN, "fm audio gain %i", gain);
    FMprocessor->setAudioGain(gain);
    FMaudioGain = gain;
}

void RadioInterface::setDevice(int d) {
    log(LOG_UI, LOG_MIN, "device %i", d);
    if (deviceList[d].device == inputDevice)
	return;

    settings->beginGroup(deviceType);
    if (settingsUi.agcComboBox->isEnabled()) {
	settings->setValue(DEV_AGC, agc);
    } else
	settingsUi.agcComboBox->setEnabled(true);
    if (settingsUi.gainSpinBox->isEnabled()) {
	settings->setValue(DEV_IF_GAIN, settingsUi.gainSpinBox->value());
    } else
	settingsUi.gainSpinBox->setEnabled(true);
    if (settingsUi.lnaSpinBox->isEnabled()) {
	settings->setValue(DEV_LNA_GAIN, settingsUi.lnaSpinBox->value());
    } else
	settingsUi.lnaSpinBox->setEnabled(true);
    settings->setValue(DEV_ID, deviceId);
    settings->endGroup();

    if (isFM) {
	if (playing)
	    stopFM();
    } else
	stopDAB();

    delete DABprocessor;
    delete FMprocessor;

    inputDevice = deviceList[d].device;
    deviceType = deviceList[d].deviceType;
    deviceUiControls = deviceList[d].controls;
    makeDABprocessor();
    makeFMprocessor();

    // do it all again for the new device
    settings->beginGroup(deviceType);
    agc = settings->value(DEV_AGC, DEV_DEF_AGC).toInt();
    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_ON, !(deviceUiControls & HW_AGC));
    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_SOFTWARE, !(deviceUiControls & SW_AGC));
    qobject_cast<QListView*> (settingsUi.agcComboBox->view())->setRowHidden(AGC_COMBINED, !(deviceUiControls & COMBO_AGC));
    if (qobject_cast<QListView*> (settingsUi.agcComboBox->view())->isRowHidden(agc)) {
	inputDevice->setAgcControl(false);
	settingsUi.agcComboBox->setCurrentIndex(0);
    } else
	settingsUi.agcComboBox->setCurrentIndex(agc);
    deviceId = settings->value(DEV_ID, "").toString();

    // set device model first...
    deviceStrings devNames[MAX_DEVICES];
    int dc = inputDevice->devices((deviceStrings *) &devNames, MAX_DEVICES);
    settingsUi.deviceNameComboBox->clear();
    if (dc <= 0) {
	settingsUi.deviceNameComboBox->setEnabled(false);
	deviceId = "";
    } else {
	settingsUi.deviceNameComboBox->setEnabled(true);
	for (int i = 0; i < dc; i++) {
	    settingsUi.deviceNameComboBox->addItem(devNames[i].name);
	    settingsUi.deviceNameComboBox->setItemData(i, QString(devNames[i].id), Qt::UserRole);
	    if (strlen(devNames[i].description) > 0)
		settingsUi.deviceNameComboBox->setItemData(i, QString(devNames[i].description), Qt::ToolTipRole);
	    if (devNames[i].id == deviceId)
		settingsUi.deviceNameComboBox->setCurrentIndex(i);
	}
    }

    // ...and settings later, as the LNA is known to change depending on model
    inputDevice->getIfRange(&minIfGain, &maxIfGain);
    inputDevice->getLnaRange(&minLnaGain, &maxLnaGain);
    if (qobject_cast<QStandardItemModel*> (settingsUi.agcComboBox->model())->item(agc)->isEnabled()) {
	inputDevice->setAgcControl(agc == AGC_ON);
	settingsUi.agcComboBox->setCurrentIndex(agc);
    } else {
	inputDevice->setAgcControl(false);
	settingsUi.agcComboBox->setCurrentIndex(0);
    }
    if (deviceUiControls & IF_GAIN) {
	ifGain = settings->value(DEV_IF_GAIN, (minIfGain + maxIfGain) / 2).toInt();
	checkIfGain();
	inputDevice->setIfGain(ifGain);
	settingsUi.gainSpinBox->setMinimum(minIfGain);
	settingsUi.gainSpinBox->setMaximum(maxIfGain);
	settingsUi.gainSpinBox->setValue(ifGain);
    } else {
	settingsUi.gainSpinBox->setEnabled(false);
	settingsUi.gainSpinBox->setValue(0);
    }
    if (deviceUiControls & LNA_GAIN) {
	lnaGain = settings->value(DEV_LNA_GAIN, minLnaGain).toInt();
	checkLnaGain();
	inputDevice->setLnaGain(lnaGain);
	settingsUi.lnaSpinBox->setMinimum(minLnaGain);
	settingsUi.lnaSpinBox->setMaximum(maxLnaGain);
	settingsUi.lnaSpinBox->setValue(lnaGain);
    } else {
	settingsUi.lnaSpinBox->setEnabled(false);
	settingsUi.lnaSpinBox->setValue(0);
    }
    settings->endGroup();

    // reset software agc
    resetSwAgc();
    resetAgcStats();

    makeDABprocessor();
    makeFMprocessor();
    if (!isFM)
	startDAB();
    currentService.setValid();
}

void RadioInterface::setDeviceName(int d) {
    QString id;

    log(LOG_UI, LOG_MIN, "device name %s", qPrintable(settingsUi.deviceNameComboBox->itemText(d)));
    id = settingsUi.deviceNameComboBox->itemData(d, Qt::UserRole).toString();

    if (id == deviceId)
	return;

    if (isFM) {
	if (playing)
	    stopFM();
    } else
	stopDAB();

    delete DABprocessor;
    delete FMprocessor;

    if (inputDevice->setDevice(qPrintable(id)))
	deviceId = id;

    // reset LNA, as it changes with model
    settings->beginGroup(deviceType);
    if (deviceUiControls & LNA_GAIN) {
	inputDevice->getLnaRange(&minLnaGain, &maxLnaGain);
	lnaGain = settings->value(DEV_LNA_GAIN, minLnaGain).toInt();
	checkLnaGain();
	inputDevice->setLnaGain(lnaGain);
	settingsUi.lnaSpinBox->setMinimum(minLnaGain);
	settingsUi.lnaSpinBox->setMaximum(maxLnaGain);
	settingsUi.lnaSpinBox->setValue(lnaGain);
    } else {
	settingsUi.lnaSpinBox->setEnabled(false);
	settingsUi.lnaSpinBox->setValue(lnaGain);
    }
    settings->endGroup();
 
    // reset software agc
    resetSwAgc();
    resetAgcStats();

    makeDABprocessor();
    makeFMprocessor();
    if (!isFM)
	startDAB();
    currentService.setValid();
}

void RadioInterface::setAgcControl(int newAgc) {
    int oldAgc = agc;

    log(LOG_UI, LOG_MIN, "AGC %i", newAgc);
    if (newAgc != agc) {
	if (inputDevice != NULL)
	    inputDevice->setAgcControl((newAgc == AGC_ON || newAgc == AGC_COMBINED));
	agc = newAgc;
	if (oldAgc == AGC_SOFTWARE || oldAgc == AGC_COMBINED)
	    inputDevice->setIfGain(ifGain);
	else
	    swAgc = ifGain;
	resetAgcStats();
    }
}

void RadioInterface::setIfGain(int gain) {
    int oldGain = ifGain;

    log(LOG_UI, LOG_MIN, "IF %i", gain);
    if (inputDevice != NULL)
	inputDevice->setIfGain(gain);
    ifGain = gain;
    if (gain != oldGain) {
	swAgc = ifGain;
	resetAgcStats();
    }
}

void RadioInterface::setLnaGain(int gain) {
    log(LOG_UI, LOG_MIN, "LNA %i", gain);
    if (inputDevice != NULL)
	inputDevice->setLnaGain(gain);
    lnaGain = gain;
}

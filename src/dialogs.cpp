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
#include <QDebug>
#include "Qt-audio.h"
#include "audiosink.h"
#include "settings.h"
#include "radio.h"
#include "fm-demodulator.h"
#include "ui_about.h"

// yes / no confirmation
bool yesNo(QWidget *parent) {
    QMessageBox::StandardButton resultButton = QMessageBox::question(parent, TARGET,
		QWidget::tr("Are you sure?"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    return resultButton == QMessageBox::Yes;
}

// generic warning
void warning(QWidget *parent, QString what) {
     QMessageBox::warning(parent, QWidget::tr("Warning"), what);
}

// "about" window
void RadioInterface::handleAboutAction() {
    QFile file(":/AUTHORS");
    QDialog *aboutDialog = new QDialog;
    Ui::aboutWindow aboutUi;

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
    if (settingsDialog == nullptr) {
	settingsDialog = new QDialog;
	settingsUi.setupUi(settingsDialog);
	settingsDialog->setWindowTitle(windowTitle());
//	settingsDialog->move(window()->frameGeometry().topLeft() +
//		window()->rect().center() - settingsDialog->rect().center());
	settingsUi.closeButton->setIcon(settingsDialog->style()->standardIcon(QStyle::SP_DialogCloseButton));
	settingsDialog->connect(settingsUi.closeButton, SIGNAL(clicked(void)),
		settingsDialog, SLOT(close()));
	settingsDialog->connect(settingsDialog, SIGNAL(rejected()),
		this, SLOT(settingsClose(void)));

	// UI tab
	QStringList list = QStyleFactory::keys();
	QString currentStyle = QApplication::style()->objectName().toLower();
	for (int i = 0; i < list.size(); i++) {
		settingsUi.styleComboBox->insertItem(i, list[i]);
		if (list[i].toLower() == currentStyle)
			settingsUi.styleComboBox->setCurrentIndex(i);
	}
	settingsDialog->connect(settingsUi.styleComboBox, SIGNAL(activated (const QString &)),
        	this, SLOT (setUiStyle(const QString &)));

	// Sound tab
	settingsDialog->connect(settingsUi.modeComboBox, SIGNAL(activated(const QString &)),
        	this, SLOT(setSoundMode(const QString &)));
	settingsDialog->connect(settingsUi.outputComboBox, SIGNAL(activated(int)),
        	this, SLOT(setSoundOutput(int)));
	settingsDialog->connect(settingsUi.latencySpinBox, SIGNAL(valueChanged(int)),
        	this, SLOT(setLatency(int)));
    	if (isQtAudio) {
	    settingsUi.modeComboBox->setCurrentIndex(0);
	    settingsUi.alsaWidget->hide();
	} else {
	    settingsUi.modeComboBox->setCurrentIndex(1);
	    ((audioSink *) soundOut)->setupChannels(settingsUi.outputComboBox);
	    settingsUi.outputComboBox->setCurrentIndex(soundChannel);
	    settingsUi.latencySpinBox->setValue(latency);
	}

	// FM tab
	settingsUi.decoderComboBox->setCurrentIndex(FMdecoder);
	if (deemphasis == 1)
	    settingsUi.deemphasisComboBox->setCurrentIndex(0);
	else
	    settingsUi.deemphasisComboBox->setCurrentText(QString::number(deemphasis));
	if (lowPassFilter < 0)
	    settingsUi.lowPassComboBox->setCurrentIndex(0);
	else
	    settingsUi.lowPassComboBox->setCurrentText(QString::number(lowPassFilter));
	if (FMfilter == 0.95*fmRate)
	    settingsUi.fmFilterComboBox->setCurrentIndex(0);
	else
	    settingsUi.fmFilterComboBox->setCurrentText(QString::number(FMfilter/1000));
	settingsUi.fmDegreeFilterSpinBox->setValue(FMdegree);
	settingsDialog->connect(settingsUi.decoderComboBox, SIGNAL(activated(int)),
        	this, SLOT(setDecoder(int)));
	settingsDialog->connect(settingsUi.deemphasisComboBox, SIGNAL(activated(const QString &)),
        	this, SLOT(setDeemphasis(const QString &)));
	settingsDialog->connect(settingsUi.lowPassComboBox, SIGNAL(activated(const QString &)),
        	this, SLOT(setLowPassFilter(const QString &)));
	settingsDialog->connect(settingsUi.fmFilterComboBox, SIGNAL(activated(const QString &)),
        	this, SLOT(setFMFilter(const QString &)));
	settingsDialog->connect(settingsUi.fmDegreeFilterSpinBox, SIGNAL(valueChanged(int)),
        	this, SLOT(setFMDegree(int)));

	// Device tab
	for (const auto dev: deviceList) {
		settingsUi.deviceComboBox->addItem(dev.deviceName);
		if (inputDevice == dev.device)
		    settingsUi.deviceComboBox->setCurrentIndex(settingsUi.deviceComboBox->count()-1);
	}
	settingsDialog->connect(settingsUi.deviceComboBox, SIGNAL(activated(int)),
        	this, SLOT(setDevice(int)));
	if (inputDevice != nullptr) {
	    if (deviceUiControls & AGC) {
		settingsUi.agcCheckBox->setChecked(agc);
		settingsDialog->connect(settingsUi.agcCheckBox, SIGNAL(stateChanged(int)),
        		inputDevice, SLOT(setAgcControl(int)));
	    } else
		settingsUi.agcCheckBox->setEnabled(false);
	    if (deviceUiControls & IF_GAIN) {
		settingsUi.gainSpinBox->setValue(ifGain);
		settingsDialog->connect(settingsUi.gainSpinBox, SIGNAL(valueChanged(int)),
        		inputDevice, SLOT(setIfGain(int)));
	    } else
		settingsUi.gainSpinBox->setEnabled(false);
	    if (deviceUiControls & LNA_GAIN) {
		settingsUi.lnaSpinBox->setValue(lnaGain);
		settingsDialog->connect(settingsUi.lnaSpinBox, SIGNAL(valueChanged(int)),
			inputDevice, SLOT(setLnaGain(int)));
	    } else
		settingsUi.lnaSpinBox->setEnabled(false);
	} else {
	    settingsUi.agcCheckBox->setEnabled(false);
	    settingsUi.gainSpinBox->setEnabled(false);
	    settingsUi.lnaSpinBox->setEnabled(false);
	}
    }
    settingsUi.decoderComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.deemphasisComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.lowPassComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.fmFilterComboBox->setEnabled(FMprocessor != nullptr);
    settingsUi.fmDegreeFilterSpinBox->setEnabled(FMprocessor != nullptr);

    settingsDialog->show();
}

void RadioInterface::settingsClose(void) {
    // Ui tab
    settings->beginGroup(GROUP_UI);
    settings->setValue(UI_THEME, QApplication::style()->objectName().toLower());
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
    settings->endGroup();

    // Device tab
    settings->beginGroup(deviceName);
    if (settingsUi.agcCheckBox->isEnabled())
	settings->setValue(DEV_AGC, settingsUi.agcCheckBox->isChecked()? "1": "0");
    if (settingsUi.gainSpinBox->isEnabled())
	settings->setValue(DEV_IF_GAIN, settingsUi.gainSpinBox->value());
    if (settingsUi.lnaSpinBox->isEnabled())
	settings->setValue(DEV_LNA_GAIN, settingsUi.lnaSpinBox->value());
    settings->endGroup();
}

void RadioInterface::setUiStyle(const QString &style) {
    QApplication::setStyle(style);
}

void RadioInterface::setSoundMode(const QString &mode) {
    bool nextIsQtAudio = (mode == "Qt");
    bool stop = playing;

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
	    ((audioSink *) soundOut)->setupChannels(settingsUi.outputComboBox);
	    settingsUi.outputComboBox->setCurrentIndex(soundChannel);
//	}
	((audioSink *) soundOut)->selectDevice(soundChannel);
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
    ((audioSink *) soundOut)->selectDevice(int16_t(c));
}

void RadioInterface::setLatency(int newLatency) {
    bool stop = playing;

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
    soundOut->setVolume(volumeKnob->value()/100);
    ((audioSink *) soundOut)->selectDevice(soundChannel);
    if (stop) {
	if (isFM)
            startFM(int(FMfreq*1000000));
	else
            startDABService(&currentService);
    }
}

void RadioInterface::setDecoder(int decoder) {
    FMprocessor->setFMdecoder(fm_Demodulator::fm_demod(decoder));
    FMdecoder = decoder;
}

void RadioInterface::setDeemphasis(const QString &v) {
    if (v == "None")
	deemphasis = 1;
    else
	deemphasis = v.toInt();
    FMprocessor->setDeemphasis((int32_t) deemphasis);
}

void RadioInterface::setLowPassFilter(const QString &v) {
    if (v == "None")
	lowPassFilter = -1;
    else
	lowPassFilter = v.toInt();
    FMprocessor->setLFcutoff((int32_t) lowPassFilter);
}

void RadioInterface::setFMFilter(const QString &v) {
    if (v == "None")
	FMfilter = 0.95*fmRate;
    else
	FMfilter = v.toInt()*1000;
    FMprocessor->setBandwidth(FMfilter);
}

void RadioInterface::setFMDegree(int degree) {
    FMprocessor->setBandfilterDegree(degree);
    FMdegree = degree;
}

void RadioInterface::setDevice(int d) {
    if (deviceList[d].device == inputDevice)
	return;

    settings->beginGroup(deviceName);
    if (settingsUi.agcCheckBox->isEnabled()) {
	settings->setValue(DEV_AGC, settingsUi.agcCheckBox->isChecked()? "1": "0");
	settingsDialog->disconnect(settingsUi.agcCheckBox, SIGNAL(stateChanged(int)),
        	inputDevice, SLOT(setAgcControl(int)));
    } else
	settingsUi.agcCheckBox->setEnabled(true);
    if (settingsUi.gainSpinBox->isEnabled()) {
	settings->setValue(DEV_IF_GAIN, settingsUi.gainSpinBox->value());
	settingsDialog->disconnect(settingsUi.gainSpinBox, SIGNAL(valueChanged(int)),
        	inputDevice, SLOT(setIfGain(int)));
    } else
	settingsUi.gainSpinBox->setEnabled(true);
    if (settingsUi.lnaSpinBox->isEnabled()) {
	settings->setValue(DEV_LNA_GAIN, settingsUi.lnaSpinBox->value());
	settingsDialog->disconnect(settingsUi.lnaSpinBox, SIGNAL(valueChanged(int)),
		inputDevice, SLOT(setLnaGain(int)));
    } else
	settingsUi.lnaSpinBox->setEnabled(true);
    settings->endGroup();

    if (playing) {
	if (isFM)
            stopFM();
	else
            stopDAB();
    }

    delete DABprocessor;
    delete FMprocessor;

    inputDevice = deviceList[d].device;
    deviceName = deviceList[d].deviceName;
    deviceUiControls = deviceList[d].controls;
    makeDABprocessor();
    makeFMprocessor();

    // do it all again for the new device
    settings->beginGroup(deviceName);
    if (deviceUiControls & AGC) {
	agc = (settings->value(DEV_AGC, DEV_DEF_AGC).toInt() > 0);
	inputDevice->setAgcControl(agc);
	settingsUi.agcCheckBox->setChecked(agc);
	settingsDialog->connect(settingsUi.agcCheckBox, SIGNAL(stateChanged(int)),
		inputDevice, SLOT(setAgcControl(int)));
    } else
	settingsUi.agcCheckBox->setEnabled(false);
    if (deviceUiControls & IF_GAIN) {
	ifGain = settings->value(DEV_IF_GAIN, DEV_DEF_IF_GAIN).toInt();
	inputDevice->setIfGain(ifGain);
	settingsUi.gainSpinBox->setValue(ifGain);
	settingsDialog->connect(settingsUi.gainSpinBox, SIGNAL(valueChanged(int)),
		inputDevice, SLOT(setIfGain(int)));
    } else
	settingsUi.gainSpinBox->setEnabled(false);
    if (deviceUiControls & LNA_GAIN) {
	lnaGain = settings->value(DEV_LNA_GAIN, DEV_DEF_LNA_GAIN).toInt();
	inputDevice->setLnaGain(lnaGain);
	settingsUi.lnaSpinBox->setValue(lnaGain);
	settingsDialog->connect(settingsUi.lnaSpinBox, SIGNAL(valueChanged(int)),
		inputDevice, SLOT(setLnaGain(int)));
    } else
	settingsUi.lnaSpinBox->setEnabled(false);
    settings->endGroup();
}

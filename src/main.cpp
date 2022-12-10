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
#include <QApplication>
#include <QSettings>
#include <QTranslator>
#include <QString>
#include <QDir>
#include <iostream>
#include "constants.h"
#include "logger.h"
#include "radio.h"

#if CHOOSE_CONFIG == Unix
#define DEFAULT_CFG ".config/" TARGET ".conf"
#elif CHOOSE_CONFIG == Windows
#define DEFAULT_CFG TARGET ".ini"
#else
#define "." DEFAULT_CFG TARGET
#endif

static
void usage(const char *name) {
#ifdef CHOOSE_CONFIG
    fprintf(stderr, "usage: %s [[-i <config file>] [-d <debug level>][-v]|-h|-V]\n", name);
#else
    fprintf(stderr, "usage: %s [-d <debug level>][-v]|-h|-V]\n", name);
#endif
}

int main(int argc, char **argv) {
    QString configFile = QString(DEFAULT_CFG);
    QString locale = QLocale::system().name();
    QTranslator *translator;
    QSettings *settings;
    RadioInterface *radioInterface;
    int opt;
    qint64 mask;

#if IS_WINDOWS
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
    }
#endif
#ifdef CHOOSE_CONFIG
    configFile = QDir::homePath();
    configFile.append("/");
    configFile.append(QString(DEFAULT_CFG));
#endif
    QCoreApplication::setOrganizationName(ORGNAME);
    QCoreApplication::setOrganizationDomain(ORGDOMAIN);
    QCoreApplication::setApplicationName(TARGET);
    QCoreApplication::setApplicationVersion(QString(CURRENT_VERSION));

#ifdef CHOOSE_CONFIG
    while ((opt = getopt(argc, argv, "d:hi:vV")) != -1)
#else
    while ((opt = getopt(argc, argv, "d:hvV")) != -1)
#endif
	switch (opt) {
	case 'd':
	    mask = QString(optarg).toLongLong(NULL, 0);
	    setLogMask(mask);
	    break;
#ifdef CHOOSE_CONFIG
	case 'i':

	    // this is an absolute path or relative to CWD, not relative to the home directory
	    configFile = optarg;
	    break;
#endif
	case 'v':
	    incLogVerbosity();
	    break;
	case 'V':
	    fprintf(stderr, "%s Version %s (%s)\n", argv[0], CURRENT_VERSION, CURRENT_DATE);
	    exit(0);
	case 'h':
	    usage(argv[0]);
	    exit(0);
	default:
	   usage(argv[0]);
	   exit(1);
	}

#if IS_WINDOWS
    settings = new QSettings(configFile, QSettings::IniFormat);
#else
    settings = new QSettings(configFile, QSettings::NativeFormat);
#endif

#if QT_VERSION >= 0x050600
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    if (locale != "en_US" || locale != "en_GB") {
	translator = new QTranslator;

	if (locale == "de_AT" || locale==  "de_CH")
	    locale = "de_DE";
	if (translator->load(QString(":/i18n/") + locale))
	    QCoreApplication::installTranslator(translator);
	else
	    locale = "en_GB";
    }
    QLocale::setDefault(QLocale((const QString&)locale));

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/" TARGET ".ico"));
    radioInterface = new RadioInterface(settings);
    radioInterface->show();
    a.exec();
    fflush(stdout);
    fflush(stderr);
    delete radioInterface;
    delete settings;
    return 0;
}

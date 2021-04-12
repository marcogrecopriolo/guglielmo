TEMPLATE	= app
objectName      = guglielmo
objectVersion	= 0.1
currDate	= $$system(date "+\"%Y-%m-%d %H:%M:%S %z\"")
os		= $$system(uname)
orgName		= SQSL
orgDomain	= sqsl.org
TARGET		= $$objectName
DEFINES		+= TARGET=\\\"$$objectName\\\" CURRENT_VERSION=\\\"$$objectVersion\\\" ORGNAME=\\\"$$orgName\\\" ORGDOMAIN=\\\"$$orgDomain\\\"
DEFINES		+= CURRENT_DATE=\'\"$$currDate\"\'
QT		+= widgets multimedia
CONFIG		-= console
QMAKE_CXXFLAGS	+= -std=c++11
QMAKE_CFLAGS	+= -flto -ffast-math
MAKE_CXXFLAGS	+= -flto -ffast-math
QMAKE_CXXFLAGS  += -isystem $$[QT_INSTALL_HEADERS]
RC_ICONS	= guglielmo.ico
RESOURCES	+= guglielmo.qrc
contains(os, Linux) {
	DEFINES	+= CHOOSE_CONFIG=Linux
}

DEPENDPATH += . \
	      ./src \
	      ./include \
	      ./src/ofdm \
	      ./src/protection \
	      ./src/fm \
	      ./src/rds \
	      ./src/backend \
	      ./src/backend/audio \
	      ./src/backend/data \
	      ./src/backend/data/mot \
	      ./src/backend/data/journaline \
	      ./src/output \
	      ./src/support \
	      ./src/support/viterbi-jan \
	      ./src/support/viterbi-spiral \
	      ./include/fm \
	      ./include/rds \
	      ./include/ofdm \
	      ./include/protection \
	      ./include/backend \
	      ./include/backend/audio \
	      ./include/backend/data \
	      ./include/backend/data/mot \
	      ./include/backend/data/journaline \
	      ./include/output \
	      ./include/support \
	      ./devices 

INCLUDEPATH += . \
	      ./src \
	      ./include \
	      ./include/protection \
	      ./include/dab \
	      ./include/fm \
	      ./include/rds \
	      ./include/ofdm \
	      ./include/backend \
	      ./include/backend/audio \
	      ./include/backend/data \
	      ./include/backend/data/mot \
	      ./include/backend/data/journaline \
	      ./include/output \
	      ./include/support \
	      ./include/support/viterbi-jan \
	      ./include/support/viterbi-spiral \
	      ./devices 

# Input
HEADERS += ./include/radio.h \
	   ./include/constants.h \
	   ./include/settings.h \
	   ./include/country-codes.h \
	   ./include/dab/dab-processor.h \
	   ./include/fm/fm-demodulator.h \
	   ./include/fm/fm-processor.h \
	   ./include/support/squelchClass.h \
	   ./include/rds/rds-blocksynchronizer.h \
	   ./include/rds/rds-decoder.h \
	   ./include/rds/rds-groupdecoder.h \
	   ./include/rds/rds-group.h \
	   ./include/rds/ebu-codetables.h \
	   ./include/ofdm/timesyncer.h \
	   ./include/ofdm/sample-reader.h \
	   ./include/ofdm/ofdm-decoder.h \
	   ./include/ofdm/phasereference.h \
	   ./include/ofdm/phasetable.h \
	   ./include/ofdm/freq-interleaver.h \
	   ./include/ofdm/tii_detector.h \
	   ./include/ofdm/fic-handler.h \
	   ./include/ofdm/fib-decoder.h  \
	   ./include/ofdm/fib-table.h \
	   ./include/ofdm/dab-config.h \
	   ./include/protection/protTables.h \
	   ./include/protection/protection.h \
	   ./include/protection/eep-protection.h \
	   ./include/protection/uep-protection.h \
	   ./include/backend/msc-handler.h \
	   ./include/backend/galois.h \
	   ./include/backend/reed-solomon.h \
	   ./include/backend/rscodec.h \
	   ./include/backend/charsets.h \
	   ./include/backend/firecode-checker.h \
	   ./include/backend/frame-processor.h \
	   ./include/backend/backend.h \
	   ./include/backend/backend-driver.h \
	   ./include/backend/backend-deconvolver.h \
	   ./include/backend/audio/mp2processor.h \
	   ./include/backend/audio/mp4processor.h \
	   ./include/backend/audio/bitWriter.h \
	   ./include/backend/data/data-processor.h \
	   ./include/backend/data/pad-handler.h \
	   ./include/backend/data/virtual-datahandler.h \
	   ./include/backend/data/tdc-datahandler.h \
	   ./include/backend/data/ip-datahandler.h \
	   ./include/backend/data/mot/mot-handler.h \
	   ./include/backend/data/mot/mot-object.h \
	   ./include/backend/data/mot/mot-dir.h \
	   ./include/backend/data/journaline-datahandler.h \
	   ./include/backend/data/journaline/dabdatagroupdecoder.h \
	   ./include/backend/data/journaline/crc_8_16.h \
	   ./include/backend/data/journaline/log.h \
	   ./include/backend/data/journaline/newssvcdec_impl.h \
	   ./include/backend/data/journaline/Splitter.h \
	   ./include/backend/data/journaline/dabdgdec_impl.h \
	   ./include/backend/data/journaline/newsobject.h \
	   ./include/backend/data/journaline/NML.h \
	   ./include/output/audio-base.h \
	   ./include/output/newconverter.h \
	   ./include/output/audiosink.h \
	   ./include/output/Qt-audio.h \
           ./include/output/Qt-audiodevice.h \
	   ./include/support/process-params.h \
	   ./include/support/viterbi-jan/viterbi-handler.h \
	   ./include/support/viterbi-spiral/viterbi-spiral.h \
	   ./include/support/fir-filters.h \
	   ./include/support/fft.h \
	   ./include/support/fft-filters.h \
	   ./include/support/iir-filters.h \
	   ./include/support/oscillator.h \
	   ./include/support/pllC.h \
	   ./include/support/sincos.h \
           ./include/support/fft-handler.h \
	   ./include/support/ringbuffer.h \
	   ./include/support/dab-params.h \
	   ./include/support/band-handler.h \
	   ./include/support/text-mapper.h \
	   ./include/support/Xtan2.h \
	   ./include/support/dab-tables.h \
	   ./include/support/ensemble-printer.h \
	   ./devices/device-handler.h

FORMS	+= ./guglielmo.ui \
	   ./about.ui \
	   ./settings.ui

SOURCES += ./src/main.cpp \
	   ./src/radio.cpp \
	   ./src/dialogs.cpp \
	   ./src/dab/dab-processor.cpp \
	   ./src/fm/fm-demodulator.cpp \
	   ./src/fm/fm-processor.cpp \
	   ./src/rds/rds-blocksynchronizer.cpp \
	   ./src/rds/rds-decoder.cpp \
	   ./src/rds/rds-group.cpp \
	   ./src/rds/rds-groupdecoder.cpp \
	   ./src/ofdm/timesyncer.cpp \
	   ./src/ofdm/sample-reader.cpp \
	   ./src/ofdm/ofdm-decoder.cpp \
	   ./src/ofdm/phasereference.cpp \
	   ./src/ofdm/phasetable.cpp \
	   ./src/ofdm/freq-interleaver.cpp \
	   ./src/ofdm/tii_detector.cpp \
	   ./src/ofdm/fic-handler.cpp \
	   ./src/ofdm/fib-decoder.cpp  \
	   ./src/protection/protTables.cpp \
	   ./src/protection/protection.cpp \
	   ./src/protection/eep-protection.cpp \
	   ./src/protection/uep-protection.cpp \
	   ./src/backend/msc-handler.cpp \
	   ./src/backend/galois.cpp \
	   ./src/backend/reed-solomon.cpp \
	   ./src/backend/rscodec.cpp \
	   ./src/backend/charsets.cpp \
	   ./src/backend/firecode-checker.cpp \
	   ./src/backend/backend.cpp \
           ./src/backend/backend-driver.cpp \
           ./src/backend/backend-deconvolver.cpp \
	   ./src/backend/audio/mp2processor.cpp \
	   ./src/backend/audio/mp4processor.cpp \
	   ./src/backend/audio/bitWriter.cpp \
	   ./src/backend/data/pad-handler.cpp \
	   ./src/backend/data/data-processor.cpp \
	   ./src/backend/data/tdc-datahandler.cpp \
	   ./src/backend/data/ip-datahandler.cpp \
	   ./src/backend/data/mot/mot-handler.cpp \
	   ./src/backend/data/mot/mot-object.cpp \
	   ./src/backend/data/mot/mot-dir.cpp \
	   ./src/backend/data/journaline-datahandler.cpp \
	   ./src/backend/data/journaline/crc_8_16.c \
	   ./src/backend/data/journaline/log.c \
	   ./src/backend/data/journaline/newssvcdec_impl.cpp \
	   ./src/backend/data/journaline/Splitter.cpp \
	   ./src/backend/data/journaline/dabdgdec_impl.c \
	   ./src/backend/data/journaline/newsobject.cpp \
	   ./src/backend/data/journaline/NML.cpp \
	   ./src/output/audio-base.cpp \
	   ./src/output/newconverter.cpp \
	   ./src/output/audiosink.cpp \
           ./src/output/Qt-audio.cpp \
           ./src/output/Qt-audiodevice.cpp \
	   ./src/support/fir-filters.cpp \
	   ./src/support/fft.cpp \
	   ./src/support/fft-filters.cpp \
	   ./src/support/iir-filters.cpp \
	   ./src/support/oscillator.cpp \
	   ./src/support/pllC.cpp \
	   ./src/support/sincos.cpp \
	   ./src/support/viterbi-jan/viterbi-handler.cpp \
	   ./src/support/viterbi-spiral/viterbi-spiral.cpp \
           ./src/support/fft-handler.cpp \
	   ./src/support/dab-params.cpp \
	   ./src/support/band-handler.cpp \
	   ./src/support/text-mapper.cpp \
	   ./src/support/Xtan2.cpp \
	   ./src/support/dab-tables.cpp \
	   ./src/support/ensemble-printer.cpp \
	   ./devices/device-handler.cpp
#
#
unix {
DESTDIR		= ./linux-bin

INCLUDEPATH	+= /usr/local/include
INCLUDEPATH	+= /usr/include/qt5/qwt
LIBS		+= -lfftw3f  -lfftw3 -lusb-1.0 -ldl
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsndfile
LIBS		+= -lsamplerate

# FIXME currently needs to be set by hand
LIBS		+= -lqwt-qt5
#
# comment or uncomment for the devices you want to have support for
# (you obviously have libraries installed for the selected ones)
CONFIG		+= rtlsdr
CONFIG		+= sdrplay
CONFIG		+= airspy
CONFIG		+= hackrf
CONFIG		+= lime
#CONFIG		+= pluto

CONFIG		+= faad
#CONFIG		+= fdk-aac

#if you want to use the default qt way of sound out
CONFIG          += qt-audio

#CONFIG		+= try-epg		# do not use
CONFIG		+= PC
#CONFIG		+= RPI
}

#
# an attempt to have it run under W32 through cross compilation
win32 {

#for 64 bit
#	TARGET		= guglielmo-64-1.0
#	DEFINES		+= __BITS64__
#	DESTDIR		= /usr/shared/sdr-j-development/w64-programs/windows-guglielmo64
#	INCLUDEPATH	+= /usr/x64-w64-mingw32/sys-root/mingw/include
#	LIBS		+= -L/usr/x64-w64-mingw32/sys-root/mingw/lib
#for 32 bit
	TARGET		= guglielmo-32-1.0
	DESTDIR		= /usr/shared/w32-programs/windows-guglielmo32
	INCLUDEPATH	+= /usr/i686-w64-mingw32/sys-root/mingw/include
	LIBS		+= -L/usr/i686-w64-mingw32/sys-root/mingw/lib

#	common:
INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f -lfftw3
LIBS		+= -lportaudio
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -lusb-1.0
LIBS		+= -lz
CONFIG		+= faad

#CONFIG		+= PC
CONFIG		+= NO_SSE
}
#
try-epg	{
	DEFINES		+= TRY_EPG
	QT		+= xml
	DEPENDPATH	+= ./src/backend/data/epg \
	                   ./include/backend/data/epg 
	INCLUDEPATH	+= ./include/backend/data/epg 
	HEADERS		+= ./include/backend/data/epg/epgdec.h 
	SOURCES		+= ./src/backend/data/epg/epgdec.cpp 
}

# for RPI2 use:
RPI	{
	DEFINES		+= __MSC_THREAD__
	DEFINES		+= __THREADED_BACKEND
	DEFINES		+= NEON_AVAILABLE
	QMAKE_CFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

PC	{
#	DEFINES		+= __THREADED_BACKEND
#	DEFINES		+= __MSC_THREAD__
	DEFINES		+= SSE_AVAILABLE
	HEADERS		+= ./src/support/viterbi-spiral/spiral-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-sse.c
}

NO_SSE	{
	HEADERS		+= ./src/support/viterbi-spiral/spiral-no-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-no-sse.c
}

faad	{
	DEFINES		+= __WITH_FAAD__
	HEADERS		+= ./include/backend/audio/faad-decoder.h 
	SOURCES		+= ./src/backend/audio/faad-decoder.cpp 
	LIBS		+= -lfaad
}

fdk-aac	{
	DEFINES		+= __WITH_FDK_AAC__
	INCLUDEPATH	+= ./specials/fdk-aac
	HEADERS		+= ./include/backend/audio/fdk-aac.h 
	SOURCES		+= ./src/backend/audio/fdk-aac.cpp 
	LIBS		+= -lfdk-aac
}

#	devices
#       rtlsdr
rtlsdr {
        DEFINES         += HAVE_RTLSDR
        DEPENDPATH      += ./devices/rtlsdr-handler
        INCLUDEPATH     += ./devices/rtlsdr-handler
        HEADERS         += ./devices/rtlsdr-handler/rtlsdr-handler.h
        SOURCES         += ./devices/rtlsdr-handler/rtlsdr-handler.cpp
}
#
#       the SDRplay
#
sdrplay {
        DEFINES         += HAVE_SDRPLAY
        DEPENDPATH      += ./devices/sdrplay-handler
        INCLUDEPATH     += ./devices/sdrplay-handler
        HEADERS         += ./devices/sdrplay-handler/sdrplay-handler.h
        SOURCES         += ./devices/sdrplay-handler/sdrplay-handler.cpp
}
#
# airspy support
#
airspy {
	DEFINES		+= HAVE_AIRSPY
	DEPENDPATH	+= ./devices/airspy 
	INCLUDEPATH	+= ./devices/airspy-handler \
	                   ./devices/airspy-handler/libairspy
	HEADERS		+= ./devices/airspy-handler/airspy-handler.h \
	                   ./devices/airspy-handler/airspyfilter.h \
	                   ./devices/airspy-handler/libairspy/airspy.h
	SOURCES		+= ./devices/airspy-handler/airspy-handler.cpp \
	                   ./devices/airspy-handler/airspyfilter.cpp
}
#
#       the HACKRF One
#
hackrf {
        DEFINES         += HAVE_HACKRF
        DEPENDPATH      += ./devices/hackrf-handler
        INCLUDEPATH     += ./devices/hackrf-handler
        HEADERS         += ./devices/hackrf-handler/hackrf-handler.h
        SOURCES         += ./devices/hackrf-handler/hackrf-handler.cpp
}
#
#       the Lime SDR
#
lime {
        DEFINES         += HAVE_LIME
        DEPENDPATH      += ./devices/lime-handler
        INCLUDEPATH     += ./devices/lime-handler
        HEADERS         += ./devices/lime-handler/lime-handler.h \
	                   ./devices/lime-handler/LimeSuite.h \
	                   ./devices/lime-handler/LMS7002M_parameters.h
        SOURCES         += ./devices/lime-handler/lime-handler.cpp
}
#
pluto   {
        DEFINES         += HAVE_PLUTO
        QT              += network
        INCLUDEPATH     += ./devices/pluto-handler
        HEADERS         += ./devices/pluto-handler/dabFilter.h
        HEADERS         += ./devices/pluto-handler/pluto-handler.h
        SOURCES         += ./devices/pluto-handler/pluto-handler.cpp
	LIBS            += -liio  -lad9361
}

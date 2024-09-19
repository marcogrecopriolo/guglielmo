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
 *    Taken from qt-dab, with bug fixes and enhancements.
 *
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef	DEVICE_HANDLER_H
#define	DEVICE_HANDLER_H

#include <stdint.h>
#include "constants.h"

#if IS_WINDOWS
#define GETPROCADDRESS GetProcAddress
#define CLOSE_LIBRARY FreeLibrary
#else
#define GETPROCADDRESS dlsym
#define CLOSE_LIBRARY dlclose
#endif

struct agcStats {
    int min;
    int max;
    int overflows;
};

#define DEV_SHORT 32
#define DEV_LONG 64
#define MAX_DEVICES 6

struct deviceStrings {
    char name[DEV_SHORT];
    char id[DEV_SHORT];
    char description[DEV_LONG];
};

class deviceHandler: public QThread {
    public:
    deviceHandler(void);
    virtual ~deviceHandler(void) {}

    virtual int	devices(deviceStrings *devs, int max) { (void) devs; (void) max; return 0; }

    // we use an id rather than device list position (as used by RTLSDR / SDRPLAY)
    // because the V3 SDRPLAY API only reports devices that are available, so list
    // positions are unreliable: for one, the device that we are currently using
    // will skew the list!
    virtual bool setDevice(const char *) { return true; }
    virtual bool restartReader(int32_t) { return true; }
    virtual void stopReader(void) {}
    virtual int32_t getSamples(std::complex<float> *v, int32_t amount, agcStats *stats) {
	(void) v;
	(void) stats;
	return amount;
    }
    virtual int32_t Samples(void) { return 0; }
    virtual void resetBuffer(void) {}
    virtual int16_t bitDepth(void) { return 10; }
    virtual int32_t amplitude(void);
    virtual int32_t getRate(void) { return 192000; }
    virtual void getIfRange(int32_t *min, int32_t *max) { *min = 0, *max = GAIN_SCALE - 1; }
    virtual void getLnaRange(int32_t *min, int32_t *max) { *min = 0, *max = 0; }
    virtual void setIfGain(int) {}
    virtual void setLnaGain(int) {}
    virtual void setAgcControl(int) {}

    protected:
    int32_t vfoFrequency;
};
#endif

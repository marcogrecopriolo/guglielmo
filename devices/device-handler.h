#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dabMini program
 *
 *    dabMini is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dabMini is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dabMini; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	We have to create a simple virtual class here, since we
 *	want the interface with different devices (including  filehandling)
 *	to be transparent
 */
#ifndef	__DEVICE_HANDLER__
#define	__DEVICE_HANDLER__

#include	<stdint.h>
#include	"constants.h"
#include	<QThread>

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

class	deviceHandler: public QThread {
public:
			deviceHandler 	(void);
virtual			~deviceHandler 	(void);
virtual		int32_t	deviceCount	(void);
virtual		QString	deviceName	(int32_t);
virtual		void	deviceModel	(int32_t, char *, int32_t);
virtual		bool	setDevice	(int32_t);
virtual		bool	restartReader	(int32_t);
virtual		void	stopReader	(void);
virtual		int32_t	getSamples	(std::complex<float> *, int32_t, agcStats *);
virtual		int32_t	Samples		(void);
virtual		void	resetBuffer	(void);
virtual		int16_t	bitDepth	(void) { return 10; }
virtual		int32_t amplitude	(void);
virtual		int32_t getRate         (void);
virtual         void getIfRange      (int32_t *min, int32_t *max) { *min = 0, *max = GAIN_SCALE - 1; }
virtual         void getLnaRange     (int32_t *min, int32_t *max) { *min = 0, *max = 0; }
virtual		void	setIfGain	(int);
virtual		void	setLnaGain	(int);
virtual		void	setAgcControl	(int);

//
protected:
		int32_t	vfoFrequency;
};
#endif

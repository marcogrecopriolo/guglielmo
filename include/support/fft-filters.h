#
/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef	__FFT_FILTER
#define	__FFT_FILTER

#include	"constants.h"
#include	"fft.h"

class	fftFilter {
public:
			fftFilter	(int32_t, int16_t);
			~fftFilter	(void);
	void		setBand		(int32_t, int32_t, int32_t);
	void		setSimple	(int32_t, int32_t, int32_t);
	void		setLowPass	(int32_t, int32_t);
	DSPCOMPLEX	Pass		(DSPCOMPLEX);
	DSPFLOAT	Pass		(DSPFLOAT);

private:
	int32_t		fftSize;
	int16_t		filterDegree;
	int16_t		OverlapSize;
	int16_t		NumofSamples;
	common_fft	*MyFFT;
	DSPCOMPLEX	*FFT_A;
	common_ifft	*MyIFFT;
	DSPCOMPLEX	*FFT_C;
	common_fft	*FilterFFT;
	DSPCOMPLEX	*filterVector;
	DSPFLOAT	*RfilterVector;
	DSPCOMPLEX	*Overloop;
	int32_t		inp;
};

#endif


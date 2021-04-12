#
/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sdr-j-fm
 *
 *    sdr-j-fm is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    sdr-j-fm is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdr-j-fm if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"iir-filters.h"
#ifndef	__MINGW32__
#ifdef	__FREEBSD__
#include	<stdlib.h>
#else
#include	"alloca.h"
#endif
#endif

#define	MAXORDER	0176

static
DSPFLOAT	sinhm1 (DSPFLOAT x) {
	return log (x + sqrt (x * x + 1));
}

static
DSPFLOAT	coshm1 (DSPFLOAT x) {
	return log (x + sqrt (x * x - 1));
}

int16_t	guessOrderB (int16_t apass, int16_t astop, int32_t wpass, int32_t wstop) {
DSPFLOAT	tmp;
DSPFLOAT nominator, denom;

	nominator	= (pow (10.0, -0.1 * astop) - 1);
	denom		= (pow (10.0, -0.1 * apass) - 1);
	nominator	= log (nominator / denom);
	tmp		= nominator / (2 * log ((DSPFLOAT)wstop / wpass)); 
/*
 */
	return	(int)(tmp + 0.5);
}

int	guessOrderC (int apass, int astop, int wpass, int wstop) {
DSPFLOAT	tmp;
DSPFLOAT nominator, denom;

	nominator	= (pow (10.0, -0.1 * astop) - 1);
	denom		= (pow (10.0, -0.1 * apass) - 1);
	nominator	= sqrt (nominator / denom);
	tmp		= coshm1 (nominator) / coshm1 ((DSPFLOAT) wstop / wpass);
/*
 */
	return	((int)(tmp + 0.5) + 1);
}

int	guessOrderIC (int apass, int astop, int wpass, int wstop) {
	return guessOrderC (apass, astop, wpass, wstop);
}

static
DSPFLOAT	Bilineair (element *Transfer, int fs, int NumofQuad) {
element	*CurrentQuad;
DSPFLOAT	f2, f4;	     /* do not make them int's, larger fs will overflow	*/
DSPFLOAT	N0, N1, N2;
DSPFLOAT	D0, D1, D2;
int	i;
DSPFLOAT	gain	= 1.0;

	f2	=  2 * fs;
	f4	= f2 * f2;
	for (i = 0; i < NumofQuad; i ++) {
	   CurrentQuad = &Transfer [i];

	   N0 = CurrentQuad -> A0 * f4 + CurrentQuad -> A1 * f2 +
	                                 CurrentQuad -> A2;
	   N1 = 2 * (CurrentQuad -> A2 - CurrentQuad -> A0 * f4);
	   N2 = CurrentQuad -> A0 * f4 - CurrentQuad -> A1 * f2 +
	                                 CurrentQuad -> A2;

	   D0 = CurrentQuad -> B0 * f4 + CurrentQuad -> B1 * f2 + 
	                                 CurrentQuad -> B2;
	   D1 = 2 * (CurrentQuad -> B2 - CurrentQuad -> B0 * f4);
	   D2 = CurrentQuad -> B0 * f4 - CurrentQuad -> B1 * f2 +
	                                 CurrentQuad -> B2;

	   CurrentQuad	-> A0	= 1.0;
	   CurrentQuad	-> A1	= N1 / N0;
	   CurrentQuad	-> A2	= N2 / N0;
	   CurrentQuad	-> B0	= 1.0;
	   CurrentQuad	-> B1	= D1 / D0;
	   CurrentQuad	-> B2	= D2 / D0;
	   gain			*= (N0 / D0);
	}

	return gain;
}


DSPFLOAT	warpDtoA (int fd, int fs) {
	return 2.0 * fs * tan ((2 * M_PI * fd) / (2 * fs));
}

int	warpAtoD (DSPFLOAT omega, int fs) {
        return (int)(2.0 * fs / tan (omega / (2 * fs)));
}

DSPFLOAT	newButterworth (element *biQuads,
	                        int numofQuads, int order, int apass) {
element	*currentQuad;
int16_t	i;
DSPFLOAT	Phim;
DSPFLOAT	sigma, omega;
DSPFLOAT	R;
DSPFLOAT	Eps	= sqrt (pow (10.0, -0.1 * apass) - 1);
	R = 1.0 / pow (Eps, 1.0 / order);
	if (order & 01) {
	   currentQuad	= &biQuads [0];
	   currentQuad	-> A0	= 0;
	   currentQuad	-> A1	= 0;
	   currentQuad	-> A2	= R;
	   currentQuad	-> B0	= 0;
	   currentQuad	-> B1	= 1;
	   currentQuad	-> B2	= R;
	   
	   for (i = 1; i < numofQuads; i ++) {
	      currentQuad = &biQuads [i];

	      Phim = (M_PI * (2 * (i - 1) + order + 1) / (2 * order));
	      sigma  = R * cos (Phim);
	      omega  = R * sin (Phim);

	      currentQuad -> A0 = 0;
	      currentQuad -> A1 = 0;
	      currentQuad -> A2 = (sigma * sigma + omega * omega);
	      currentQuad -> B0 = 1;
	      currentQuad -> B1 = -2 * sigma;
	      currentQuad -> B2 = (sigma * sigma + omega * omega);
	   }
	}
	else
	for (i = 0; i < numofQuads; i ++) {
	   currentQuad = &biQuads [i];

	   Phim = (M_PI * (2 * i + order + 1) / (2 * order));
	   sigma  = R * cos (Phim);
	   omega  = R * sin (Phim);

	   currentQuad -> A0 = 0;
	   currentQuad -> A1 = 0;
	   currentQuad -> A2 = (sigma * sigma + omega * omega);
	   currentQuad -> B0 = 1;
	   currentQuad -> B1 = -2 * sigma;
	   currentQuad -> B2 = (sigma * sigma + omega * omega);
	}
	return 1.0;
}

DSPFLOAT	newChebyshev (element *biQuads,
	              int numofQuads, int order, int apass) {
int16_t	i;
DSPFLOAT	sinhD, coshD;
DSPFLOAT	Phim;
DSPFLOAT	sigma, omega;
element	*currentQuad;
DSPFLOAT	D;
DSPFLOAT	Eps	= sqrt (pow (10.0, -0.1 * apass) - 1);

	D = sinhm1 (1.0 / Eps) / order;
	sinhD	= sinh (D);
	coshD	= cosh (D);
	if (order & 01) {
	   currentQuad	= &biQuads [0];
	   currentQuad	-> A0	= 0;
	   currentQuad	-> A1	= 0;
	   currentQuad	-> A2	= sinhD;
	   currentQuad	-> B0	= 0;
	   currentQuad	-> B1	= 1;
	   currentQuad	-> B2	= sinhD;
	   for (i = 1; i < numofQuads; i ++) {
	      currentQuad = &biQuads [i];

	      Phim = (M_PI * (2 * (i - 1) + 1) / (2 * order));
	      sigma  = - sinhD * sin (Phim);
	      omega  =   coshD * cos (Phim);

	      currentQuad -> A0 = 0;
	      currentQuad -> A1 = 0;
	      currentQuad -> A2 = (sigma * sigma + omega * omega);
	      currentQuad -> B0 = 1;
	      currentQuad -> B1 = -2 * sigma;
	      currentQuad -> B2 = (sigma * sigma + omega * omega);
	   }
	}
	else
	for (i = 0; i < numofQuads; i ++) {
	   currentQuad = &biQuads [i];

	   Phim = (M_PI * (2 * i + 1) / (2 * order));
	   sigma  = - sinhD * sin (Phim);
	   omega  =   coshD * cos (Phim);

	   currentQuad -> A0 = 0;
	   currentQuad -> A1 = 0;
	   currentQuad -> A2 = (sigma * sigma + omega * omega);
	   currentQuad -> B0 = 1;
	   currentQuad -> B1 = -2 * sigma;
	   currentQuad -> B2 = (sigma * sigma + omega * omega);
	}

	if ((order & 01) == 0)		// even
	   return pow (10.0, 0.05 * apass);
	else
	   return 1.0;
}

DSPFLOAT	newChebyshevI (element *biQuads,
	               int numofQuads, int order, int apass) {
int	i;
element	*currentQuad;
DSPFLOAT	Phim;
DSPFLOAT	sigma, omega, sigmaP, omegaP, sigmazm, omegazm;
DSPFLOAT	D;
DSPFLOAT	A1, A2, B1, B2;
int	astop;
DSPFLOAT	Eps;
DSPFLOAT	sinhD, coshD;
	(void)apass;
	astop	= -90;		/* db		*/
	Eps	= 1.0 / sqrt (pow (10.0, -0.1 * astop) - 1);
	D = sinhm1 (1.0 / Eps) / order;

	sinhD	= sinh (D);
	coshD	= cosh (D);
	if (order & 01) {
	   currentQuad	= &biQuads [0];
	   currentQuad	-> A0	= 0;
	   currentQuad	-> A1	= 0;
	   currentQuad	-> A2	= 1.0 / sinhD;
	   currentQuad	-> B0	= 0;
	   currentQuad	-> B1	= 1;
	   currentQuad	-> B2	= 1.0 / sinhD;
	   for (i = 1; i < numofQuads; i ++) {
	      currentQuad = &biQuads [i];

	      Phim = (M_PI * (2 * (i - 1) + 1) / (2 * order));
	      sigmaP = -sinhD * sin (Phim);
	      omegaP =  coshD * cos (Phim);

	      sigma  = sigmaP / (sigmaP * sigmaP + omegaP * omegaP);
	      omega  = - omegaP / (sigmaP * sigmaP + omegaP * omegaP);

	      sigmazm = 0.0;
	      omegazm = 1.0 / cos (Phim);

	      A1	= -2 * sigmazm;
	      A2	= sigmazm * sigmazm + omegazm * omegazm;

	      B1	= -2 * sigma;
	      B2	= sigma * sigma + omega * omega;

	      currentQuad -> A0	= 1.0;
	      currentQuad -> B0	= A2 / B2;

	      currentQuad -> A1	= A1;
	      currentQuad -> B1	= B1 * A2 / B2;

	      currentQuad -> A2	= A2;
	      currentQuad -> B2	= B2 * A2 / B2;
	   }
	}
	else
	for (i = 0; i < numofQuads; i ++) {
	   currentQuad = &biQuads [i];

	   Phim = (M_PI * (2 * i + 1) / (2 * order));
	   sigmaP = -sinh (D) * sin (Phim);
	   omegaP =  cosh (D) * cos (Phim);

	   sigma  = sigmaP / (sigmaP * sigmaP + omegaP * omegaP);
	   omega  = - omegaP / (sigmaP * sigmaP + omegaP * omegaP);

	   sigmazm = 0.0;
	   omegazm = 1.0 / cos (Phim);

	   A1	= -2 * sigmazm;
	   A2	= sigmazm * sigmazm + omegazm * omegazm;

	   B1	= -2 * sigma;
	   B2	= sigma * sigma + omega * omega;

	   currentQuad -> A0	= 1.0;
	   currentQuad -> B0	= A2 / B2;

	   currentQuad -> A1	= A1;
	   currentQuad -> B1	= B1 * A2 / B2;

	   currentQuad -> A2	= A2;
	   currentQuad -> B2	= B2 * A2 / B2;
	}
	return 1.0;
}

DSPFLOAT	newNormalized (element *Quads,
	                       int32_t numofQuads,
	                       int32_t order, int32_t apass, int8_t fType) {
/*
 */
	if (order <= 0)	/* never trust a caller		*/
	   order = 6;

	if (fType == S_CHEBYSHEV) 
	   return newChebyshev (Quads, numofQuads, order, apass);
	else
	if (fType == S_BUTTERWORTH) 
	   return newButterworth (Quads, numofQuads, order, apass);
	else
	if (fType == S_INV_CHEBYSHEV)
	   return newChebyshevI (Quads, numofQuads, order, apass);
	else
	   return newButterworth (Quads, numofQuads, order, apass);
}
/*
 *	Just an old square root formula, now in the DSPCOMPLEX domain
 */
void	cQuadratic (DSPCOMPLEX A, DSPCOMPLEX B, DSPCOMPLEX C,
	                       DSPCOMPLEX *D, DSPCOMPLEX *E) {
DSPCOMPLEX	temp	= sqrt (B * B - cmul (A * C, 4.0));

	*D	= ( -B + temp) / cmul (A, 2.0);
	*E	= ( -B - temp) / cmul (A, 2.0);
}
/*
 *	Unnormalize for the BandPass
 */
DSPFLOAT	unnormalizeBP (element  *temp, element *Transfer,
	                   DSPFLOAT Wo, DSPFLOAT BW, int numofbiQuads) {
DSPCOMPLEX A, B, C, D, E;
int	i;

	for (i = 0; i < numofbiQuads; i ++) {
/*
 *	for numerators with a zero A0 and A1 term
 */
	   if (temp [i]. A0 == 0.0) {
	      Transfer [2 * i]. A0 = 0.0;
	      Transfer [2 * i]. A1 = sqrt (temp [i]. A2) * BW;
	      Transfer [2 * i]. A2 = 0.0;
	      Transfer [2 * i + 1]. A0 = 0.0;
	      Transfer [2 * i + 1]. A1 = sqrt (temp [i]. A2) * BW;
	      Transfer [2 * i + 1]. A2 = 0.0;
	   }
	   else {
	    // make coefficients into DSPCOMPLEX numbers
	      A = DSPCOMPLEX (temp [i]. A0, 0.0);
	      B = DSPCOMPLEX (temp [i]. A1, 0.0);
	      C = DSPCOMPLEX (temp [i]. A2, 0.0);
	      cQuadratic (A, B, C, &D, &E);
	    // make required substitutions, then factor again
	      A = DSPCOMPLEX (1.0, 0.0);
	      B = cmul (-D, BW);
	      C = DSPCOMPLEX (Wo * Wo, 0.0);
	      cQuadratic (A, B, C, &D, &E);
	    // final values for coefficients
	      Transfer [2 * i]. A0 = 1.0;
	      Transfer [2 * i]. A1 = -2.0 * real (D);
	      Transfer [2 * i]. A2 = real (D * conj (D));
	      Transfer [2 * i + 1]. A0 = 1.0;
	      Transfer [2 * i + 1]. A1 = -2.0 * real (E);
	      Transfer [2 * i + 1]. A2 = real (E * conj (E));
	   }
	/*
	 *  now for the denominator (which always has A0 # 0)
	 */
	   A = DSPCOMPLEX (temp [i]. B0, 0.0);
	   B = DSPCOMPLEX (temp [i]. B1, 0.0);
	   C = DSPCOMPLEX (temp [i]. B2, 0.0);
	   cQuadratic (A, B, C, &D, &E);
	 // make required substitutions, then factor again
	   A = DSPCOMPLEX (1.0, 0.0);
	   B = cmul (-D, BW);
	   C = DSPCOMPLEX (Wo * Wo, 0);
	   cQuadratic (A, B, C, &D, &E);
	 // final values for coefficients
	   Transfer [2 * i]. B0 = 1.0;
	   Transfer [2 * i]. B1 = -2.0 * real (D);
	   Transfer [2 * i]. B2 = real (D * conj (D));
	   Transfer [2 * i + 1]. B0 = 1.0;
	   Transfer [2 * i + 1]. B1 = -2.0 * real (E);
	   Transfer [2 * i + 1]. B2 = real (E * conj (E));
	}

	return 1.0;
}

	LowPassIIR::LowPassIIR (int16_t apass, int16_t astop,
	                        int32_t fpass, int32_t fstop, int32_t fs):
	                           Basic_IIR ((guessOrderB (apass, astop,
	                                                   fpass, fstop) + 1) / 2) {  
int16_t		order	= guessOrderB (apass, astop, fpass, fstop);
DSPFLOAT	omega;
element	*currentQuad;
int16_t	i;

/*
 *	just to be on the safe side, check the frequency
 *	before warping (omage in radials)
 */
	if (2 * fpass >= fs)	
	   fpass = fs / 4;	/* answer stupidity with stupidity	*/
	omega		= warpDtoA (fpass, fs);

	gain		= newNormalized	(Quads,
	                                 numofQuads, order, apass, S_BUTTERWORTH);
//
//	unnormalize. For a lowpass filter, this is simple, we
//	replace the S by (S / omega). After some shuffling a
//	quad becomes
//	(omega ^ 2) / (S ^ 2 + omega * Q * S + omega ^2)

	for (i = 0; i < numofQuads; i ++) {
	   currentQuad = &Quads [i];

//	   currentQuad -> A0 = currentQuad -> A0;
//	   currentQuad -> B0 = currentQuad -> B0;

	   currentQuad -> A1 = currentQuad -> A1 * omega;
	   currentQuad -> B1 = currentQuad -> B1 * omega;

	   currentQuad -> A2 = currentQuad -> A2 * omega * omega;
	   currentQuad -> B2 = currentQuad -> B2 * omega * omega;
	}

	gain		*= Bilineair		(Quads, fs, numofQuads);
}

	LowPassIIR::LowPassIIR (int16_t order,
	                        int32_t fpass, int32_t fs,
	                        int16_t ftype):
	                           Basic_IIR (((order + 1) & MAXORDER) / 2) {
int	apass	= -1;	/* db attenuation in the pass band	*/
DSPFLOAT	omega;
element	*currentQuad;
int16_t	i;

/*
 *	just to be on the safe side, check the frequency
 *	before warping (omage in radials)
 */
	if (2 * fpass >= fs)	
	   fpass = fs / 4;	/* answer stupidity with stupidity	*/
	omega		= warpDtoA (fpass, fs);

	gain		= newNormalized	(Quads,
	                                 numofQuads, order, apass, ftype);
//
//	unnormalize. For a lowpass filter, this is simple, we
//	replace the S by (S / omega). After some shuffling a
//	quad becomes
//	(omega ^ 2) / (S ^ 2 + omega * Q * S + omega ^2)

	for (i = 0; i < numofQuads; i ++) {
	   currentQuad = &Quads [i];

//	   currentQuad -> A0 = currentQuad -> A0;
//	   currentQuad -> B0 = currentQuad -> B0;

	   currentQuad -> A1 = currentQuad -> A1 * omega;
	   currentQuad -> B1 = currentQuad -> B1 * omega;

	   currentQuad -> A2 = currentQuad -> A2 * omega * omega;
	   currentQuad -> B2 = currentQuad -> B2 * omega * omega;
	}

	gain		*= Bilineair		(Quads, fs, numofQuads);
}

	LowPassIIR::~LowPassIIR () {
}

//======================================================================

	HighPassIIR::HighPassIIR (int16_t order,
	                          int32_t fpass, int32_t fs,
	                          int16_t ftype):
	                            Basic_IIR (((order + 1) & MAXORDER) / 2){
int32_t	apass	= - 1;		/* passband attenuation in db's	*/
DSPFLOAT	omega;
int16_t	i;
element		*currentQuad;
/*
 *	just to be on the safe side, check fpass and warp (omega in radials)
 */
	if (2 * fpass >= fs)
	   fpass = fs / 4;
	omega	= warpDtoA (fpass, fs);

	gain	= newNormalized (Quads, numofQuads, order, apass, ftype);
//
//	For highpass filters, unnormalization implies
//	substitution S -> (omega / s). After some shuffling
//	we end up with
//
	for (i = 0; i < numofQuads; i ++) {
	   DSPFLOAT	A0, A1, A2, B0, B1, B2;
	   currentQuad = &Quads [i];
	   A0		= currentQuad	-> A0;
	   A1		= currentQuad	-> A1;
	   A2		= currentQuad	-> A2;
	   B0		= currentQuad	-> B0;
	   B1		= currentQuad	-> B1;
	   B2		= currentQuad	-> B2;
	   
	   gain	*= A2 / B2;
	   currentQuad -> A0 = 1.0;
	   currentQuad -> B0 = 1.0;

	   currentQuad -> A1 = (A1 / A2) * omega;
	   currentQuad -> B1 = (B1 / B2) * omega;

	   currentQuad -> A2 = (A0 / A2) * omega * omega;
	   currentQuad -> B2 = (B0 / B2) * omega * omega;
	}

	gain	*= Bilineair		(Quads, fs, numofQuads);
}

	HighPassIIR::~HighPassIIR () {
}

//=======================================================================
//
//	handling numofQuads in a bandpass context
//	is somewhat strange, we end up with
//	a double portion of quads, so we allocate them in
//	the beginning

BandPassIIR::BandPassIIR (int16_t order,
	                  int32_t flow, int32_t fhigh, int32_t fs,
	                  int16_t ftype):Basic_IIR ((order + 1) & MAXORDER) {
DSPFLOAT	omegaL, omegaH;

	order		= (order + 1) & MAXORDER;
element	*temp		= (element *) alloca (numofQuads * sizeof (element));
DSPFLOAT	Wo, BW;
int	apass	= -1;	/* attenuation 1 db	*/
DSPCOMPLEX A, B, C, D, E;
/*
 *	be aware of incorrect parameters
 */
	if (flow >= fs / 2)
           flow = (int)(0.2 * fs);
	if (fhigh >= fs / 2)
           fhigh = (int)(0.3 * fs);

	omegaL	= warpDtoA (flow, fs);
	omegaH	= warpDtoA (fhigh, fs);
	Wo	= sqrt (omegaL * omegaH);
	BW	= omegaH - omegaL;

	gain	= newNormalized (temp, numofQuads / 2, order, apass, ftype);
//
//	Unnormalizing for the bandpass case implies substitution 
//	S -> (s ^ 2 + W0 ^ 2) / (BW * s)
//	Notice that substituting this, we end up with fourth order terms,
//	complicating things.
//	We follow the approach taken by Les Thede, i.e. if we start with
//	a quadratic term, we determine the roots, then we car write
//	the quadratic term s^2 + a. s + b as (s + x1) * (s + x2), where
//	x1 and x2 generally will be DSPCOMPLEX
//	Substitution is in these first order terms. Substitution will yield
//	quadratic forms with DSPCOMPLEX coefficients.
//	We again can be decomposed into DSPCOMPLEX conjugate pairs that
//	can be combined into two quadratics with real coefficients.

	gain	*= unnormalizeBP (temp, Quads, Wo, BW, numofQuads / 2);
/*
 *	Careful: from now on we have the full numofQuads biQuads
 */
	gain	*= Bilineair (Quads, fs, numofQuads);
}

	BandPassIIR::~BandPassIIR () {
}


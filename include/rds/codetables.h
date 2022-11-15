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

#ifndef __CODETABLES_H__
#define __CODETABLES_H__

uint16_t tabG0[] = {
/*		0x00	0x01	0x02	0x03	0x04	0x05	0x06	0x07	0x08	0x09	0x0A	0x0B	0x0C	0x0D	0x0E	0x0F	*/
/* 0x00 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x01 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x02 */	' ',	'!',	'"',	'#',	0xA4,	'%',	'&',	'\'',	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 0x03 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 0x04 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	0x2015,	0x5F,
/* 0x06 */	0x2551,	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	0x0AF,	' ',
/* 0x08 */	0xE1,	0xE0,	0xE9,	0xE8,	0xED,	0xEC,	0xF3,	0xF5,	0xFA,	0xF9,	0xD1,	0xC7,	0x015E,	0x03B2,	0xA1,	0x0132,
/* 0x09 */	0xE2,	0xE4,	0xEA,	0xEB,	0xEE,	0xEF,	0xF4,	0xF6,	0xFB,	0xFC,	0xF1,	0xE7,	0x015F,	0x011F,	0x0131,	0x0133,
/* 0x0A */	0xAA,	0x03B1,	0xA9,	0x2030,	0x011E,	0x011B,	0x0148,	0x0151,	0x03C0,	0x20AC,	0xA3,	0x24,	0x2190,	0x2191,	0x2192,	0x2193,
/* 0x0B */	0xBA,	0xB9,	0xB2,	0xB3,	0xB1,	0x0130,	0x0144,	0x0171,	0xB5,	0xBF,	0xF7,	0xB0,	0xBC,	0xBD,	0xBE,	0xA7,
/* 0x0C */	0xC1,	0xC0,	0xC9,	0xC8,	0xCD,	0xCC,	0xD3,	0xD2,	0xDA,	0xD9,	0x0158,	0x010C,	0x0160,	0x017D,	0x0110,	0x013F,
/* 0x0D */	0xC2,	0xC4,	0xCA,	0xCB,	0xCE,	0xCF,	0xD4,	0xD6,	0xDB,	0xDC,	0x0159,	0x010D,	0x0161,	0x017E,	0x0111,	0x0140,
/* 0x0E */	0xC3,	0xC5,	0xC6,	0x0152,	0x0176,	0xDD,	0xD5,	0xD8,	0xDE,	0x014A,	0x0154,	0x0106,	0x015A,	0x0179,	0x0166,	0xF0,
/* 0x0F */	0xE3,	0xE5,	0xE6,	0x0153,	0x0175,	0xFD,	0xF5,	0xF8,	0xFE,	0x014B,	0x0155,	0x0107,	0x015B,	0x017A,	0x0167,	' '
};

uint16_t tabG1[] = {
/*		0x00	0x01	0x02	0x03	0x04	0x05	0x06	0x07	0x08	0x09	0x0A	0x0B	0x0C	0x0D	0x0E	0x0F	*/
/* 0x00 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x01 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x02 */	' ',	'!',	'"',	'#',	0xA4,	'%',	'&',	'\'',	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 0x03 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 0x04 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	'X',	'X',
/* 0x06 */	0x2551,	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	0xAF,	' ',
/* 0x08 */	0xE1,	0xE0,	0xE9,	0xE8,	0xED,	0xEC,	0xF3,	0xF5,	0xFA,	0xF9,	0xD1,	0xC7,	0x015E,	0x03B2,	0xA1,	0x0132,
/* 0x09 */	0xE2,	0xE4,	0xEA,	0xEB,	0xEE,	0xEF,	0xF4,	0xF6,	0xFB,	0xFC,	0xF1,	0xE7,	0x015F,	0x011F,	0x0131,	0x0133,
/* 0x0A */	0xAA,	0x0140,	0xA9,	0x2030,	0x0103,	0x11B,	0x0148,	0x0151,	0x0165,	0x20AC,	0xA3,	0x24,	0x2190,	0x2191,	0x2192,	0x2193,
/* 0x0B */	0xBA,	0xB9,	0xB2,	0xB3,	0xB1,	0x0130,	0x0144,	0x0171,	0x0163,	0xBF,	0xF7,	0xB0,	0xBC,	0xBD,	0xBE,	0xA7,
				/* FIXME */
/* 0x0C */	0x0404,	0x042F,	'b',	0x0420,	0x0414,	0x042D,	0x0424,	0x0403,	0x042A,	0x0418,	0x0416,	0x040C,	0x041B,	0x0402,	0x0452,	0x042B,
				/* FIXME */
/* 0x0D */	0x045E,	0x0409,	'd',	0x0428,	0x0426,	0x042E,	0x0429,	0x040A,	0x040F,	0x040D,	0x0417,	0x010D,	0x0161,	0x017E,	0x0111,	0x0107,
										/* FIXME */
/* 0x0E */	0x03A0,	0x03B1,	0x03B0,	0x03C8,	0x03B4,	0x03B5,	0x03C6,	0x03B3,	0x03B3,	0x03B9,	0x03A3,	0x03C7,	0x03BB,	0x03BC,	0x03C5,	0x03C9,
/* 0x0F */	0x03C0,	0x03A9,	0x03C1,	0x03C3,	0x03C4,	0x03BE,	0x0398,	0x0393,	0x039E,	0x03C5,	0x03B6,	0x03C2,	0x039B,	0x03A8,	0x0394,	' '
};

uint16_t tabG2[] = {
/*		0x00	0x01	0x02	0x03	0x04	0x05	0x06	0x07	0x08	0x09	0x0A	0x0B	0x0C	0x0D	0x0E	0x0F	*/
/* 0x00 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x01 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x02 */	' ',	'!',	'"',	'#',	0xA4,	'%',	'&',	'\'',	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 0x03 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 0x04 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	'X',	'X',
/* 0x06 */	0x2551,	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	0xAF,	' ',
/* 0x08 */	0x068A,	0x068C,	'X',	0x068E,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',
/* 0x09 */	0x0638,	'X',	'X',	'X',	'X',	'X',	0x06A9,	'X',	0x0632,	0x06BE,	0x0788,	0x068D,	0x015F,	0x11F,	0x131,	0x0133,
/* 0x0A */	0x05D0,	0x05D1,	0x05DB,	0x05D3,	0x05D4,	0x05D5,	0x05D6,	0x05D7,	0x05E2,	0x05D9,	0x05E0,	0x05DA,	0x05DC,	0x05E4,	0x05DD,	0x05D1,
/* 0x0B */	0x05D5,	0x05DD,	0x05E6,	0x05DE,	0x05E3,	0x05E2,	0x05E6,	0x05E7,	0x05E8,	0x05E9,	0x05EA,	0xB0,	0xBC,	0xBD,	0xBE,	0xA7,

				/* FIXME */
/* 0x0C */	0x0404,	0x042F,	'b',	0x0420,	0x0414,	0x042D,	0x0424,	0x0403,	0x042A,	0x0418,	0x0416,	0x040C,	0x041B,	0x0402,	0x0452,	0x042B,
				/* FIXME */
/* 0x0D */	0x045E,	0x0409,	'd',	0x0428,	0x0426,	0x042E,	0x0429,	0x040A,	0x040F,	0x040D,	0x0417,	0x010D,	0x0161,	0x017E,	0x0111,	0x107,
										/* FIXME */
/* 0x0E */	0x03A0,	0x03B1,	0x03B0,	0x03C8,	0x03B4,	0x03B5,	0x03C6,	0x03B3,	0x3B3,	0x03B9,	0x03A3,	0x03C7,	0x03BB,	0x03BC,	0x03C5,	0x03C9,
/* 0x0F */	0x03C0,	0x03A9,	0x03C1,	0x03C3,	0x03C4,	0x03BE,	0x0398,	0x0393,	0x39E,	0x03C5,	0x03B6,	0x03C2,	0x039B,	0x03A8,	0x0394,	' '
};

#endif

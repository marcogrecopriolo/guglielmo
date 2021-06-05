/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopiolo@gmail.com>
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
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	'X',	'X',
/* 0x06 */	'|',	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	'X',	' ',
/* 0x08 */	0xE1,	0xE0,	0xE9,	0xE8,	0xED,	0xEC,	0xF3,	0xF5,	0xFA,	0xF9,	0xD1,	0xC7,	0x015E,	0x3B2,	'X',	0x0132,
/* 0x09 */	0xE2,	0xE4,	0xEA,	0xEB,	0xEE,	0xEF,	0xF4,	0xF6,	0xFB,	0xFC,	0xF1,	0xE7,	0x015F,	0x11D,	'X',	0x0133,
/* 0x0A */	'X',	0x3B1,	0xA9,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xA3,	0x24,	0x2190,	0x2191,	0x2192,	0x2193,
/* 0x0B */	'X',	0xB9,	0xB2,	0xB3,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xBC,	0xBD,	0xBE,	'X',
/* 0x0C */	0xC1,	0xC0,	0xC9,	0xC8,	0xCD,	0xCC,	0xD3,	0xD2,	0xDA,	0xD9,	0x0158,	0x010C,	0x160,	0x017D,	'X',	'X',
/* 0x0D */	0xC2,	0xC4,	0xCA,	0xCB,	0xCE,	0xCF,	0xD4,	0xD6,	0xDB,	0xDC,	0x0159,	0x010D,	0x0161,	0x017E,	'X',	'X',
/* 0x0E */	0xC3,	0xC5,	0xC6,	0x0152,	0x0176,	0xDD,	0xD5,	0xD8,	'X',	'X',	0x0154,	0x0106,	0x015A,	0x0179,	'X',	'X',
/* 0x0F */	0xE3,	0xE5,	0xE6,	0x0153,	0x0175,	0xFD,	0xF5,	0xF8,	'X',	'X',	0x0155,	0x0107,	0x015B,	0x017A,	'X',	'X'
};

uint16_t tabG1[] = {
/*		0x00	0x01	0x02	0x03	0x04	0x05	0x06	0x07	0x08	0x09	0x0A	0x0B	0x0C	0x0D	0x0E	0x0F	*/
/* 0x00 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x01 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x02 */	' ',	'!',	'"',	'#',	0xA4,	'%',	'&',	'\'',	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 0x03 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 0x04 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	'X',	'X',
/* 0x06 */	'|',	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	'X',	' ',
/* 0x08 */	0xE1,	0xE0,	0xE9,	0xE8,	0xED,	0xEC,	0xF3,	0xF5,	0xFA,	0xF9,	0xD1,	0xC7,	0x015E,	0x3B2,	'X',	0x0132,
/* 0x09 */	0xE2,	0xE4,	0xEA,	0xEB,	0xEE,	0xEF,	0xF4,	0xF6,	0xFB,	0xFC,	0xF1,	0xE7,	0x015F,	0x11D,	'X',	0x0133,
/* 0x0A */	'X',	0x3B1,	0xA9,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xA3,	0x24,	0x2190,	0x2191,	0x2192,	0x2193,
/* 0x0B */	'X',	0xB9,	0xB2,	0xB3,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xBC,	0xBD,	0xBE,	'X',
/* 0x0C */	0xC1,	0xC0,	0xC9,	0xC8,	0xCD,	0xCC,	0xD3,	0xD2,	0xDA,	0xD9,	0x0158,	0x010C,	0x160,	0x017D,	'X',	'X',
/* 0x0D */	0xC2,	0xC4,	0xCA,	0xCB,	0xCE,	0xCF,	0xD4,	0xD6,	0xDB,	0xDC,	0x0159,	0x010D,	0x0161,	0x017E,	'X',	'X',
/* 0x0E */	0xC3,	0xC5,	0xC6,	0x0152,	0x0176,	0xDD,	0xD5,	0xD8,	'X',	'X',	0x0154,	0x0106,	0x015A,	0x0179,	'X',	'X',
/* 0x0F */	0xE3,	0xE5,	0xE6,	0x0153,	0x0175,	0xFD,	0xF5,	0xF8,	'X',	'X',	0x0155,	0x0107,	0x015B,	0x017A,	'X',	'X'
};

uint16_t tabG2[] = {
/*		0x00	0x01	0x02	0x03	0x04	0x05	0x06	0x07	0x08	0x09	0x0A	0x0B	0x0C	0x0D	0x0E	0x0F	*/
/* 0x00 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x01 */	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',	' ',
/* 0x02 */	' ',	'!',	'"',	'#',	0xA4,	'%',	'&',	'\'',	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/* 0x03 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 0x04 */	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x05 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',	'[',	'\\',	']',	'X',	'X',
/* 0x06 */	'|',	'a',	'b',	'c',	'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x07 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',	'x',	'y',	'z',	'{',	'|',	'}',	'X',	' ',
/* 0x08 */	0xE1,	0xE0,	0xE9,	0xE8,	0xED,	0xEC,	0xF3,	0xF5,	0xFA,	0xF9,	0xD1,	0xC7,	0x015E,	0x3B2,	'X',	0x0132,
/* 0x09 */	0xE2,	0xE4,	0xEA,	0xEB,	0xEE,	0xEF,	0xF4,	0xF6,	0xFB,	0xFC,	0xF1,	0xE7,	0x015F,	0x11D,	'X',	0x0133,
/* 0x0A */	'X',	0x3B1,	0xA9,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xA3,	0x24,	0x2190,	0x2191,	0x2192,	0x2193,
/* 0x0B */	'X',	0xB9,	0xB2,	0xB3,	'X',	'X',	'X',	'X',	'X',	'X',	'X',	'X',	0xBC,	0xBD,	0xBE,	'X',
/* 0x0C */	0xC1,	0xC0,	0xC9,	0xC8,	0xCD,	0xCC,	0xD3,	0xD2,	0xDA,	0xD9,	0x0158,	0x010C,	0x160,	0x017D,	'X',	'X',
/* 0x0D */	0xC2,	0xC4,	0xCA,	0xCB,	0xCE,	0xCF,	0xD4,	0xD6,	0xDB,	0xDC,	0x0159,	0x010D,	0x0161,	0x017E,	'X',	'X',
/* 0x0E */	0xC3,	0xC5,	0xC6,	0x0152,	0x0176,	0xDD,	0xD5,	0xD8,	'X',	'X',	0x0154,	0x0106,	0x015A,	0x0179,	'X',	'X',
/* 0x0F */	0xE3,	0xE5,	0xE6,	0x0153,	0x0175,	0xFD,	0xF5,	0xF8,	'X',	'X',	0x0155,	0x0107,	0x015B,	0x017A,	'X',	'X'
};

#endif

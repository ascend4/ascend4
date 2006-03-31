/*
 *  Ascend Character Types
 *  Shortcut character recognition
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: actype.c,v $
 *  Date last modified: $Date: 1997/07/18 12:27:43 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#include<limits.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include "actype.h"

#ifndef lint
static CONST char AcTypeId[] = "$Id: actype.c,v 1.4 1997/07/18 12:27:43 mthomas Exp $";
#endif

CONST unsigned char ascend_char_t[UCHAR_MAX+1] = {
  0,				/*   0 nul */
  0,				/*   1 soh */
  0,				/*   2 stx */
  0,				/*   3 etx */
  0,				/*   4 eot */
  0,				/*   5 enq */
  0,				/*   6 ack */
  0,				/*   7 bel */
  0,				/*  10 bs */
  9,				/*  11 ht */
  9,				/*  12 nl */
  0,				/*  13 vt */
  9,				/*  14 np */
  9,				/*  15 cr */
  0,				/*  16 so */
  0,				/*  17 si */
  0,				/*  20 dle */
  0,				/*  21 dc1 */
  0,				/*  22 dc2 */
  0,				/*  23 dc3 */
  0,				/*  24 dc4 */
  0,				/*  25 nak */
  0,				/*  26 syn */
  0,				/*  27 etb */
  0,				/*  30 can */
  0,				/*  31 em  */
  0,				/*  32 sub */
  0,				/*  33 esc */
  0,				/*  34 fs  */
  0,				/*  35 gs  */
  0,				/*  36 rs  */
  0,				/*  37 us  */
  9,				/*  40 sp  */
  0,				/*  41 ! */
  0,				/*  42 " */
  0,				/*  43 # */
  0,				/*  44 $ */
  0,				/*  45 % */
  0,				/*  46 & */
  0,				/*  47 ' */
  1,				/*  50 ( */
  1,				/*  51 ) */
  1,				/*  52 * */
  0,				/*  53 + */
  0,				/*  54 , */
  1,				/*  55 - */
  1,				/*  56 . */
  1,				/*  57 / */
  7,				/*  60 0 */
  7,				/*  61 1 */
  7,				/*  62 2 */
  7,				/*  63 3 */
  7,				/*  64 4 */
  7,				/*  65 5 */
  7,				/*  66 6 */
  7,				/*  67 7 */
  7,				/*  70 8 */
  7,				/*  71 9 */
  0,				/*  72 : */
  0,				/*  73 ; */
  0,				/*  74 < */
  0,				/*  75 = */
  0,				/*  76 > */
  1,				/*  77 ? */
  0,				/* 100 @ */
  19,				/* 101 A */
  19,				/* 102 B */
  19,				/* 103 C */
  19,				/* 104 D */
  19,				/* 105 E */
  19,				/* 106 F */
  19,				/* 107 G */
  19,				/* 110 H */
  19,				/* 111 I */
  19,				/* 112 J */
  19,				/* 113 K */
  19,				/* 114 L */
  19,				/* 115 M */
  19,				/* 116 N */
  19,				/* 117 O */
  19,				/* 120 P */
  19,				/* 121 Q */
  19,				/* 122 R */
  19,				/* 123 S */
  19,				/* 124 T */
  19,				/* 125 U */
  19,				/* 126 V */
  19,				/* 127 W */
  19,				/* 130 X */
  19,				/* 131 Y */
  19,				/* 132 Z */
  0,				/* 133 [ */
  0,				/* 134 \ */
  0,				/* 135 ] */
  1,				/* 136 ^ */
  3,				/* 137 _ */
  0,				/* 140 ` */
  19,				/* 141 a */
  19,				/* 142 b */
  19,				/* 143 c */
  19,				/* 144 d */
  19,				/* 145 e */
  19,				/* 146 f */
  19,				/* 147 g */
  19,				/* 150 h */
  19,				/* 151 i */
  19,				/* 152 j */
  19,				/* 153 k */
  19,				/* 154 l */
  19,				/* 155 m */
  19,				/* 156 n */
  19,				/* 157 o */
  19,				/* 160 p */
  19,				/* 161 q */
  19,				/* 162 r */
  19,				/* 163 s */
  19,				/* 164 t */
  19,				/* 165 u */
  19,				/* 166 v */
  19,				/* 167 w */
  19,				/* 170 x */
  19,				/* 171 y */
  19,				/* 172 z */
  0,				/* 173 { */
  0,				/* 174 | */
  0,				/* 175 } */
  0,				/* 176 ~ */
  0,				/* 177 del */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

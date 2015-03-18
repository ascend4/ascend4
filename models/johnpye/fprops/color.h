/*	ASCEND modelling environment
	Copyright (C) 2012 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//* @file
	Platform-independent console colour output.
*/
#ifndef ASC_COLOR_H
#define ASC_COLOR_H

#include <stdio.h>

#ifdef __WIN32__
enum ConsoleColor{
	ASC_FG_BLUE = 1
	,ASC_FG_GREEN = 2
	,ASC_FG_RED = 4
	,ASC_FG_BRIGHT = 8
	,ASC_BG_BLUE = 16
	,ASC_BG_GREEN = 32
	,ASC_BG_RED = 64
	,ASC_BG_BRIGHT = 128
};
#else
enum ConsoleColor{
	ASC_FG_RED = 1
	,ASC_FG_GREEN = 2
	,ASC_FG_BLUE = 4
	,ASC_FG_BRIGHT = 8
	,ASC_BG_RED = 16
	,ASC_BG_GREEN = 32
	,ASC_BG_BLUE = 64
	,ASC_BG_BRIGHT = 128
};
#endif

#define ASC_FG_BLACK (0)
#define ASC_FG_DARKGREY (ASC_FG_BRIGHT)
#define ASC_FG_BROWN (ASC_FG_RED|ASC_FG_GREEN)
#define ASC_FG_MAGENTA (ASC_FG_RED|ASC_FG_BLUE)
#define ASC_FG_CYAN (ASC_FG_BLUE|ASC_FG_GREEN)
#define ASC_FG_YELLOW (ASC_FG_BROWN|ASC_FG_BRIGHT)
#define ASC_FG_BRIGHTCYAN (ASC_FG_CYAN|ASC_FG_BRIGHT)
#define ASC_FG_BRIGHTRED (ASC_FG_RED|ASC_FG_BRIGHT)
#define ASC_FG_BRIGHTGREEN (ASC_FG_GREEN|ASC_FG_BRIGHT)
#define ASC_FG_BRIGHTBLUE (ASC_FG_BLUE|ASC_FG_BRIGHT)
#define ASC_FG_PINK (ASC_FG_MAGENTA|ASC_FG_BRIGHT)
#define ASC_FG_GREY (ASC_FG_RED|ASC_FG_GREEN|ASC_FG_BLUE)
#define ASC_FG_WHITE (ASC_FG_GREY|ASC_FG_BRIGHT)

#define ASC_BG_YELLOW (ASC_BG_RED|ASC_BG_GREEN|ASC_BG_BRIGHT)
#define ASC_BG_WHITE (ASC_BG_RED|ASC_BG_GREEN|ASC_BG_BLUE|ASC_BG_BRIGHT)
#define ASC_BG_GREY (ASC_BG_RED|ASC_BG_GREEN|ASC_BG_BLUE)
#define ASC_BG_BLACK (0)
#define ASC_BG_DARKGREY (ASC_BG_BRIGHT)

int color_on(FILE *f, int colorcode);

int color_off(FILE *f);

#define FPROPS_MSG(FMT, ...) \
	{color_on(stderr,ASC_FG_BROWN);\
	fprintf(stderr,"%s:%d",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"(%s):",__func__);\
	color_off(stderr);\
	fprintf(stderr," " FMT "\n",##__VA_ARGS__);}

#define FPROPS_ERRMSG(STR,...) \
	{color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s:%d(%s):", __FILE__, __LINE__ ,__func__); \
	color_off(stderr);\
	fprintf(stderr," " STR "\n", ##__VA_ARGS__);}

#endif /* ASC_COLOR_H */

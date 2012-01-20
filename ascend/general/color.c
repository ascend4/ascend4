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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//* @file
	Platform-independent console colour output.
*/

#include "color.h"
#include "config.h"

#ifdef __WIN32__
# include <windows.h>
# include <conio.h>
#endif

#ifdef ASC_XTERM_COLORS
static int color_test(){
	static int use_xterm_color = 0;
	char *term;
	if(!use_xterm_color){
		term = getenv("TERM");

# ifdef __WIN32__
		if(term!=NULL){
			if(strcmp(term,"cygwin")==0){
				Asc_FPrintf(stderr,"\n\n\nUsing Windows color codes\n\n\n");
				use_xterm_color=1;
			}
		}
# else
		if(term!=NULL){
			if(strcmp(term,"msys")==0 || strcmp(term,"xterm")==0){
				/* MSYS (rxvt), putty, xterm. */
				use_xterm_color=1;
			}else{
				/* unrecognised $TERM */
				use_xterm_color=-1;
			}
		}else{
			/* no $TERM var */
			use_xterm_color=-1;
		}
# endif
	}
	return use_xterm_color;
}


# ifndef __WIN32__

/* ANSI color codes from integer bitfield input... */

int color_on(FILE *f, int colorcode){
	int use_color = color_test();

	if(use_color){
		int fg = colorcode & (ASC_FG_GREY);
		int bg = (colorcode & (ASC_BG_GREY)) >> 4;
		int bold = colorcode & ASC_FG_BRIGHT;
		fprintf(f, "\033[3%d%sm",fg,(bold?";1":""));
		if(bg){
			fprintf(f, "\0334%dm",bg);
		}
	}
	return 0;
}

int color_off(FILE *f){
	int use_color = color_test();

	if(use_color){
		fprintf(f, "\033[0m");
	}
	return 0;
}

# else

/* Windows color codes... */

int color_on(FILE *f, int colorcode){
	int use_color = color_test();
	HANDLE Hnd;
	if(use_color==1){
	   Hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	   if(Hnd == INVALID_HANDLE_VALUE)return 0;
	   SetConsoleTextAttribute(Hnd, (WORD)colorcode);
	}
	return 0;
}

int color_off(FILE *f){
	return color_on(f,ASC_FG_GREY);
}

# endif

#else
/* if ASC_XTERM_COLORS is turned off, we're not using any colors ever */
int color_on(FILE *f, int colorcode){return 0;}
int color_off(FILE *f){return 0;}
#endif /*ASC_XTERM_COLORS*/

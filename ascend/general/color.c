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

#include "color.h"
#include "config.h"

#ifdef __WIN32__
# include <windows.h>
# include <conio.h>
#endif

#ifdef ASC_XTERM_COLORS
static int color_test(){
	/* 1: use xterm; 2: use windows; -1: no color */
	static int use_xterm_color = 0;
	char *term;
	if(!use_xterm_color){
		term = getenv("TERM");

# ifdef __WIN32__
		if(term!=NULL){
			if(strcmp(term,"cygwin")==0){
				Asc_FPrintf(stderr,"\n\n\nUsing Windows color codes\n\n\n");
				use_xterm_color=2;
			}else if(strcmp(term,"xterm")==0){
				Asc_FPrintf(stderr,"\n\n\nUsing xterm color codes\n\n\n");
				use_xterm_color=1;
			}
		}
# else
		if(term!=NULL){
			if(strcmp(term,"xterm")==0 || strcmp(term,"xterm-256color")==0){
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


/* ANSI color codes from integer bitfield input... */

int color_on(FILE *f, int colorcode){
	int use_color = color_test();
	int fg, bg, bold;
# ifdef __WIN32__
	HANDLE Hnd;
# endif

	switch(use_color){
	case 1:
		fg = colorcode & (ASC_FG_GREY);
		bg = (colorcode & (ASC_BG_GREY)) >> 4;
		bold = colorcode & ASC_FG_BRIGHT;
# ifdef __WIN32__
		/* on this platform, we have to switch red and blue bits for both fg and bg */
		fg = fg - (fg & 5) + ((1 & fg)<<2) + ((fg & 4)>>2);
		bg = bg - (bg & 5) + ((1 & bg)<<2) + ((bg & 4)>>2);
# endif
		fprintf(f, "\033[3%d%sm",fg,(bold?";1":""));
		if(bg){
			fprintf(f, "\0334%dm",bg);
		}
		break;
# ifdef __WIN32__
	case 2:
	   Hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	   if(Hnd == INVALID_HANDLE_VALUE)return 0;
	   SetConsoleTextAttribute(Hnd, (WORD)colorcode);
	   break;
#endif
	}
	return 0;
}

int color_off(FILE *f){
	int use_color = color_test();
# ifdef __WIN32__
	HANDLE Hnd;
# endif

	switch(use_color){
	case 1:
		fprintf(f, "\033[0m");
		break;
# ifdef __WIN32__
	case 2:
		Hnd = GetStdHandle(STD_OUTPUT_HANDLE);
		if(Hnd == INVALID_HANDLE_VALUE)return 0;
		SetConsoleTextAttribute(Hnd, (WORD)ASC_FG_GREY);
		break;
# endif
	}
	return 0;
}

#else
/* if ASC_XTERM_COLORS is turned off, we're not using any colors ever */
int color_on(FILE *f, int colorcode){return 0;}
int color_off(FILE *f){return 0;}
#endif /*ASC_XTERM_COLORS*/

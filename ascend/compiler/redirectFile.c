/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*/

#include <stdio.h>

#include <ascend/general/platform.h>
#include "redirectFile.h"

#ifdef REIMPLEMENT_STREAMS
FILE *g_ascend_errors=NULL;
FILE *g_ascend_warnings=NULL;
FILE *g_ascend_information=NULL;

void Asc_RedirectCompilerDefault() {
  g_ascend_errors = stderr;         /* error file */
  g_ascend_warnings = stderr;       /* whine file */
  g_ascend_information = stderr;    /* note file */
}

void Asc_RedirectCompilerStreams(FILE *errfile, FILE *warnfile, FILE *infofile)
{
  g_ascend_errors = errfile;         /* error file */
  g_ascend_warnings = warnfile;      /* whine file */
  g_ascend_information = infofile;   /* note file */
}
#endif

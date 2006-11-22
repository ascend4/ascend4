/*	ASCEND modelling environment
	Copyright (C) 1999 Benjamin A Allan
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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	File redirection for ascend.

	This module defines the fundamental constants used by the rest of
	Ascend and pulls in system headers.
	There is not corresponding compiler.c. The variables
	declared in this header are defined in ascParse.y.

	This header and tcl/tk headers are known to conflict. This header
	should be included AFTER tcl.h or tk.h, not before.

	Requires:
	#include "utilities/ascConfig.h"
*//*
	by Ben Allan
	Version: $Revision: 1.1 $
	Version control file: $RCSfile: redirectFile.h,v $
	Date last modified: $Date: 2000/01/25 02:26:22 $
	Last modified by: $Author: ballan $

	ChangeLog
	10/13/2005  Added Asc_RedirectCompilerStreams()  (J.D. St.Clair)
*/

#ifndef ASC_REDIRECTFILE_H
#define ASC_REDIRECTFILE_H

#include <utilities/ascConfig.h>

#ifdef REIMPLEMENT_STREAMS
ASC_DLLSPEC(FILE *) g_ascend_errors;       /**< File for error messages.  Default is stderr. */
ASC_DLLSPEC(FILE *) g_ascend_warnings;     /**< File for warning messages.  Default is stderr. */
ASC_DLLSPEC(FILE *) g_ascend_information;  /**< File for informational messages.  Default is stderr. */

ASC_DLLSPEC(void) Asc_RedirectCompilerDefault(void);
/**< Set the default files/streams to receive ASCEND messages. */

ASC_DLLSPEC(void ) Asc_RedirectCompilerStreams(FILE *errfile,
                                        FILE *warnfile,
                                        FILE *infofile);
/**< Set specific files/streams to receive ASCEND messages. */

#endif /* REIMPLEMENT_STREAMS */

#endif /* ASC_REDIRECTFILE_H*/


/*	FRAME3DD: Static and dynamic structural analysis of 2D & 3D frames and trusses
	Copyright (C) 2007-2008 John Pye

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Error reporting functions for use with parser (little-used at the moment)
*/
#ifndef ERROR_H
#define ERROR_H

#include "types.h"
#include "print.h"

/* global variables */

extern print *errorGlobalPrint;

/* macros */

#define errorReport(args)        errorReportLocationWith(1,__FILE__, __LINE__,args)
#define errorReportWith(ex,args) errorReportLocationWith(ex,__FILE__, __LINE__,args)
#define errorReportLocationWith(ex,file,line,args) \
  ( errorInitPrint() \
  , printFormat( errorGlobalPrint, ("error in %s, line %d: ", file, line) ) \
  , printFormat( errorGlobalPrint, args ) \
  , exit( ex ) \
  )

#define errorReportExt(args)        errorReportExtWith(1,args)
#define errorReportExtWith(ex,args) \
  ( errorInitPrint() \
  , printFormat( errorGlobalPrint, ("error: ") ) \
  , printFormat( errorGlobalPrint, args ) \
  , exit( ex ) \
  )

/* functions */

void  errorInitPrint( void );
void  errorSetPrint( print *p, cbool dispose );
print *errorGetPrint( void );

#endif /* ERROR_H */


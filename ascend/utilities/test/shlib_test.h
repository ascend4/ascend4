/* ASCEND modelling environment
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

#ifndef TEST_ASCDYNALOAD_SHLIB_H
#define TEST_ASCDYNALOAD_SHLIB_H

#include <ascend/general/platform.h>

typedef int valuetype;
typedef int (*initFunc)(void);
typedef int (*isInitializedFunc)(void);
typedef void (*cleanupFunc)(void);

/** A public datum. */
ASC_DLLSPEC int value;

/** Initializes the library. Returns -5. */
ASC_DLLSPEC int init(void);
/** Returns TRUE if library has been initialized, FALSE otherwise. */
ASC_DLLSPEC int isInitialized(void);
/** Cleans up the library. */
ASC_DLLSPEC void cleanup(void);

#endif  /* TEST_ASCDYNALOAD_SHLIB_H */

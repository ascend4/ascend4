/*
 *  Utility functions for Ascend
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: old_utils.h,v $
 *  Date last modified: $Date: 1998/01/29 01:04:09 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Programming System.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph James Zaher
 *  Copyright (C) 1993, 1994 Benjamin Andrew Allan, Joseph James Zaher
 *
 *  The Ascend Programming System is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Ascend is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  THE UTILITIES IN THIS HEADER ARE DEPRECATED.
 *  <pre>
 *  This module defines the dimensionality checking and some other
 *  auxillaries for Ascend.
 *
 *  This file is called old_utils because these utilities are outdated and
 *  need to go away.  DO NOT MAKE ANY MORE REFERENCES TO THESE FUNCTIONS!
 *
 *  Contents:     ASCEND Utilities module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                03/94 - Re-wrote name making functions which no longer
 *                        need pre-allocated string space as an argument
 *                        to write to.  Added an additional function which
 *                        creates instance names utilizing the shortest
 *                        path.  The dimension string output function also
 *                        no longer requires pre-allocation of a string.
 *                04/94 - Provided a relation dimension checker.
 *
 *  Description:  This module provides supplementary functions which may
 *                prove useful by any client of the ASCEND system.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "compiler/instance_enum.h"
 *                #include "compiler/fractions.h"
 *                #include "compiler/dimen.h"
 *                #include "compiler/relation_type.h"
 *  </pre>
 */

#ifndef _OLD_UTILS_H
#define _OLD_UTILS_H

/**
 * functions that are soon to go away are surrounded with
 * #if (NOLONGERSUPPORTED == 0). These functions should not be used.
 */
#define NOLONGERSUPPORTED 1

extern char *asc_make_dimensions(CONST dim_type *dim);
/**<
 *  <!--  dimens = asc_make_dimensions(dim)                            -->
 *  <!--  char *dimens;                                                -->
 *  <!--  dim_type *dim;                                               -->
 *
 *  Prints the dimensions to a sufficiently long string which
 *  is created and returned.  The string should be destroyed when
 *  no longer in use.
 *
 * @deprecated No longer supported.
 */

extern int g_check_dimensions_noisy;
/**<                                                  
 *  If 0, warnings are suppressed. If 1, warnings are given
 *  from asc_check_dimensions().
 *
 * @deprecated No longer supported.
 */

extern int asc_check_dimensions(CONST struct relation *rel, dim_type *dimens);
/**<
 *  <!--  valid = asc_check_dimensions(rel,dimens);                    -->
 *  <!--  int valid;                                                   -->
 *  <!--  struct relation *rel;                                        -->
 *  <!--  dim_type *dimens;                                            -->
 *
 *  Scans a relation in postfix and collects all dimensional
 *  information by applying each token.  It returns a value of TRUE
 *  only if no real instances or real atom instances with wild
 *  dimensionality and no dimensional inconsistencies were encountered.
 *  If the return value is 2 rather than 1, then the dimensionality
 *  has been determined before.
 *  The address of an allocated dimension type is passed in so that
 *  the dimensions of the relation (or at least what the function
 *  thinks the dimensions ought to be) can be also obtained.<br><br>
 *
 *
 *  THIS ONLY WORKS ON e_token relations and later for e_opcode
 *  relations. rel is assumed to be valid when called. !!!
 * @deprecated No longer supported.
 */

#endif  /* _OLD_UTILS_H */


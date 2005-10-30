/*
 *  Hash function
 *  by Tom Epperly
 *  10/24/89
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: hashpjw.c,v $
 *  Date last modified: $Date: 1997/07/18 11:38:36 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 */

#include <stdio.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "general/hashpjw.h"

#ifndef lint
static CONST char HashpjwID[] = "$Id: hashpjw.c,v 1.1 1997/07/18 11:38:36 mthomas Exp $";
#endif

unsigned long hashpjw(register CONST char *str,
                      register unsigned long int size)
{
  register CONST char *p;
  register unsigned long h=0,g;

  asc_assert((NULL != str) && (size > 0));

  for(p = str; *p != '\0'; p++) {
    h = (h << 4) + (*p);
    if (0 != (g = h&0xf0000000)) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % size;
}

/*
 * This is a temporary integer hashing function.
 * It is relatively expensive, as we first do int to str
 * transformation. This needs to be fixed with a proper
 * integer hashing function.
 */
unsigned long hashpjw_int(int id,
                          register unsigned long int size)
{
  char tmp[64];

  (void)snprintf(tmp, 64, "%d", id);
  return hashpjw(tmp, size);
}




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
#include "utilities/ascConfig.h"
#include "general/hashpjw.h"

#ifndef lint
static CONST char HashpjwID[] = "$Id: hashpjw.c,v 1.1 1997/07/18 11:38:36 mthomas Exp $";
#endif

unsigned long hashpjw(register CONST char *str,
		      register unsigned long int size)
{
  register CONST char *p;
  register unsigned long h=0,g;
  for(p = str; *p != '\0'; p++) {
    h = (h << 4) + (*p);
    if ((g = h&0xf0000000)) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % size;
}

/*
 * This is a temporary integer hashing function.
 * It is relatively expensive, as we first do str to int
 * transformation. This needs to be fixed with a proper
 * integer hashing function.
 */
unsigned long hashpjw_int(int id,
			  register unsigned long int size)
{
  char tmp[64], *p;
  register unsigned long h=0,g;

  sprintf(tmp,"%d",id);
  for(p = tmp; *p != '\0'; p++) {
    h = (h << 4) + (*p);
    if ((g = h&0xf0000000)) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % size;
}




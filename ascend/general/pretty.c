/*
 *  pretty.c
 *  I/O Formatting Functions
 *  by Ben Allan
 *  Created: 01/98
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: pretty.c,v $
 *  Date last modified: $Date: 2001/01/28 03:39:36 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.  COPYING is found in ../compiler.
 */

#include <ctype.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "pretty.h"

static int outstring(FILE *fp, char *s, int indent)
{
  int loop = indent;
  while(loop>0) {
    FPUTC(' ',(fp));
    loop--;
  }
  return (indent + FPRINTF(fp,"%s\n",s));
}

/*
 * remember not to change the content of the string, in the net.
 */
int print_long_string(FILE *fp, char *string, int width, int indent)
{
  char oldchar;
  char *head, *tail, *lastspace;
  int lw,count;

  /* there's an off by 1 in the logic, so ... */
  width++;
  if (fp == NULL || string == NULL) {
    return 0;
  }
  if (width < 4 || indent < 0) {
    return FPRINTF(fp,"%s",string);
  }
  lw = 0; count = 0;
  head = string;
  lastspace = NULL;

  while (*head != '\0') {
    tail = head;
    while (*tail != '\0' && lw < width) {
      if (isspace(*tail)) {
        lastspace = tail;
      }
      lw++;
      tail++;
    }
    if (lw < width) { /* found END of line */
      return (count + outstring(fp,head,indent));
    }
    if (lastspace == NULL) { /* drat, line overrun */
      /* find next space or end */
      while (*tail != '\0' && !isspace(*tail)) {
        lw++;
        tail++;
      }
      if (*tail != '\0') { /* found a break */
        oldchar = *tail;
        *tail = '\0';
        count += outstring(fp,head,indent);
        *tail = oldchar;
        head = tail;
        lw = 0;
      } else { /* found  END */
        return (count + outstring(fp,head,indent));
      }
    } else { /*  found break */
      oldchar = *lastspace;
      lastspace[0] = '\0';
      count += outstring(fp,head,indent);
      *lastspace = oldchar;
      head = lastspace;
      lastspace = NULL;
      lw = 0;
    }
    head++;
  }
  return count;
}

/* Computes whether the string s starts with */
/*EOL*/
/* or not. Does not change the string content. */
static int isEOL(char *s) {
  if (s[0] == '/' &&
      s[1] == '*' &&
      s[2] == 'E' &&
      s[3] == 'O' &&
      s[4] == 'L' &&
      s[5] == '*' &&
      s[6] == '/') {
    return 1;
  }
  return 0;
}

/*
 * remember not to change the content of the string, in the net.
 */
int print_long_string_EOL(FILE *fp,char *string, int indent)
{
  char *head, *tail;
  int count = 0;

  if (fp == NULL || string == NULL) {
    return 0;
  }
  if (indent < 0) {
    FPRINTF(fp,"%s",string);
  }
  head = string;

  while (*head != '\0') {
    tail = head;
    while (*tail != '\0') {
      if (isEOL(tail)) {
        tail[0] = '\0';
        count += outstring(fp, head, indent);
        tail[0] = '/';
        tail += 6;
        head = tail;
        break;
      } else {
        tail++;
      }
    }
    if (*tail == '\0') {
      /* end of string with leftovers */
      count += outstring(fp, head, indent);
      break;
    }
    head++;
  }
  return count;
}

#ifdef PLSTEST
int main()
{
  int w = 12;
  char *a, *b;
  a = (char *)malloc(80);
  b = (char *)malloc(80);
  sprintf(b,"%s","12345/*EOL*/345 67890123	45 678 90\n1234 5678901/*EOL*/ 9012");
  sprintf(a,"%s","123456789012345 67890123	45 678 90\n1234 56789012345678 9012");

  fprintf(stderr,">a>>%s<<<\n\n",a);

  for (;w>=4;w--) {
    fprintf(stderr,"\n#fmt %d\n",w);
    print_long_string(stderr,a,w,4);
  }

  fprintf(stderr,">b>>%s<<<\n",b);
  fprintf(stderr,">>>%s<<<\n",b);
  fprintf(stderr,"\n#fmt EOL\n");
  print_long_string_EOL(stderr,b,4);
  fprintf(stderr,">>><<<\n");

  free(a); free(b);
  return 0;
}
#endif /* test */

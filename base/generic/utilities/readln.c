/*
 *  Read line module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: readln.c,v $
 *  Date last modified: $Date: 1997/07/18 12:04:24 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is in ../compiler.
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/readln.h"

#define	BUFLEN	1000

static int killnl(s,nl)
char *s;
int *nl;
/**
 ***  Kills newline, returning the new line length.  *nl is set to
 ***  TRUE if there was a newline.
 **/
{
   register char *end;

   end = s + strlen(s);
   if( (*nl = (end != s && end[-1] == '\n')) ) {
     *--end = '\0';
   }
   return(end-s);
}

static char *strsave(s)
char *s;
/**
 ***  Allocates just enough memory to store s and copies it to
 ***  that location.  Returns pointer to allocated memory, or NULL
 ***  if ascmalloc() fails.
 **/
{
   char *sav;

   if( (sav=(char *)ascmalloc(strlen(s)+1)) != NULL )
      strcpy(sav,s);
   return(sav);
}

int readln(s,mx)
char *s;
int mx;
{
   return( freadln(s,mx,stdin) );
}

int freadln(s,mx,fp)
char *s;
int mx;
FILE *fp;
{
   int nl;
   int len;

   if( fgets(s,mx,fp)==NULL )
      return(-1);
   len=killnl(s,&nl);
   if( !nl ) { /* Newline not found yet */
      int c;
      while( (c=getc(fp)) != '\n' && c != EOF )
         ;
   }
   return(len);
}

char *areadln()
{
   char	buf[BUFLEN];
   return( readln(buf,BUFLEN)<0 ? NULL : strsave(buf) );
}

char *afreadln(fp)
FILE *fp;
{
   char	buf[BUFLEN];
   return( freadln(buf,BUFLEN,fp)<0 ? NULL : strsave(buf) );
}

long readlong(def)
long def;
{
   char	line[BUFLEN];
   long	val;

   if( readln(line,BUFLEN)<0 || sscanf(line,"%ld",&val) < 1 )
      return(def);
   return(val);
}

double readdouble(def)
double def;
{
   char line[BUFLEN];
   double val;

   if( readln(line,BUFLEN)<0 || sscanf(line,"%lf",&val) < 1 )
      return(def);
   return(val);
}

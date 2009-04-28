/*
 *  Main program
 *  by Tom Epperly
 *  Part of Ascend
 *  Version: $Revision: 1.28 $
 *  Version control file: $RCSfile: main.c,v $
 *  Date last modified: $Date: 1998/02/18 22:57:15 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
 *  COPYING.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>  /* was compiler/actype.h */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/pool.h>
#include "compiler.h"
#include "ascCompiler.h"
#include "module.h"
#include "symtab.h"


#include "functype.h"
#include "types.h"
#include "instance_enum.h"
#include "setinstval.h"
#include "child.h"
#include "type_desc.h"
#include "stattypes.h"
#include "slist.h"
#include "scanner.h"
#include "evaluate.h"
#include "units.h"
#include "typedef.h"
#include "dump.h"
#include "prototype.h"
#include "slist.h"
#include "typedef.h"
#include "library.h"
#include "arrayinst.h"
#include "instance_name.h"
#include "relation_type.h"
#include "logical_relation.h"
#include "find.h"
#include "relation.h"
#include "logrelation.h"
#include "setinstval.h"
#include "interface.h"
#include "universal.h"
#include "extfunc.h"
#include "packages.h" /* user packages and functions */
#include "pending.h"
#include "value_type.h"
#include "temp.h"
#include "forvars.h"

#ifndef lint
static CONST char MainDriverID[] = "$Id: main.c,v 1.28 1998/02/18 22:57:15 ballan Exp $";
#endif

#ifdef YYDEBUG
extern int zz_debug;
#endif
extern int zz_parse();

/* cmu hacks so we don't move binaries out from under testers */
/* if ZEPHYR == 0, hacks turned off */
#define ZEPHYR 0
/* who to tell, and who not to tell when he himself runs it */
#define ZNOTIFY "ballan"

static
void MainOpenModule(char *name)
{
  if (Asc_OpenModule(name,NULL)==NULL){
    FPRINTF(ASCERR,"Unable to find module %s in search path. Goodbye.\n",
            name);
    exit(2);/*NOTREACHED*/
  }
}


int main(int argc, char **argv)
{
  int c,errflg=0;
  char *filename;
#if ZEPHYR
  char zbuf[1024];
#endif
  extern int optind,opterr;
  extern char *optarg;
#ifdef YYDEBUG
  zz_debug=0;
#endif
  while ((c = getopt(argc,argv,"d")) != EOF) {
    switch(c){
    case 'd':
#ifdef YYDEBUG
      zz_debug=1;
#else
      FPRINTF(ASCERR,"Sorry this wasn't compiled with YYDEBUG defined.\n");
#endif
      break;
    case '?':
      errflg++;
    }
  }
  if (optind == (argc-1)){
    filename=argv[optind];
  }
  else errflg++;
  if (errflg) {
    FPRINTF(ASCERR,"usage: %s [-d] filename\n",argv[0]);
    exit(2);/*NOTREACHED*/
  }
  PRINTF("ASCEND VERSION IV\n");
  PRINTF("Compiler Implemention Version: 2.0\n");
  PRINTF("Written by Tom Epperly\n");
  PRINTF("Copyright(C) 1990, 1993, 1994 Thomas Guthrie Weidner Epperly\n");
  PRINTF("             1995, 1996 Kirk Andre Abbott, Benjamin Andrew Allan\n");
  PRINTF("             1997 Carnegie Mellon University\n");
  PRINTF(
   "ASCEND comes with ABSOLUTELY NO WARRANTY; for details type `warranty'.\n");
  PRINTF(
   "This is free software, and you are welcome to redistribute it under\n");
  PRINTF("certain conditions; type `copying' for details.\n");

  if( Asc_CompilerInit(1) != 0 ) {
    ASC_PANIC("Asc_CompilerInit returned nonzero");
  }

  /*
   *  Import the value of ASCENDLIBRARY into the environment before
   *  trying to read any modules.
   */
  Asc_ImportPathList(ASC_ENV_LIBRARY);

  MainOpenModule(filename);

#if ZEPHYR
  /*
   * USER and HOST are not universally used, but we'll assume an mthomas setup
   */
  if (getenv("USER")!=NULL) {
    if ( strcmp(getenv("USER"),ZNOTIFY)!=0) {
      if (getenv("HOST") !=NULL) {
        sprintf(zbuf,"/usr/local/bin/zwrite -d -q %s -m %s@%s running %s",
          ZNOTIFY,getenv("USER"),getenv("HOST"), argv[0]);
      } else {
        sprintf(zbuf,"/usr/local/bin/zwrite -d -q %s -m %s running %s",
          ZNOTIFY,getenv("USER"), argv[0]);
      }
    }
  } else {
    sprintf(zbuf,"/usr/local/bin/zwrite -d -q %s -m %s %s",
      ZNOTIFY,"Somebody somewhere ran ", argv[0]);
  }
  system(zbuf);
#endif

  zz_parse();
  ascstatus("Memory status after parsing input file");
  Interface();			/* call an interface */
  ascstatus("Memory status after interface exits");

  Asc_CompilerDestroy();

  ascshutdown("Memory status just before exiting");
  return 0;
}

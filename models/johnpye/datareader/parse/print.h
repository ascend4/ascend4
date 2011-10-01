#ifndef PRINT_H
#define PRINT_H

#include <stdlib.h>
#include <stdio.h>
#include "types.h"

/* global variables */

extern FILE *printGlobalStdOut;

/* macros */

#define printFormat(p,args) \
  ( printGlobalStdOut = p->file \
  , printFormatted args \
  , fflush( printGlobalStdOut ) \
  )

/* types */

typedef struct
{
  FILE *file;
  cbool close;
} print;

/* functions */

print *printCreateFile( FILE *file );
print *printCreateFileName( char *name );
print *printCreateFileNameAppend( char *name );
void  printFormatted( char *s, ... );
void  printDispose( print *p );

#endif /* PRINT_H */

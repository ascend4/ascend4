/* new.c */

#include <string.h>
#include "error.h"
#include "new.h"

/* global variables */

static cbool newGlobalReportsError = true;

/* functions */

void *newAllocLocation( int n, char *file, int line )
{
  void *ptr = malloc( n );
  
  if ( !ptr && newGlobalReportsError )
    errorReportLocationWith( 1, file, line, ("Object allocation failed.\n") );

  return ptr;
}

void *newReAllocLocation( void *arr, int n, char *file, int line )
{
  void *ptr = realloc( arr, n );
  
  if ( !ptr && newGlobalReportsError )
    errorReportLocationWith( 1, file, line, ("Object reallocation failed.\n") ); 
    
  return ptr;
}

void *newCopy( void *obj, int n )
{
  void *obj1 = malloc( n );
  if ( obj1 )
    memcpy( obj1, obj, n );
  return obj1;
}

void *newCopyLocation( void *obj, int n, char *file, int line )
{
  void *obj1 = malloc( n );
  
  if ( obj1 )
    memcpy( obj1, obj, n );
  else if ( newGlobalReportsError )
    errorReportLocationWith( 1, file, line, ("Object copy failed.\n") );
  
  return obj1;
}

void newReportsError( cbool report )
{
  newGlobalReportsError = report;
}

/* end new.c */

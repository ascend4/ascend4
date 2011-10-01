#ifndef NEW_H
#define NEW_H

/** @file
	Memory allocation functions used by parse.c. 
	@todo May need to review this stuff.
*/

#include <stdlib.h>
#include "types.h"

/* macros */

#define NEW(type) \
  (type*)newAllocLocation(sizeof(type),__FILE__,__LINE__)

#define newMaybe(type) \
  (type*)malloc(sizeof(type))

#define array(type,n) \
  (type*)newAllocLocation((n)*sizeof(type),__FILE__,__LINE__)

#define rearray(type,arr,n) \
  (type*)newReAllocLocation(arr,(n)*sizeof(type),__FILE__,__LINE__)

#define arrayMaybe(type,n) \
  (type*)malloc((n)*sizeof(type))

#define rearrayMaybe(type,arr,n) \
  (type*)realloc(arr,(n)*sizeof(type))

#define copy(type,obj) \
  (type*)newCopyLocation(obj,sizeof(type),__FILE__,__LINE__)
  
#define copyMaybe(type,obj) \
  (type*)newCopy(obj,sizeof(type))

/* functions */

void *newAllocLocation( int n, char *file, int line );
void *newReAllocLocation( void *arr, int n, char *file, int line );
void *newCopy( void *obj, int n );
void *newCopyLocation( void *obj, int n, char *file, int line );
void newReportsError( cbool error );

#endif /* NEW_H */

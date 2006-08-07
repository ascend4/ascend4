/* TextTable: A small C library to read data tables in text files */

/*
 * Copyright (c) 2003,2004, Jean-Sebastien Roy (js@jeannot.org)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* @(#) $Jeannot: texttable.h,v 1.14 2004/10/26 17:47:29 js Exp $ */

#ifndef _TEXTTABLE_
#define _TEXTTABLE_

#include <stddef.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * TextTable types
 */
/* We should add : quoted string */
typedef enum {
  TEXTTABLE_CHAR = 0,  /* char */
  TEXTTABLE_SHORT,  /* short */
  TEXTTABLE_INT,  /* int */
  TEXTTABLE_LONG,  /* long */
#ifdef HAS_LONGLONG
  TEXTTABLE_LONGLONG,  /* long long */
#endif
  TEXTTABLE_FLOAT,  /* float */
  TEXTTABLE_DOUBLE, /* double */
  TEXTTABLE_STRING,  /* string */
  TEXTTABLE_UNKNOWN, /* unknow type */
  TEXTTABLE_TYPE_MAX
} texttable_type;

/*
 * TextTable errors
 */
typedef enum {
  TEXTTABLE_NOERR = 0,  /* no error */
  TEXTTABLE_ENOMEM = 1,  /* memory allocation error */
  TEXTTABLE_EINVAL = 2,  /* invalid argument */
  TEXTTABLE_EUNKTYPE = 3,  /* unknown type */
  TEXTTABLE_ETOOFEWC = 4, /* too few columns */
  TEXTTABLE_ETOOMANYC = 5, /* too many columns */
  TEXTTABLE_EINDATA = 6, /* invalid data */
  TEXTTABLE_EFERROR = 7, /* file error */
  TEXTTABLE_ERR_MAX = 8
} texttable_error;

/*
 * A TextTable
 */
typedef struct texttable_
{
  size_t columns;
  size_t rows;
  texttable_type *type;
  char **name;
  union
  {
    void **dvoid;
    char **dchar;
    short **dshort;
    int **dint;
    long **dlong;
#ifdef HAS_LONGLONG
    long long **dlonglong;
#endif
    float **dfloat;
    double **ddouble;
    char ***dstring;
  } data;
} texttable;

extern size_t texttable_sizeof[TEXTTABLE_UNKNOWN];

/*
 * Given a number of columns, allocate a texttable structure.
 * Returns a texttable_error.
 */
extern int texttable_new(texttable *tt, size_t columns);

/*
 * Free a texttable structure including any allocated strings.
 */
extern void texttable_free(texttable *tt);

/*
 * Allocate a texttable structure by reading the first line of stream 'file'.
 * The fields are separated by 'delimiter'.
 * If 'hasnames' is != 0, names for the fields are extracted from the first line
 * and the file position is set to the begining of the second line.
 *
 * Returns a texttable_error.
 */
extern int texttable_readheader(texttable *tt, FILE *file, char delimiter,
  int hasnames);

/*
 * Guess the types of a texttable, according to the first 'rows' lines of 
 * the file (or all is rows is < 0).
 * The fields are separated by 'delimiter'.
 *
 * Returns a texttable_error.
 */
extern int texttable_guesstype(texttable *tt, FILE *file, char delimiter, 
  size_t rows);

/*
 * Allocate the provided number of rows in the texttable
 *
 * Returns a texttable_error.
 */
extern int texttable_allocate(texttable *tt, size_t rows);

/*
 * Resize the data tables of a texttable to ensure maxrows rows are available.
 * If maxrows < tt->rows, adjust tt->rows.
 *
 * Returns a texttable_error.
 */
extern int texttable_reallocate(texttable *tt, size_t maxrows);

/*
 * Append data from stream file into texttable tt
 * The fields are separated by 'delimiter'.
 *
 * Returns a texttable_error.
 */
extern int texttable_readdata(texttable *tt, FILE *file, char delimiter);

/*
 * Write a texttable tt to stream 'file' using delimiter 'delimiter'.
 * If hasnames != 0, write the names of the fields in the first line.
 *
 * Returns a texttable_error.
 */
extern int texttable_write(texttable *tt, FILE *file, char delimiter, 
  int hasnames);

/*
 * Return the error message relative to texttable_error err.
 */
extern char *texttable_strerror(int err);

/*
 * Print an error message. Used by the CHECKTTSTATUS macro
 */
extern void texttable_perror(int status, char *file, int line);

/* High level API */

/*
 * Read some data columns from a file using the specified delimiter.
 * If hasname !=0, the first line contains the names of the columns.
 * type must point to an array of column types
 * If name != NULL, point names to an array of the names of the columns.
 * Point rows to the number of rows read.
 * 
 * Returns a texttable_error.
 */
extern int texttable_readtable(FILE *file, char delimiter, int hasnames,
  size_t columns,  texttable_type *type, char ***name, size_t *rows, ...);

/*
 * Write some data columns to a file using the specified delimiter.
 * columns is the number of columns to write.
 * If name != NULL, write the names of the columns on the first line.
 * rows is the number of rows to write.
 * 
 * Returns a texttable_error.
 */
extern int texttable_writetable(FILE *file, char delimiter, size_t columns,
  texttable_type *type, char **name, size_t rows, ...);

#if defined(__cplusplus)
}
#endif

/*
 * Assume status is a variable containing a texttable_error.
 * If an error occured, print it and and exit.
 */
#define CHECKTTSTATUS if(status != TEXTTABLE_NOERR) texttable_perror(status, \
  __FILE__, __LINE__)

#endif /* _TEXTTABLE_ */

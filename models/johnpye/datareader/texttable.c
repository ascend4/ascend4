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

static char const rcsid[] =
  "@(#) $Jeannot: texttable.c,v 1.15 2004/10/26 17:47:29 js Exp $";

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <stdarg.h>

#include "texttable.h"

/* Extern constant */

size_t texttable_sizeof[TEXTTABLE_TYPE_MAX-1] = {
  sizeof(char),
  sizeof(short),
  sizeof(int),
  sizeof(long),
#ifdef HAS_LONGLONG
  sizeof(long long),
#endif
  sizeof(float),
  sizeof(double),
  sizeof(char *)
};

/* Static constants */

static char *texttable_errorstrings[TEXTTABLE_ERR_MAX] = {
  "texttable: no error",
  "texttable: memory allocation error",
  "texttable: invalid argument",
  "texttable: unknown type",
  "texttable: too few columns",
  "texttable: too many columns",
  "texttable: invalid data",
  "texttable: file error"
};

/* Static prototypes */

/*
 * Read a line from stream 'file'.
 * Return NULL if an error occurs.
 * If non NULL, the returned pointer point to a '\0' terminated string.
 * The end of line character '\n' is ommited.
 */
static char *texttable_fgetln(FILE *file);

/*
 * Read a line from stream 'file'.
 * Return NULL if an error occurs.
 * If non NULL, the returned pointer point to a '\0' terminated string.
 * The end of line character '\n' is ommited.
 * Temporary storage of size '*bufsize' provided by 'buf' is used if 
 * buff != NULL, buf may be reallocated. In this case, the returned pointer
 * point to the reallocated buffer. *bufsize contains the new size of the
 * buffer.
 */
static char *texttable_fgets(FILE *file, char *buf, size_t *bufsize);

/*
 * Separate strings using 'delimiter' as the delimiter.
 * Locate and replace in the *stringp the first occurence of the delimiter
 * character. The location of the next character after the delimiter 
 * character (or NULL, if the end of the string was reached) is stored in
 * *stringp.  The original value of *stringp is returned.
 *
 * Loosely based upon the 'strsep' BSD libc function.
 */
static char *texttable_strsep(char **stringp, char delimiter);

/* Implementation */

char *texttable_fgetln(FILE *file)
{
  return texttable_fgets(file, NULL, NULL);
}

/* TEXTTABLE_FGETS_BUFFER must be greater or equal to 2 */
#define TEXTTABLE_FGETS_BUFFER 256
char *texttable_fgets(FILE *file, char *buf, size_t *bufsize)
{
  char *nbuf, *cur;
  size_t remsize, tbufsize = 0;
  
  if (bufsize == NULL)
    bufsize = &tbufsize;
  
  if (buf == NULL)
  {
    if (*bufsize < 2)
      *bufsize = TEXTTABLE_FGETS_BUFFER;
    buf = malloc((*bufsize)*sizeof(*buf));
  }

  remsize = *bufsize;
  cur = buf;
  while (1)
  {
    char *res;
    
    res = fgets(cur, (int) remsize, file);
    if (res == NULL)
    {
      free(buf);
      return NULL;
    }
    if ((res = strchr(cur,'\n')) != NULL)
    {
      *res = '\0';
      return buf;
    }
    if (feof(file))
      return buf;
    
    nbuf = realloc(buf, (*bufsize)*2*sizeof(*buf));
    if (nbuf == NULL)
    {
      free(buf);
      return NULL;
    }
    buf = nbuf;
    cur = buf + *bufsize - 1;
    remsize = *bufsize + 1;
    *bufsize += *bufsize;
  }
}

char *texttable_strsep(char **stringp, char delimiter)
{
  char *s, *tok = *stringp;
  char c;

  if (tok == NULL)
    return NULL;

  for (s = tok;(c = *s);s++)
  {
    if (delimiter == c)
    {
      *s = 0;
      *stringp = s+1;
      return tok;
    }
  }
  *stringp = NULL;
  return tok;
}

int texttable_new(texttable *tt, size_t columns)
{
  size_t column;

  tt->type = NULL;
  tt->name = NULL;
  tt->data.dvoid = NULL;
  tt->rows = 0;
  tt->columns = columns;
  
  if (tt->columns == 0)
    return TEXTTABLE_NOERR;
  
  tt->type = malloc(sizeof(*tt->type)*tt->columns);
  if (tt->type == NULL)
  {
    texttable_free(tt);
    return TEXTTABLE_ENOMEM;
  }
  for (column=0;column<tt->columns;column++)
    tt->type[column] = TEXTTABLE_UNKNOWN;

  tt->name = malloc(sizeof(*tt->name)*tt->columns);
  if (tt->name == NULL)
  {
    texttable_free(tt);
    return TEXTTABLE_ENOMEM;
  }
  for (column=0;column<tt->columns;column++)
    tt->name[column] = NULL;

  tt->data.dvoid = malloc(sizeof(*tt->data.dvoid)*tt->columns);
  if (tt->data.dvoid == NULL)
  {
    texttable_free(tt);
    return TEXTTABLE_ENOMEM;
  }
  for (column=0;column<tt->columns;column++)
    tt->data.dvoid[column] = NULL;

  return TEXTTABLE_NOERR;
}

void texttable_free(texttable *tt)
{
  size_t column;

  if (tt->name != NULL)
  {
    for (column=0;column<tt->columns;column++)
      if (tt->name[column] != NULL)
        free(tt->name[column]);
    free(tt->name);
    tt->name = NULL;
  }

  if (tt->data.dvoid != NULL)
  {
    for (column=0;column<tt->columns;column++)
      if (tt->data.dvoid[column] != NULL)
      {
        /* free strings */
        if (tt->type[column] == TEXTTABLE_STRING)
        {
          size_t row;
          for (row = 0; row < tt->rows; row++)
            free(((char **)tt->data.dvoid[column])[row]);
        }
        free(tt->data.dvoid[column]);
      }
    free(tt->data.dvoid);
    tt->data.dvoid = NULL;
  }
  
  if (tt->type != NULL)
  {
    free(tt->type);
    tt->type = NULL;
  }
}

int texttable_readheader(texttable *tt, FILE *file, char delimiter,
  int hasnames)
{
  char *c;
  size_t columns;
  int err;
  char *header;
  long offset;
  
  offset = ftell(file);
  if (offset == -1)
    return TEXTTABLE_EFERROR;
  
  header = texttable_fgetln(file);
  if (header == NULL)
  {
    if (feof(file) || ferror(file))
      return TEXTTABLE_EFERROR;
    return TEXTTABLE_ENOMEM;
  }
    
  /* Counting the columns */
  c = header;
  columns = 1;
  while (*c)
    if (*(c++) == delimiter)
      columns ++;
  
  if (!hasnames)
  {
    err = fseek(file, offset, SEEK_SET);
    if (err)
      return TEXTTABLE_EFERROR;
  }
  
  err = texttable_new(tt, columns);
  if (err)
  {
    free(header);
    return err;
  }
  
  /* Reading the names */
  if (hasnames)
  {
    char *input = header, *tok;
    for (columns = 0;(tok = texttable_strsep(&input, delimiter)) != NULL; 
      columns++)
    {
      tt->name[columns] = ASC_STRDUP(tok);
      if (tt->name[columns] == NULL)
      {
        texttable_free(tt);
        free(header);
        return TEXTTABLE_ENOMEM;
      }
    }
  }
  
  free(header);
  
  return TEXTTABLE_NOERR;
}

int texttable_guesstype(texttable *tt, FILE *file, char delimiter, size_t rows)
{
  size_t column;
  int err;
  char *buf;
  long offset;
  
  offset = ftell(file);
  if (offset == -1)
    return TEXTTABLE_EFERROR;
  
  while ((rows--) && (buf = texttable_fgetln(file)) != NULL)
  {
    char *input = buf, *tok;
    for (column = 0;
      (tok = texttable_strsep(&input, delimiter)) != NULL;column++)
    {
      if (column == tt->columns)
        return TEXTTABLE_ETOOMANYC; /* To many columns */

      if (tok[0] == '\0' && tt->type[column] != TEXTTABLE_CHAR
        && tt->type[column] != TEXTTABLE_STRING)
      {
        if(tt->type[column] == TEXTTABLE_UNKNOWN)
          tt->type[column] = TEXTTABLE_CHAR;
        else
          tt->type[column] = TEXTTABLE_STRING;
      }

      if (tt->type[column] == TEXTTABLE_UNKNOWN)
        tt->type[column] = TEXTTABLE_INT; /* Default type */

      switch (tt->type[column])
      {
        case TEXTTABLE_CHAR:
          if (tok[0] != '\0' && tok[1] != '\0') /* strlen > 1 */
            tt->type[column] = TEXTTABLE_STRING;
          break;
        case TEXTTABLE_SHORT:
        case TEXTTABLE_INT:
        case TEXTTABLE_LONG:
#ifdef HAS_LONGLONG
        case TEXTTABLE_LONGLONG:
#endif
        {
          char *endptr;
#ifdef HAS_LONGLONG
          long long value;
#else
          long value;
#endif

          errno = 0; /* Clear error */
#ifdef HAS_LONGLONG
          value = strtoll(tok, &endptr, 10);
#else
          value = strtol(tok, &endptr, 10);
#endif
          if (endptr == tok)
          {
            /* No digits */
            tt->type[column] = TEXTTABLE_STRING;
            break;
          }
          /* Skip whitespace */
          if(*endptr)
            for (;isspace(*endptr);endptr++);
          if (*endptr)
          {
            /* Invalid character */
            tt->type[column] = TEXTTABLE_DOUBLE;
            /* will go to 'case TEXTTABLE_DOUBLE' */
          }
          else
          {
            if (errno == ERANGE)
            {
              tt->type[column] = TEXTTABLE_STRING;
              break;
            }
            switch (tt->type[column])
            {
              case TEXTTABLE_SHORT:
                if (value <= SHRT_MAX && value >= SHRT_MIN)
                  break;
                tt->type[column] = TEXTTABLE_INT;
              case TEXTTABLE_INT:
                if (value <= INT_MAX && value >= INT_MIN)
                  break;
                tt->type[column] = TEXTTABLE_LONG;
              case TEXTTABLE_LONG:
#ifdef HAS_LONGLONG
                if (value <= LONG_MAX && value >= LONG_MIN)
                  break;
                tt->type[column] = TEXTTABLE_LONGLONG;
              case TEXTTABLE_LONGLONG:
#endif
                break;
              default:;
                /* NOTREACHED */
            }
            break;
          }
        }
        case TEXTTABLE_FLOAT:
        case TEXTTABLE_DOUBLE:
        {
          char *endptr;
          double value;

          errno = 0; /* Clear error */
          value = strtod(tok, &endptr);
          if (endptr == tok)
          {
            /* No digits */
            tt->type[column] = TEXTTABLE_STRING;
            break;
          }
          /* Skip whitespace */
          for (;isspace(*endptr);endptr++);
          if (*endptr || errno == ERANGE)
          {
            /* Invalid character */
            tt->type[column] = TEXTTABLE_STRING;
            break;
          }
          if (tt->type[column] == TEXTTABLE_FLOAT)
          {
            if (value <= FLT_MAX && value > -FLT_MAX)
              break;
            tt->type[column] = TEXTTABLE_DOUBLE;
          }
          break;
        }
        case TEXTTABLE_STRING:
          break;
        default:
          free(buf);
          return TEXTTABLE_EUNKTYPE;
      }
    }
  
    free(buf);
  }
    
  if (ferror(file))
    return TEXTTABLE_EFERROR;
  
  if (!feof(file) && (rows != -1))
    return TEXTTABLE_ENOMEM;
  
  err = fseek(file, offset, SEEK_SET);
  if (err)
    return TEXTTABLE_EFERROR;

  return TEXTTABLE_NOERR;
}

int texttable_allocate(texttable *tt, size_t rows)
{
  int status;
  status = texttable_reallocate(tt, rows);
  if (status) return status;
  tt->rows = rows;
  return TEXTTABLE_NOERR;
}

int texttable_reallocate(texttable *tt, size_t maxrows)
{
  size_t column;

  for (column=0;column<tt->columns;column++)
    if (tt->type[column]<0 || tt->type[column]>=TEXTTABLE_UNKNOWN)
      return TEXTTABLE_EUNKTYPE;
  
  if (maxrows<tt->rows)
    tt->rows = maxrows;
  
  for (column=0;column<tt->columns;column++)
  {
    void *ndata;
    ndata = realloc(tt->data.dvoid[column],
      maxrows*texttable_sizeof[tt->type[column]]);
    if (ndata != NULL)
     tt->data.dvoid[column] = ndata;
    else if (maxrows>tt->rows)
      return TEXTTABLE_ENOMEM;
  }

  return TEXTTABLE_NOERR;
}

int texttable_readdata(texttable *tt, FILE *file, char delimiter)
{
  char *buf = NULL;
  size_t maxrows, column, bufsize = 0;
  int err;

  maxrows = tt->rows;
  
  while ((buf = texttable_fgets(file, buf, &bufsize)) != NULL)
  {
    char *input = buf, *tok;

    /* Allocate enough space */
    if (tt->rows == maxrows)
    {
      maxrows += maxrows;
      if (!maxrows)
        maxrows = 256;
      texttable_reallocate(tt, maxrows);
    }
  
    for (column = 0;
      (tok = texttable_strsep(&input, delimiter)) != NULL;column++)
    {
      if (column == tt->columns)
      {
        free(buf);
        return TEXTTABLE_ETOOMANYC; /* To many columns */
      }
        
      switch (tt->type[column])
      {
        case TEXTTABLE_CHAR:
          tt->data.dchar[column][tt->rows] = tok[0];
          break;
        case TEXTTABLE_SHORT:
        case TEXTTABLE_INT:
        case TEXTTABLE_LONG:
#ifdef HAS_LONGLONG
        case TEXTTABLE_LONGLONG:
#endif
        {
          char *endptr;
#ifdef HAS_LONGLONG
          long long value;
#else
          long value;
#endif

          errno = 0; /* Clear error */
#ifdef HAS_LONGLONG
          value = strtoll(tok, &endptr, 10);
#else
          value = strtol(tok, &endptr, 10);
#endif
          if (endptr == tok)
          {
            /* No digits */
            free(buf);
            return TEXTTABLE_EINDATA;
          }
          /* Skip whitespace */
          for (;isspace(*endptr);endptr++);
          if (*endptr)
          {
            /* Invalid character */
            free(buf);
            return TEXTTABLE_EINDATA;
          }
          if (errno == ERANGE)
          {
            free(buf);
            return TEXTTABLE_EINDATA;
          }
          switch (tt->type[column])
          {
            case TEXTTABLE_SHORT:
              if (value <= SHRT_MAX && value >= SHRT_MIN)
              {
                tt->data.dshort[column][tt->rows] = (short)value;
                break;
              }
              free(buf);
              return TEXTTABLE_EINDATA;
            case TEXTTABLE_INT:
              if (value <= INT_MAX && value >= INT_MIN)
              {
                tt->data.dint[column][tt->rows] = (int)value;
                break;
              }
              free(buf);
              return TEXTTABLE_EINDATA;
            case TEXTTABLE_LONG:
#ifdef HAS_LONGLONG
              if (value <= LONG_MAX && value >= LONG_MIN)
              {
                tt->data.dlong[column][tt->rows] = (long)value;
                break;
              }
              free(buf);
              return TEXTTABLE_EINDATA;
            case TEXTTABLE_LONGLONG:
              tt->data.dlonglong[column][tt->rows] = (long long)value;
#else
              tt->data.dlong[column][tt->rows] = (long)value;
#endif
              break;
            default:;
              /* NOTREACHED */
          }
          break;
        }
        case TEXTTABLE_FLOAT:
        case TEXTTABLE_DOUBLE:
        {
          char *endptr;
          double value;

          errno = 0; /* Clear error */
          value = strtod(tok, &endptr);
          if (endptr == tok)
          {
            /* No digits */
            free(buf);
            return TEXTTABLE_EINDATA;
          }
          /* Skip whitespace */
          if (*endptr)
            for (;isspace(*endptr);endptr++);
          if (*endptr || errno == ERANGE)
          {
            /* Invalid character */
            free(buf);
            return TEXTTABLE_EINDATA;
          }
          if (tt->type[column] == TEXTTABLE_FLOAT)
          {
            if (value <= FLT_MAX && value > -FLT_MAX)
            {
              tt->data.dfloat[column][tt->rows] = (float)value;
              break;
            }
            else
            {
              free(buf);
              return TEXTTABLE_EINDATA;
            }
          }
          tt->data.ddouble[column][tt->rows] = (double)value;
          break;
        }
        case TEXTTABLE_STRING:
        {
          char *newstr = ASC_STRDUP(tok);
          if (newstr == NULL)
          {
            free(buf);
            return TEXTTABLE_ENOMEM;
          }
          tt->data.dstring[column][tt->rows] = newstr;
          break;
        }
        default:
          free(buf);
          return TEXTTABLE_EUNKTYPE;
      }
    }

    if (column != tt->columns)
    {
      free(buf);
      return TEXTTABLE_ETOOFEWC; /* Too few columns */
    }
    
    tt->rows ++;
  }
  
  /* Unallocate surplus */
  if (maxrows > tt->rows)
  {
    err = texttable_reallocate(tt, tt->rows);
    if (err)
      return err; /* NOTREACHED */
  }
    
  if (ferror(file))
    return TEXTTABLE_EFERROR;
  
  if (!feof(file))
    return TEXTTABLE_ENOMEM;
  
  /* We should test for a file error */

  return TEXTTABLE_NOERR;
}

int texttable_write(texttable *tt, FILE *file, char delimiter, int hasnames)
{
  size_t row, column;
  int err;
  
  if (hasnames)
  {
    for (column=0;column<tt->columns;column++)
    {
      err = fputs(tt->name[column], file);
      if (err == EOF)
        return TEXTTABLE_EFERROR;
      if (column<(tt->columns-1))
      {
        err = putc(delimiter, file);
        if (err == EOF)
          return TEXTTABLE_EFERROR;
      }
    }
    err = putc('\n', file);
    if (err == EOF)
      return TEXTTABLE_EFERROR;
  }
  
  for (row = 0;row<tt->rows;row++)
  {
    for (column = 0;column<tt->columns;column++)
    {
      switch (tt->type[column])
      {
        case TEXTTABLE_CHAR:
          if (tt->data.dchar[column][row])
          {
            err = fputc(tt->data.dchar[column][row], file);
            if (err == EOF)
              return TEXTTABLE_EFERROR;
          }
          break;
        case TEXTTABLE_SHORT:
          err = fprintf(file, "%d", (int)(tt->data.dshort[column][row]));
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
        case TEXTTABLE_INT:
          err = fprintf(file, "%d", tt->data.dint[column][row]);
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
        case TEXTTABLE_LONG:
          err = fprintf(file, "%ld", tt->data.dlong[column][row]);
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
#ifdef HAS_LONGLONG
        case TEXTTABLE_LONGLONG:
          err = fprintf(file, "%lld", tt->data.dlonglong[column][row]);
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
#endif
        case TEXTTABLE_FLOAT:
          err = fprintf(file, "%g", (double)(tt->data.dfloat[column][row]));
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
        case TEXTTABLE_DOUBLE:
          err = fprintf(file, "%g", tt->data.ddouble[column][row]);
          if (err < 0)
            return TEXTTABLE_EFERROR;
          break;
        case TEXTTABLE_STRING:
          err = fputs(tt->data.dstring[column][row], file);
          if (err == EOF)
            return TEXTTABLE_EFERROR;
          break;
        default:
          return TEXTTABLE_EUNKTYPE;
      }
      if (column<(tt->columns-1))
      {
        err = putc(delimiter, file);
        if (err == EOF)
          return TEXTTABLE_EFERROR;
      }
    }  
    err = putc('\n', file);
    if (err == EOF)
      return TEXTTABLE_EFERROR;
  }

  return TEXTTABLE_NOERR;
}

char *texttable_strerror(int err)
{
  if (err<0 || err>=TEXTTABLE_ERR_MAX)
    return "texttable: error unknown";
  return texttable_errorstrings[err];
}

void texttable_perror(int status, char *file, int line)
{
#ifndef NDEBUG
  fprintf(stderr, "%s source=%s line=%d\n", texttable_strerror(status), 
    file, line);
#else
  fprintf(stderr, "%s\n", texttable_strerror(status));
#endif
  exit(EXIT_FAILURE);
}

/* High level API */

int texttable_readtable(FILE *file, char delimiter, int hasnames,
  size_t columns,  texttable_type *type, char ***name, size_t *rows, ...)
{
  int status;
  texttable tt;
  va_list ap;
  size_t c;

  status = texttable_readheader(&tt, file, delimiter, hasnames);
  if (status) goto cleanUp;

  if (tt.columns<columns)
  {
    status = TEXTTABLE_ETOOFEWC;
    goto cleanUp;
  }

  if (tt.columns>columns)
  {
    status = TEXTTABLE_ETOOMANYC;
    goto cleanUp;
  }

  for (c=0;c<columns;c++)
    tt.type[c] = type[c];

  status = texttable_readdata(&tt, file, delimiter);
  if (status) goto cleanUp;

  if(columns > 0)
  {
    va_start(ap, rows);
    for (c=0;c<columns;c++)
    {
      void **ptr = va_arg(ap, void **);
      if (ptr != NULL)
      {
        *ptr = tt.data.dvoid[c];
        tt.data.dvoid[c] = NULL;
      }
    }
    va_end(ap);
  }

  *rows = tt.rows;

  if (name != NULL)
  {
    *name = tt.name;
    tt.name = NULL;
  }
    
cleanUp:
  texttable_free(&tt);
  return status;
}

int texttable_writetable(FILE *file, char delimiter, size_t columns,
  texttable_type *type, char **name, size_t rows, ...)
{
  int status;
  texttable tt;
  va_list ap;
  size_t c;

  status = texttable_new(&tt, columns);
  if (status) goto cleanUp;

  if (name != NULL)
  {
    for (c=0;c<columns;c++)
      tt.name[c] = name[c];
  }

  if(columns > 0)
  {
    va_start(ap, rows);
    for (c=0;c<columns;c++)
    {
      void *ptr = va_arg(ap, void *);
      tt.type[c] = type[c];
      tt.data.dvoid[c] = ptr;
    }
    va_end(ap);
  }

  tt.rows = rows;

  status = texttable_write(&tt, file, delimiter, name != NULL);

  if (name != NULL)
  {
    for (c=0;c<columns;c++)
    {
      tt.name[c] = NULL;
      tt.data.dvoid[c] = NULL;
    }
  }

cleanUp:
  texttable_free(&tt);
  return status;
}

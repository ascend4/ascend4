#ifndef PARSE_H
#define PARSE_H

/** @file
	General-purpose parser functions.
*/

#include "config.h"
#include "types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif

/* macros */

#define catVariable (catAlpha | catDigit | catUnderscore)
#define catKeyword  (catAlpha | catDigit | catMinus)

#define parseLexThisVariable(p,s) parseLexThisCategory(p,catVariable,s)
#define parseLexVariable(p,s)     parseLexCategory(p,catVariable,s)

#define parseLexThisKeyword(p,s)  parseLexThisCategory(p,catKeyword,s)
#define parseLexKeyword(p,s)      parseLexCategory(p,catKeyword,s)

#define parseLexThisOper(p,s)     parseLexThisCategory(p,catOper,s)
#define parseLexOper(p,s)         parseLexCategory(p,catOper,s)

#define assign(ass)               (ass,1)
#define done                      (1)
#define fail                      (0)

#ifdef __GNUC__

#define many(p)                   ({while(p);1;})
#define maybe(p)                  (p,1)
#define one_or_more(p)            (p && many(p))

#endif

/* types */

typedef enum
{
  catAlpha      = 1,
  catDigit      = 2,
  catOper       = 4,
  catBracket    = 8,
  catQuote      = 16,
  
  catMinus      = 32,
  catUnderscore = 64
} category;

typedef
  category
  *judgement;

struct _parse;

typedef
  cbool
  (parseGetCharFunction)( struct _parse *, char * );

typedef
  cbool
  (parseUnGetCharFunction)( struct _parse *, char );

typedef
  void
  (parseDisposeFunction)( struct _parse * );

/**
	Parser data structure, keeps track of the data stream being parsed,
	and permits characters to be returned to the stream in the case where
	a particular pattern is not matched.
*/
typedef struct _parse{
  char      *buf;
  int       bufptr;
  judgement judge;
  int       line;

  /* for FILE implementations */
  FILE      *file;

  /* for string implementations */
  char      *contents;
  int       ptr;
  
  parseGetCharFunction   *getChar;
  parseUnGetCharFunction *unGetChar;
  parseDisposeFunction   *dispose;
} parse;

/* functions */

MSTRANP_API parse *parseCreateFile( FILE *file );
MSTRANP_API parse *parseCreateFileName( const char *name );
parse *parseCreateString( char *s );
MSTRANP_API void parseDispose( parse *p );

void parseSetJudgement( parse *p, short c, category );
void parseAddJudgement( parse *p, short c, category );
category parseGetJudgement( parse *p, short c );

cbool parseError( parse *p, char *s );
cbool parseEnd( parse *p );

cbool parseAChar( parse *p, char *c );
cbool parseAnyChar( parse *p );
cbool parseThisChar( parse *p, char c );
cbool parseUnParseChar( parse *p, char c );

cbool parseThisString( parse *p, const char *s );
cbool parseUnParseString( parse *p, char *s );

cbool parseDigit( parse *p, char *c );
cbool parseNumber( parse *p, unsigned *n );
cbool parseSignedNumber( parse *p, int *n );

cbool parseCharCategory( parse *p, category cat, char *c );

cbool parseSpaceAndComments( parse *p );


cbool parseLexThisCategory( parse *p, category cat, char *s );
cbool parseLexCategory( parse *p, category cat, char *s );
cbool parseLexNumber( parse *p, unsigned *n );
cbool parseLexSignedNumber( parse *p, int *n );
cbool parseLexThisString( parse *p, char *s );
cbool parseLexThisChar( parse *p, char c );
cbool parseLexEnd( parse *p );

#if 0
/* not currently using any of this stuff */
cbool parseLexIfKeyword( parse *p, char *s, cbool *b );
cbool parseLexIfNumber( parse *p, unsigned *n );
cbool parseLexIfSignedNumber( parse *p, int *n );
cbool parseLexKeywordNumber( parse *p, char *s, unsigned *n );
cbool parseLexKeywordSignedNumber( parse *p, char *s, int *n );
#endif

cbool parseQuoted( parse *p, char *s );
cbool parseLexQuoted( parse *p, char *s );

/* john's useful stuff */
cbool parseEOL(parse *p);
cbool parseWhiteChar(parse *p);
cbool parseWS(parse *p);
cbool parseCharIn(parse *p, const char *incl, char *c);
cbool parseCharExcept( parse *p, const char *except, char *c );
cbool parseStrExcept(parse *p, const char *except, char *str, int maxl);
cbool parseNonWS(parse *p, char *str, int maxl);
cbool parseEOLplus(parse *p);
cbool parseDouble(parse *p, double *d);

/**
	Accumulate the values from a stringified bit-field using this function.
	For each successive 0 or 1, call this function using 'value' to specify
	the binary value of that bit, and it will be set in 'result' if found to be
	'1'.

	@param value value of the current '1', if found.
	@param result (returned) value into which current value is ORed if next char
		equals '1'.
	@return 1 if a '0' or '1' is read, 0 otherwise.
*/
cbool parseBitChar(parse *p, unsigned value, unsigned *result);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_H */

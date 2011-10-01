/* parse.c */

#define MSTRANP_BUILD

#include "parse.h"
#include "new.h"
#include "types.h"
#include "error.h"
#include "CharactersInDouble.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* macros */

#define BUFSIZE 300

/* functions */

static void parseInitJudgement( judgement judge )
{
  const char *oper    = "!@#$%^&*+-=|\\~:<>./";
  const char *bracket = "[]{}()";
  const char *quote   = "\"'`";
  short c;
  int i;
  
  for ( i = 0; i < 256; i++ )
    judge[i] = 0;
  
  for ( c = 'a'; c <= 'z'; c++ )
    judge[c] = judge[c] | catAlpha;
  
  for ( c = 'A'; c <= 'Z'; c++ )
    judge[c] = judge[c] | catAlpha;
  
  for ( c = '0'; c <= '9'; c++ )
    judge[c] = judge[c] | catDigit;
  
  for ( i = 0; c = oper[i], c != '\0'; i++ )
    judge[c] = judge[c] | catOper;
  
  for ( i = 0; c = bracket[i], c != '\0'; i++ )
    judge[c] = judge[c] | catBracket;
  
  for ( i = 0; c = quote[i], c != '\0'; i++ )
    judge[c] = judge[c] | catQuote;
  
  judge['_'] = judge['_'] | catUnderscore;
  judge['-'] = judge['-'] | catMinus;
}

static void parseStandardDisposeFunction( parse *p )
{
  free( p->buf );
  free( p->judge );
  free( p );
}

static cbool parseStandardUnGetCharFunction( parse *p, char c )
{
  p->buf[p->bufptr] = c;
  
  if ( ++p->bufptr >= BUFSIZE )
  {
    p->bufptr--;
    return false;
  }
  
  if ( c == '\n' )
    p->line--;
  
  return true;
}

static void parseFileDisposeFunction( parse *p )
{
  free( p->buf );
  free( p->judge );
  fclose( p->file );
  free( p );
}

static cbool parseFileGetCharFunction( parse *p, char *c )
{
  if ( p->bufptr )
    *c = p->buf[--p->bufptr];
  else
    *c = getc( p->file );
  
  if ( *c == '\n' )
    p->line++;
  
  return ( *c != EOF );
}

static cbool parseStringGetCharFunction( parse *p, char *c )
{
  if ( p->bufptr )
    *c = p->buf[--p->bufptr];
  else if ( p->contents[p->ptr] != '\0' )
    *c = p->contents[p->ptr++];
  else
    return false;
  
  if ( *c == '\n' )
    p->line++;
  
  return true;
}

parse *parseCreateFile( FILE *file )
{
  parse *p     = NEW( parse );

  p->file      = file;
  p->buf       = array( char, BUFSIZE );
  p->bufptr    = 0;
  p->judge     = array( category, 256 );
  parseInitJudgement( p->judge );
  p->line      = 1;
  
  p->getChar   = parseFileGetCharFunction;
  p->unGetChar = parseStandardUnGetCharFunction;
  p->dispose   = parseStandardDisposeFunction;
  
  return p;
}

parse *parseCreateFileName( const char *name )
{
  parse *p     = NEW( parse );

  p->file      = fopen( name, "r" );
  if ( !p->file )
  {
    free( p );
    return NULL;
  }

  p->buf       = array( char, BUFSIZE );
  p->bufptr    = 0;
  p->judge     = array( category, 256 );
  parseInitJudgement( p->judge );
  p->line      = 1;
  
  p->getChar   = parseFileGetCharFunction;
  p->unGetChar = parseStandardUnGetCharFunction;
  p->dispose   = parseFileDisposeFunction;
  
  return p;
}

parse *parseCreateString( char *contents )
{
  parse *p     = NEW( parse );

  p->contents  = contents;
  p->ptr       = 0;
  p->buf       = array( char, BUFSIZE );
  p->bufptr    = 0;
  p->judge     = array( category, 256 );
  parseInitJudgement( p->judge );
  p->line      = 1;
  
  p->getChar   = parseStringGetCharFunction;
  p->unGetChar = parseStandardUnGetCharFunction;
  p->dispose   = parseStandardDisposeFunction;
  
  return p;
}

void parseSetJudgement( parse *p, short c, category cat )
{
  p->judge[c] = cat;
}

void parseAddJudgement( parse *p, short c, category cat )
{
  p->judge[c] = p->judge[c] | cat;
}

category parseGetJudgement( parse *p, short c )
{
  return p->judge[c];
}

cbool parseError( parse *p, char *s )
{
  char rest[300];
  int i;
  
  for ( i = 0
      ; parseAChar( p, rest+i ) && rest[i] != '\n' && i < 50
      ; i++
      );
  
  rest[i]   = '\n';
  rest[i+1] = '\0';
  
  errorReportExt( ("parse error, line %d: %s\n... %s", p->line, s, rest) );
  return true;
}

cbool parseEnd( parse *p )
{
  char c;
  return ( !parseAChar( p, &c )
        || !parseUnParseChar( p, c )
         );
}

cbool parseAChar( parse *p, char *c )
{
  return p->getChar( p, c );
}

cbool parseAnyChar( parse *p )
{
  char c;
  return parseAChar( p, &c );
}

cbool parseThisChar( parse *p, char c )
{
  char d;
  if ( !p->getChar( p, &d ) )
    return false;
  
  if ( d == c )
    return true;
  
  parseUnParseChar( p, d );
  return false;
}

cbool parseUnParseChar( parse *p, char c )
{
  return p->unGetChar( p, c );
}

cbool parseThisString( parse *p, const char *s )
{
  int i;
  //char c;
  
  for ( i = 0
      ; s[i] != '\0' && parseThisChar( p, s[i] )
      ; i++
      );
  
  if ( s[i] == '\0' )
    return true;
  
  for ( i-- ; i >= 0; i-- )
    parseUnParseChar( p, s[i] );
  
  return false;
}

cbool parseUnParseString( parse *p, char *s )
{
  int i;
  
  for ( i = strlen(s)-1; i >= 0; i-- )
    parseUnParseChar( p, s[i] );
  
  return true;
}

cbool parseSpaceAndComments( parse *p )
{
  while ( 1 )
  {
    if ( parseThisChar( p, ' ' )             
      || parseThisChar( p, '\t' )
      || parseThisChar( p, '\n' )
       )
      ;
    
    else if ( parseThisString( p, "/*" ) )
      while ( !parseThisString( p, "*/" )
           && parseAnyChar( p )
            );
 
    else if ( parseThisString( p, "//" ) )
      while ( !parseThisChar( p, '\n' )
           && parseAnyChar( p )
            );

    else
      break;
  }
          
  return true;
}

cbool parseDigit( parse *p, char *c )
{
  return ( parseAChar( p, c )
        && ( ('0' <= *c && *c <= '9')
          || (parseUnParseChar( p, *c )
	          && false)
           )
         );
}

cbool parseNumber( parse *p, unsigned *n )
{
  unsigned i = 0;
  char s[300];
  
  while ( parseDigit( p, &(s[i]) )
       && assign ( i++ )
        );
  
  return ( i > 0
        && assign( s[i] = '\0' )
        && assign( *n = atoi( s ) )
         );
}

cbool parseSignedNumber( parse *p, int *n ){
	unsigned un;
	return (
		(parseThisChar( p, '-' )
			&& parseLexNumber( p, &un )
			&& assign( *n = -(*n) )
		) || (parseThisChar( p, '+' )
			&& parseLexNumber( p, &un )
		) || (
			parseNumber( p, &un )
		)
	) && assign(*n = un);
}

cbool parseLexNumber( parse *p, unsigned *n ){
	return (
		parseSpaceAndComments( p )
		&& parseNumber( p, n )
	);
}

cbool parseLexSignedNumber( parse *p, int *n )
{
  return ( parseSpaceAndComments( p )
        && parseSignedNumber( p, n )
         );
}

cbool parseCharCategory( parse *p, category cat, char *c )
{
  return ( parseAChar( p, c )
        && ( ( cat & p->judge[(short)*c] )
          || !parseUnParseChar( p, *c )
           )
         );
}

cbool parseLexThisCategory( parse *p, category cat, char *s )
{
  int i;
  char c;
  
  parseSpaceAndComments( p );
  
  for ( i = 0
      ; s[i] != '\0' && parseThisChar( p, s[i] )
      ; i++
      );
  
  if ( s[i] == '\0' )
	if(
		!parseAChar( p, &c )
      	|| (
			parseUnParseChar( p, c )
			&& !( cat & p->judge[(short)c])
		)
    ){
		return true;
	}
  
  for ( i-- ; i >= 0; i-- )
    parseUnParseChar( p, s[i] );
  
  return false;
}

cbool parseLexCategory( parse *p, category cat, char *s )
{
  int i = 0;
  
  parseSpaceAndComments( p );
  
  while ( parseCharCategory( p, cat, s+i )
       && assign ( i++ )
        );
  
  return ( i > 0
        && assign( s[i] = '\0' )
         );
}

cbool parseLexThisString( parse *p, char *s )
{
  return ( parseSpaceAndComments( p )
        && parseThisString( p, s )
         );
}

cbool parseLexThisChar( parse *p, char c )
{
  return ( parseSpaceAndComments( p )
        && parseThisChar( p, c )
         );
}

cbool parseLexEnd( parse *p )
{
  return ( parseSpaceAndComments( p )
        && parseEnd( p )
         );
}

#if 0
cbool parseLexIfKeyword( parse *p, char *s, cbool *b )
{
  return ( parseLexThisKeyword( p, s )
        && assign ( *b = true )
         );
}

cbool parseLexIfNumber( parse *p, unsigned *n )
{
  unsigned m;
  return ( parseLexNumber( p, &m )
        && assign ( *n = m )
         );
}

cbool parseLexIfSignedNumber( parse *p, int *n )
{
  int m;
  return ( parseLexSignedNumber( p, &m )
        && assign ( *n = m )
         );
}

cbool parseLexKeywordNumber( parse *p, char *s, unsigned *n )
{
  return ( parseLexThisKeyword( p, s )
        && ( parseLexNumber( p, n )
          || parseError( p, "no argument given." )
           )
         );
}

cbool parseLexKeywordSignedNumber( parse *p, char *s, int *n )
{
  return ( parseLexThisKeyword( p, s )
        && ( parseLexSignedNumber( p, n )
          || parseError( p, "no argument given." )
           )
         );
}
#endif

cbool parseQuoted( parse *p, char *name )
{
  char c;
  int i = 0;
  
  if ( parseLexThisChar( p, '"' ) )
  {
    while ( parseAChar( p, &c )
         && c != '"'
         && c != '\n'
         && assign ( name[i++] = c )
          );
    if ( c == '"' )
    {
      name[i] = '\0';
      return true;
    }
    else
      parseError( p, "Could not parse quoted string" );
  }
  
  return false;
}

cbool parseLexQuoted( parse *p, char *s )
{
  return ( parseSpaceAndComments( p )
        && parseQuoted( p, s )
         );
}

void parseDispose( parse *p )
{
  p->dispose( p );
}

/*------- john's useful routines --------------*/


cbool parseEOL(parse *p){
	return (
		parseThisChar(p,'\n')
		|| (parseThisChar(p,'\r') && maybe(parseThisChar(p,'\n')))
	);
}

cbool parseWhiteChar(parse *p){
	return (
		parseThisChar(p,' ')
		|| parseThisChar(p,'\t')	
	);
}

cbool parseWS(parse *p){
	return parseWhiteChar(p) && many(parseWhiteChar(p));
}

cbool parseCharIn(parse *p, const char *incl, char *c){
	const char *i;
	if(parseAChar(p,c)){
		for(i=incl; *i!='\0'; ++i){
			if(*c==*i){
				return true;
			}
		}
		parseUnParseChar(p,*c);
		return false;
	}
	return false;
}

cbool parseCharExcept( parse *p, const char *except, char *c ){
	const char *e;
	if(parseAChar( p, c )){
		for(e=except; *e!='\0'; ++e){
			if(*c==*e){
				//fprintf(stderr,"UNPARSING %c (%d)\n",*c,*c);
				parseUnParseChar(p,*c);
				return false;
			}
		}
		return true;
	}
	return false;
}

cbool parseStrExcept(parse *p, const char *except, char *str, int maxl){
	char *c=str;
	int i = 0;
	while(i+1<maxl && parseCharExcept(p,except,c)){
		//fprintf(stderr,"GOT CHAR %c\n",*c);
		i++,c++;
	}
	*c = '\0'; ++i;
	if(i>maxl){
		//fprintf(stderr,"I > MAXL\n");		
		while(--i>0){
			//fprintf(stderr,"unparse '%s'\n",*c);
			parseUnParseChar(p,*(--c));
		}
		//fprintf(stderr,"FAIL\n",*c);
		return fail;
	}
	//fprintf(stderr,"GOT SOME? i=%d\n",i);
	return (i>0);
}		

cbool parseNonWS(parse *p, char *str, int maxl){
	return parseStrExcept(p," \t\n\r\f",str,maxl);
}

cbool parseEOLplus(parse *p){
	return (
		maybe(parseWS(p))
		&& parseEOL(p)
		&& many(maybe(parseWS(p)) && parseEOL(p))
	);
}

cbool parseDouble(parse *p, double *d){
	char word[150];
	int l;
	if(parseNonWS(p,word,150)){
		l = CharactersInDouble(word);
		if(l < strlen(word)){
			parseUnParseString(p,word+l);
		}
		if(l==0){
			return 0;
		}
		word[l] = '\0';
		*d = atof(word);
		return 1;
	}else{
		return 0;
	}
}

/**
	Add a bit to a flag value. If the next char is '1', OR {value} into the
	result value {result}. If the next char is '0', do nothing. Otherwise,
	it's a parsing error.
*/
cbool parseBitChar(parse *p, unsigned value, unsigned *result){
	return (
		parseThisChar(p,'0')
		|| (
			parseThisChar(p,'1')
			&& assign(*result |= value)
		)
	);
}

/* end parse.c */

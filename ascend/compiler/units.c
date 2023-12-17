/*	ASCEND modelling environment
	Copyright (C) 2006, 2011 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Ascend Units Type definitions.
*//*
	by Tom Epperly 13 Sept 1989
	Last in CVS: $Revision: 1.18 $ $Date: 1998/04/11 01:32:11 $ $Author: ballan $
*/

#include <math.h>
#include <ctype.h>
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/hashpjw.h>
#include <ascend/general/dstring.h>

#include "instance_enum.h"
#include "cmpfunc.h"
#include "symtab.h"

#include "dimen_io.h"
#include "units.h"

//#define UNITS_DEBUG
#ifdef UNITS_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

enum units_scanner_tokens {
  units_id,
  units_real,
  units_times,
  units_divide,
  units_power,
  units_open,
  units_close,
  units_end,
  units_err,			/* unexpected character at position */
  units_oversized,		/* oversized */
  units_real_err		/* bad character in real or character */
				/* missing */
};

struct ParseReturn {
  double conv;
  dim_type dim;
};

/* parsing global variables */
char *g_units_str = NULL;
unsigned long g_units_str_len = 0;
char g_units_id_space[MAXTOKENLENGTH+1];

/* hash table global variables */
struct Units *g_units_hash_table[UNITS_HASH_SIZE];
unsigned long g_units_size = 0;
unsigned long g_units_collisions = 0;

static struct ParseReturn CheckNewUnits(CONST char *	
	, unsigned long int *CONST, int *CONST
);

static struct ParseReturn ParseString(CONST char *c
	, unsigned long int *CONST pos, int *CONST error_code
    , int push_close
);


static void CopyToGlobal(register CONST char *c){
  register char *p;
  register unsigned length;
  length = strlen(c);
  if (g_units_str==NULL) {
    g_units_str = ascmalloc((unsigned)length+(unsigned)1);
    g_units_str_len = length;
  }
  else
    if (length > g_units_str_len) {
      g_units_str = ascrealloc(g_units_str,(unsigned)length+(unsigned)1);
      g_units_str_len = length;
    }
  p = g_units_str;
  while((*c) != '\0')
    if (!isspace(*c)) *(p++) = *(c++);
    else c++;
  *p = '\0';
}


static void DefineFundamentalUnit(CONST char *dimname, char *unitname){
  CONST struct Units *uptr;
  dim_type dim, *dimp;

  dimp = &dim;
  ClearDimensions(dimp);
  ParseDim(dimp,dimname);
  dimp = (dim_type *)FindOrAddDimen(dimp);
  uptr = DefineUnits(AddSymbol(unitname),(double)1.0,dimp);
  if (uptr==NULL) {
    FPRINTF(ASCERR,"Unable to define SI unit %s.\n",unitname);
  }
}


static void DefineFundamentalUnits(void){
  DefineFundamentalUnit("M",UNIT_BASE_MASS);
  DefineFundamentalUnit("Q",UNIT_BASE_QUANTITY);
  DefineFundamentalUnit("T", UNIT_BASE_TIME);
  DefineFundamentalUnit("L", UNIT_BASE_LENGTH);
  DefineFundamentalUnit("TMP", UNIT_BASE_TEMPERATURE);
  DefineFundamentalUnit("C", UNIT_BASE_CURRENCY);
  DefineFundamentalUnit("E", UNIT_BASE_ELECTRIC_CURRENT);
  DefineFundamentalUnit("LUM", UNIT_BASE_LUMINOUS_INTENSITY);
  DefineFundamentalUnit("P", UNIT_BASE_PLANE_ANGLE);
  DefineFundamentalUnit("S", UNIT_BASE_SOLID_ANGLE);
}

/* internal translate table of some utility */
static char *g_unit_base_name[NUM_DIMENS];


void InitUnitsTable(void){
  register unsigned long c;
  //register CONST struct Units *result;

  for(c=0;c<UNITS_HASH_SIZE;g_units_hash_table[c++]=NULL);
    /* no body */
  g_units_size = 0;
  g_units_collisions = 0;
  DefineUnits(AddSymbol("?"),1.0,WildDimension());
  DefineUnits(AddSymbol(""),1.0,Dimensionless());
  DefineFundamentalUnits();
  g_unit_base_name[D_MASS] = UNIT_BASE_MASS;
  g_unit_base_name[D_QUANTITY] = UNIT_BASE_QUANTITY;
  g_unit_base_name[D_LENGTH] = UNIT_BASE_LENGTH;
  g_unit_base_name[D_TIME] = UNIT_BASE_TIME;
  g_unit_base_name[D_TEMPERATURE] = UNIT_BASE_TEMPERATURE;
  g_unit_base_name[D_CURRENCY] = UNIT_BASE_CURRENCY;
  g_unit_base_name[D_ELECTRIC_CURRENT] = UNIT_BASE_ELECTRIC_CURRENT;
  g_unit_base_name[D_LUMINOUS_INTENSITY] = UNIT_BASE_LUMINOUS_INTENSITY;
  g_unit_base_name[D_PLANE_ANGLE] = UNIT_BASE_PLANE_ANGLE;
  g_unit_base_name[D_SOLID_ANGLE] = UNIT_BASE_SOLID_ANGLE;
}


void DestroyUnitsTable(void){
  register unsigned long c;
  struct Units *ptr,*next;
  for(c=0;c<UNITS_HASH_SIZE;g_units_hash_table[c++]=NULL){
    next = g_units_hash_table[c];
    while((ptr = next)!=NULL){
      next = ptr->next;
      ascfree(ptr);
    }
  }
  g_units_size = 0;
  g_units_collisions = 0;
  if (g_units_str) ascfree(g_units_str);
  g_units_str = 0;
  g_units_str_len = 0;
}


struct UnitDefinition *CreateUnitDef(symchar *lhs, CONST char *rhs,
		CONST char *filename, int linenum
){
  int len;
  struct UnitDefinition *ud;
  char *ustr;

  if (lhs==NULL || rhs == NULL || filename == NULL) {
    FPRINTF(ASCERR,"  CreateUnitDef miscalled.\n");
    return NULL;
  }
  ud = (struct UnitDefinition *)ascmalloc(sizeof(struct UnitDefinition));
  if (ud == NULL) {
    FPRINTF(ASCERR,"  malloc failed in CreateUnitDef for %s: %s %d\n",
      SCP(lhs),filename,linenum);
    return NULL;
  }
  len = strlen(rhs) + 1;
  ustr = ASC_NEW_ARRAY(char,len);
  if (ustr == NULL) {
    FPRINTF(ASCERR,"  malloc failed in CreateUnitDef for %s: %s %d\n",
      rhs,filename,linenum);
    ascfree(ud);
    return NULL;
  }
  strcpy(ustr,rhs);

  ud->new_name = lhs;
  ud->unitsexpr = ustr;
  ud->filename = filename;
  ud->linenum = linenum;
  return ud;
}


void DestroyUnitDef(struct UnitDefinition *ud){
  if (ud==NULL) {
    return;
  }
  ascfree((char *)ud->unitsexpr);
  ud->new_name = NULL;
  ud->unitsexpr = ud->filename = NULL;
  ascfree((char *)ud);
}


void ProcessUnitDef(struct UnitDefinition *ud){
  CONST struct Units *result;
  struct ParseReturn pr;
  unsigned long pos;
  char **errv;
  int code;

  if (ud==NULL) {
    return;
  }
  pr = CheckNewUnits(ud->unitsexpr,&pos,&code);
  if (code!=0) {
    errv = UnitsExplainError(ud->unitsexpr,code,pos);
    FPRINTF(ASCERR,"ERROR checking: %s.\n", errv[0]);
    FPRINTF(ASCERR,"  %s =\n",SCP(ud->new_name));
    FPRINTF(ASCERR,"  {%s};\n",errv[1]);
    FPRINTF(ASCERR,"  -%s\n",errv[2]);
    FPRINTF(ASCERR,"  %s:%d\n\n",ud->filename,ud->linenum);
	errv = UnitsExplainError(NULL,-1,0);
    return;
  }
  result = DefineUnits(ud->new_name,pr.conv,FindOrAddDimen(&pr.dim));
  if (result == NULL) {
    errv = UnitsExplainError(ud->unitsexpr,11,0);
    FPRINTF(ASCERR,"ERROR defining units: %s.\n", errv[0]);
    FPRINTF(ASCERR,"  %s =\n",SCP(ud->new_name));
    FPRINTF(ASCERR,"  {%s};\n",errv[1]);
    FPRINTF(ASCERR,"  -%s\n",errv[2]);
    FPRINTF(ASCERR,"  %s:%d\n\n",ud->filename,ud->linenum);
	errv = UnitsExplainError(NULL,-1,0);
    return;
  }
}

/*
 * it is not appropriate to replace this with a pointer hashing
 * function since the string hashed may not be a symchar.
 */
#define UnitsHashFunction(s) hashpjw(s,UNITS_HASH_SIZE)


CONST struct Units *LookupUnits(CONST char *c){
  register struct Units *result;
  register int str_cmp=1;
  if((result=g_units_hash_table[UnitsHashFunction(c)])!=NULL) {
    while(((str_cmp=strcmp(SCP(UnitsDescription(result)),c))<0) &&
	  (result->next != NULL))
      result = result->next;
    if (str_cmp==0) return result;
  }
  return NULL;
}


static struct Units *CheckUnitsMatch(struct Units *p
	, double conv, CONST dim_type *dim
){
  if((conv!=UnitsConvFactor(p))||(!SameDimen(dim,UnitsDimensions(p)))) {
    return NULL;
  }else{
    return p;
  }
}


CONST struct Units *DefineUnits(symchar *c
	, double conv, CONST dim_type *dim
){
  register unsigned long bucket;
  register struct Units *result,*tmp;
  register int str_cmp;
  assert(AscFindSymbol(c)!=NULL);
  bucket=UnitsHashFunction(SCP(c));
  if(g_units_hash_table[bucket]!=NULL){
    result=g_units_hash_table[bucket];
    str_cmp = CmpSymchar(c,UnitsDescription(result));
    if(str_cmp==0){
      return CheckUnitsMatch(result,conv,dim);
    }else if (str_cmp<0){
      /* insert before list head */
      g_units_hash_table[bucket]=
        (struct Units *)ascmalloc(sizeof(struct Units));
      g_units_hash_table[bucket]->next = result;
      result = g_units_hash_table[bucket];
    }else{
      while((result->next!=NULL)&&
	     ((str_cmp=CmpSymchar(c,UnitsDescription(result->next)))>0))
        result = result->next;
      if(str_cmp==0) return CheckUnitsMatch(result->next,conv,dim);
      tmp = result->next;
      result->next = (struct Units *)ascmalloc(sizeof(struct Units));
      result = result->next;
      result->next = tmp;
    }
    g_units_size++;
    g_units_collisions++;
    result->description = c;
    result->conversion_factor = conv;
    result->dim = dim;
  }else{
    /* empty bucket */
    g_units_size++;
    result = g_units_hash_table[bucket] =
      (struct Units *)ascmalloc(sizeof(struct Units));
    result->next = NULL;
    result->description = c;
    result->dim = dim;
    result->conversion_factor = conv;
  }
  return result;
}


static void SkipStrBlanks(CONST char *c, unsigned long int *CONST pos){
  while(isspace(c[*pos])) (*pos)++;
}


static int AddChar(register char ch, register unsigned int pos){
  if (pos < MAXTOKENLENGTH) {
    g_units_id_space[pos]=ch;
    return 1;
  }
  g_units_id_space[MAXTOKENLENGTH] = '\0';
  return 0;
}


static enum units_scanner_tokens GetUnitsToken(CONST char *c
	, unsigned long int *CONST pos
){
  register unsigned cc;
  SkipStrBlanks(c,pos);

#define ADD_CHAR_S \
	if(AddChar(c[*pos],cc++)) (*pos)++; \
	else return units_oversized;
#define ADD_CHAR_WHILE(COND) \
	do{ ADD_CHAR_S; } while(COND);
#define COMPLETE(TYPE) \
    g_units_id_space[cc]='\0'; \
    return TYPE;

  if(isalpha(c[*pos])){ /* an identifier, starting with an alpha char */
    cc = 0;
    ADD_CHAR_WHILE(isalpha(c[*pos])||(isdigit(c[*pos]))||(c[*pos]=='_'));
    COMPLETE(units_id);
  }
  else if(isdigit(c[*pos])){ /* a real or integer value */
    cc = 0;
    ADD_CHAR_WHILE(isdigit(c[*pos]));
    if(c[*pos] == '.'){
      ADD_CHAR_S;
      while (isdigit(c[*pos])){
        ADD_CHAR_S;
      }
    }
    if((c[*pos] == 'e')||(c[*pos] == 'E')){
      ADD_CHAR_S;
      if((c[*pos] == '+')||(c[*pos] == '-')){
        ADD_CHAR_S;
      }
      if(isdigit(c[*pos])){
        ADD_CHAR_WHILE(isdigit(c[*pos]));
      }else{
        COMPLETE(units_real_err);
      }
    }
	COMPLETE(units_real);
  }else switch(c[*pos]){
    case '.': /* real */
      cc = 0;
      ADD_CHAR_S;
      if(isdigit(c[*pos])) {
        ADD_CHAR_WHILE(isdigit(c[*pos]));
      }else{
		COMPLETE(units_real_err);
      }
      if((c[*pos] == 'e')||(c[*pos] == 'E')) {
        ADD_CHAR_S;
        if((c[*pos] == '+')||(c[*pos] == '-')) {
          ADD_CHAR_S;
        }
        if(isdigit(c[*pos])){
          ADD_CHAR_WHILE(isdigit(c[*pos]));
        }else{
		  COMPLETE(units_real_err);
        }
      }
      COMPLETE(units_real);
    case '^':
      (*pos)++;
      return units_power;
    case '*':
      (*pos)++;
      return units_times;
    case '/':
      (*pos)++;
      return units_divide;
    case '(':
      (*pos)++;
      return units_open;
    case ')':
      (*pos)++;
      return units_close;
    case '\0':
      return units_end;
    default:
      return units_err;
    }
#undef ADD_CHAR_S
#undef ADD_CHAR_WHILE
#undef COMPLETE
}


static
double AdjustConv(double d, struct fraction f, int *CONST error_code){
  f = Simplify(f);
  if(Numerator(f)<0){
    if (Denominator(f)!=1){
      MSG("Negative fractional exponent!");
      *error_code = 12;
      return 0.0;
    }
    return 1.0/pow(d,-(double)Numerator(f));
  }else
    return pow(d,(double)Numerator(f)/(double)Denominator(f));
}


static
FRACPART ParseInt(CONST char *c,
	unsigned long int *CONST pos, int *CONST error_code
){
  register unsigned count=0;
  SkipStrBlanks(c,pos);
  if((c[*pos]=='-')||(c[*pos]=='+'))
    g_units_id_space[count++]=c[(*pos)++];
  if(!isdigit(c[*pos])){
    *error_code = 10;
    return 1;
  }
  while(isdigit(c[*pos])){
    if(count < MAXTOKENLENGTH){
      g_units_id_space[count++]=c[(*pos)++];
    }else{
      *error_code = 10;
      return 1;
    }
  }
  g_units_id_space[count]='\0';
  return (FRACPART)atoi(g_units_id_space);
}


static 
struct fraction ParseFraction(CONST char *c,
	unsigned long int *CONST pos, int *CONST error_code
){
  register FRACPART num,denom;
  SkipStrBlanks(c,pos);
  if(c[*pos]=='('){
    MSG("Got '('");
    (*pos)++;
    MSG("Parsing denominator '%s'",c+*pos);
    num = ParseInt(c,pos,error_code);
    //MSG("After ParseInt, parsing '%s'",c+*pos);
    if(*error_code == 0){
      MSG("Numerator '%hd', now parsing '%s'",num,c+*pos);
      SkipStrBlanks(c,pos);
      if(c[*pos] == '/') {
        (*pos)++;
        MSG("Got '/', now parsing '%s'",c+*pos);
        denom = ParseInt(c,pos,error_code);
        if(*error_code == 0){
          MSG("Got denominator '%hd', now parsing '%s'",denom,c+*pos);
          SkipStrBlanks(c,pos);
          if(c[*pos] == ')') {
            (*pos)++;
            MSG("Got ')', returning fraction %hd/%hd",num,denom);
            return CreateFraction(num,denom);
          }else{ /* unclosed parenthesis */
            MSG("Unclosed paren");
            *error_code = 13;
            return CreateFraction(1,1);
          }
        }
        MSG("Failed parsing denominator");
      }else{
        MSG("Failed parsing '/'");
        if(c[*pos] == ')'){ /* okay */
          (*pos)++;
          MSG("OK, found ')', returning fraction %hd/1",num);
          return CreateFraction(num,1);
        }else{ /* error unclosed parenthesis */
          MSG("No good, no closing parenthesis");
          *error_code = 13;
          return CreateFraction(1,1);
        }
      }
    }
    MSG("Didn't parse an integer! error code = %d",*error_code);
  }else if(isdigit(c[*pos])||(c[*pos]=='+')||(c[*pos]=='-')){
    return CreateFraction(ParseInt(c,pos,error_code),1);
  }
  MSG("ParseFraction didn't like what it found");
  *error_code = 10;
  return CreateFraction(1,1);
}


static
struct ParseReturn ParseTerm(CONST char *c,
	unsigned long int *CONST pos, int *CONST error_code
){
  register CONST struct Units *lookup;
  struct fraction frac;
  //enum units_scanner_tokens tok;
  struct ParseReturn result;
  unsigned long oldpos;
  result.conv = 1.0;
  ClearDimensions(&(result.dim));
  MSG("Parsing term '%s'",c+*pos);
  SkipStrBlanks(c,pos);
  //MSG("After skipping blanks, pos %lu (char '%c')",*pos,c[*pos]);
  oldpos = *pos;
  switch(GetUnitsToken(c,pos)){
  case units_id:
    MSG("Found identifier '%s' at pos %lu",g_units_id_space,*pos);
    lookup = LookupUnits(g_units_id_space);
    if(lookup!=NULL) {
      CopyDimensions(UnitsDimensions(lookup),&result.dim);
      result.conv = UnitsConvFactor(lookup);
    }else{
      *pos = oldpos;
      *error_code = 1;
      return result;
    }
    break;
  case units_real:
    MSG("Found real '%s' at pos %lu",g_units_id_space,*pos);
    result.conv = atof(g_units_id_space);
    break;
  case units_open:
    MSG("units_open, parse '%s'",c+*pos);
    result = ParseString(c,pos,error_code,1);
    MSG("units_open, got back with '%s'", c+*pos);
    if(*error_code == 0){
      if(GetUnitsToken(c,pos)!=units_close) {/* unbalanced parenthesis */
        *error_code = 2;
        *pos = oldpos;
        return result;
      }
    }else{
      return result;
    }
    break;
  case units_err:
    *error_code = 3;
    return result;
  case units_end:
    *error_code = 7;
    return result;
  case units_oversized:
    *error_code = 5;
    *pos = oldpos;
    return result;
  case units_real_err:
    *error_code = 4;
    *pos = oldpos;
    return result;
  case units_divide:
  case units_power:
  case units_times:
    MSG("Found divide/times/power at pos %lu (error)",*pos);
    *pos = oldpos;
    *error_code = 8;
    return result;
  case units_close:
    MSG("Found closing paren at pos %lu (error)",*pos);
    *pos = oldpos;
    *error_code = 9;
    return result;
  }
  SkipStrBlanks(c,pos);
  if (c[*pos]=='^') {
    GetUnitsToken(c,pos);
    SkipStrBlanks(c,pos);
    oldpos = *pos;
    frac = ParseFraction(c,pos,error_code);
    if (*error_code==0) {
      result.dim = ScaleDimensions(&result.dim,frac);
      result.conv = AdjustConv(result.conv,frac,error_code);
    }
    if (*error_code!=0) *pos = oldpos;
  }
  return result;
}


static
struct ParseReturn MultiplyPR(CONST struct ParseReturn *r1,
		CONST struct ParseReturn *r2
){
  struct ParseReturn result;
  result.conv = r1->conv*r2->conv;
  result.dim = AddDimensions(&(r1->dim),&(r2->dim));
  return result;
}


static
struct ParseReturn DividePR(CONST struct ParseReturn *r1
    ,CONST struct ParseReturn *r2)
{
  struct ParseReturn result;
  result.conv = r1->conv/r2->conv;
  result.dim = SubDimensions(&(r1->dim),&(r2->dim));
  return result;
}


/**
  push_close: if a final closing parenthesis is found, don't swallow it
*/
static
struct ParseReturn ParseString(CONST char *c
    ,unsigned long int *CONST pos, int *CONST error_code
    ,int push_close
){
  struct ParseReturn result1,result2;
  unsigned long oldpos;
  MSG("Parsing string '%s'",c+*pos);
  result1 = ParseTerm(c,pos,error_code);
  while(*error_code == 0){
    SkipStrBlanks(c,pos);	
    oldpos = *pos;
    switch(GetUnitsToken(c,pos)){
    case units_oversized:
    case units_id:
    case units_real_err:
    case units_real:
    case units_open:
	  MSG("Found id, real or open at pos %lu (error)",*pos);
      *pos = oldpos;
      *error_code = 6;
      return result1;
    case units_times:
	  MSG("Found '*' pos %lu",*pos);
      result2 = ParseTerm(c,pos,error_code);
      if(*error_code==0){
        result1 = MultiplyPR(&result1,&result2);
      }
      break;
    case units_divide:
	  MSG("Found '/' pos %lu",*pos);
      result2 = ParseTerm(c,pos,error_code);
      if(*error_code==0){
        result1 = DividePR(&result1,&result2);
      }
      break;
    case units_close: /* closing parenthesis */
      MSG("Found ')' pos %lu",*pos);
	  if(push_close){
        (*pos)--; /* put the closing bracket back */
        return result1;
      }else{
        /* we weren't expecting to see a closing parenthesis */
        *error_code = 9;
        return result1;
      }
    case units_end: /* natural closings */
      return result1;
    case units_err:
      *error_code = 3;
      return result1;
    default:
      /* units power? */
      break;
    }
  }
  return result1;
}


/*
 * Checks the RHS of a new unit definition.
 * returns valid parsereturn iff *error_code = 0 on exit.
 * This has no effects
 * on the global unit table, so it returns a ParseReturn
 * instead of a units pointer.
 * If unit already exists and the old and new definitions
 * are incompatible, returns error.
 */
static
struct ParseReturn CheckNewUnits(CONST char *c
    ,unsigned long int *CONST pos, int *CONST error_code
){
  struct ParseReturn preturn;
  register CONST struct Units *result;

  /* initialize return codes */
  *pos = 0;
  *error_code = 0;
  /* copy string to global string while removing blanks */
  CopyToGlobal(c);
  /* check if units are previously defined */
  result = LookupUnits(g_units_str);
  if (result != NULL) {
    preturn.conv = UnitsConvFactor(result);
    preturn.dim = *(UnitsDimensions(result));
    return preturn;
  }
  /* it couldn't find a match, so the string must be parsed */
  preturn = ParseString(c,pos,error_code, 0);
  return preturn;
}


CONST struct Units *FindOrDefineUnits(CONST char *c,
		unsigned long int *CONST pos, int *CONST error_code
){
  register CONST struct Units *result;
  struct ParseReturn preturn;

  /* initialize return codes */
  *pos = 0;
  *error_code = 0;
  /* copy string to global string while removing blanks */
  CopyToGlobal(c);
  /* check if units are previously defined */
  result = LookupUnits(g_units_str);
  if (result != NULL) {
    return result;
  }
  /* it couldn't find a match, so the string must be parsed */
  preturn = ParseString(c,pos,error_code, 0);
  if (*error_code == 0) {
    result = DefineUnits(AddSymbol(g_units_str),
			 preturn.conv,
			 FindOrAddDimen(&preturn.dim));
  }
  return result;
}

/*
  Note that the use of GetDimPower in this function implies some
  'approximation' to the true dimensions recorded. Only integer powers
  get reported, so something like y = sqrt(T) would end up with units of [1].
*/
char *UnitsStringSI(const struct Units *p){
  Asc_DString ds, *dsPtr;
  char expo[20];
  char *result;
  int numseen = 0;
  int i;
  int k;

  if(p==NULL){
    return NULL;
  }
  if(IsWild(p->dim)) {
    result = ASC_NEW_ARRAY(char,2);
    sprintf(result,"*");
    return result;
  }
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  for(i=0; i < NUM_DIMENS; i++) {
    k = GetDimPower(*(p->dim),i);
    if (k > 0) {
      if (numseen) {
        Asc_DStringAppend(dsPtr,"*",1);
      }
      Asc_DStringAppend(dsPtr,g_unit_base_name[i],-1);
      if (k > 1) {
        sprintf(expo,"^%d",k);
        Asc_DStringAppend(dsPtr,expo,-1);
      }
      numseen =1;
    }
  }
  if(!numseen) {
    Asc_DStringAppend(dsPtr,"1",1);
  }
  for (i=0; i < NUM_DIMENS; i++) {
    k = GetDimPower(*(p->dim),i);
    if (k < 0) {
      Asc_DStringAppend(dsPtr,"/",1);
      Asc_DStringAppend(dsPtr,g_unit_base_name[i],-1);
      if (k < -1) {
        sprintf(expo,"^%d",-k);
        Asc_DStringAppend(dsPtr,expo,-1);
      }
    }
  }
  result = Asc_DStringResult(dsPtr);
  return result;
}


void DumpUnits(FILE *file){
  register unsigned long c;
  register struct Units *p;
  char *ds;
  FPRINTF(file,"Units dump\n");
  for(c=0;c<UNITS_HASH_SIZE;c++) {
    for(p = g_units_hash_table[c];p!=NULL;p=p->next) {
      ds = WriteDimensionString(p->dim);
      FPRINTF(file,"%35s %14g %s\n",
	      SCP(p->description),
	      p->conversion_factor,ds);
      if (ds != NULL) {
        ascfree(ds);
      }
    }
  }
}


static
char *g_unit_explain_error_strings[3] = {NULL,NULL,NULL};
#define ERRV g_unit_explain_error_strings


char **UnitsExplainError(CONST char *ustr, int code, int pos){
  static char *g_units_errors[] = {
    /*0*/"unit ok"
    ,"undefined unit in expression"
    ,"unbalanced ( or () in denominator"
    ,"illegal character"
    ,"illegal real value"
    ,/*5*/"unit name too long"
    ,"operator ( * or / ) missing"
    ,"term missing after *,/, or ("
    ,"term missing before * or /"
    ,"too many )"
    ,/*10*/"illegal fractional exponent"
    ,"redefinition of unit"
    ,"illegal negative fractional exponent"
    ,"closing ) missing" /*UEELAST*/
    ,/*UEECALL*/"error in call to UnitsExplainError" /* keep these two last */
    ,/*UEEMEM*/"malloc fail in UnitsExplainError"
  };
#define UEESIZE (sizeof(g_units_errors)/sizeof(char *))
#define UEELAST (UEESIZE-3) /* last real message */
#define UEECALL (UEESIZE-2)
#define UEEMEM (UEESIZE-1)
  int c,len;
  char *line;

  if(ERRV[2] != g_units_errors[UEECALL] &&
      ERRV[2] != g_units_errors[UEEMEM] &&
      ERRV[2] != NULL
  ){
	/* clean up the memory allocated to the line indicator on the last call */
    ascfree(ERRV[2]);
    ERRV[2] = NULL;
  }

  if(code<0 || code>UEELAST || ustr==NULL){
    ERRV[0] = g_units_errors[UEECALL];
    ERRV[1] = g_units_errors[UEECALL];
    ERRV[2] = g_units_errors[UEECALL];
    return ERRV;
  }
  len = strlen(ustr);
  if(pos<0 || pos>=len){
    ERRV[0] = g_units_errors[UEECALL];
    ERRV[1] = g_units_errors[UEECALL];
    ERRV[2] = g_units_errors[UEECALL];
    return ERRV;
  }
  line = ASC_NEW_ARRAY_CLEAR(char,len+2);
  if(line==NULL){
    ERRV[0] = g_units_errors[UEEMEM];
    ERRV[1] = g_units_errors[UEEMEM];
    ERRV[2] = g_units_errors[UEEMEM];
    return ERRV;
  }
  //MSG("error %d = '%s'",code,g_units_errors[code]);
  ERRV[0] = g_units_errors[code];
  ERRV[1] = (char *)ustr;
  ERRV[2] = line;
  c = 0;
  while(c < pos){
    line[c] = '-';
    c++;
  }
  line[c] = '^';
  c++;
  line[c] = '\0';

  return ERRV;
}

/* vim: set noai ts=4 sw=2 et: */

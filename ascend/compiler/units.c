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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/hashpjw.h>
#include <ascend/general/dstring.h>
#include <ascend/utilities/error.h>

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
static long g_units_next_ladder_id = 0;

static struct ParseReturn CheckNewUnits(CONST char *	
	, unsigned long int *CONST, int *CONST
);

static struct ParseReturn ParseString(CONST char *c
	, unsigned long int *CONST pos, int *CONST error_code
    , int push_close
);


static void CopyToGlobal(CONST char *c){
  char *p;
  unsigned length;
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
  unsigned long c;
  //CONST struct Units *result;

  for(c=0;c<UNITS_HASH_SIZE;g_units_hash_table[c++]=NULL);
    /* no body */
  g_units_size = 0;
  g_units_collisions = 0;
  g_units_next_ladder_id = 0;
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
  unsigned long c;
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
  g_units_next_ladder_id = 0;
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

struct UnitLadderItem *CreateUnitLadderItem(symchar *name,
		CONST char *unitsexpr, int is_anchor,
		CONST char *filename, int linenum
){
	struct UnitLadderItem *item;
	char *ustr = NULL;
	int len;
	if (name == NULL || filename == NULL) {
		FPRINTF(ASCERR,"  CreateUnitLadderItem miscalled.\n");
		return NULL;
	}
	if (is_anchor && unitsexpr != NULL) {
		FPRINTF(ASCERR,"  Anchor ladder item cannot have a units expression.\n");
		return NULL;
	}
	if (!is_anchor && unitsexpr == NULL) {
		FPRINTF(ASCERR,"  Definition ladder item requires a units expression.\n");
		return NULL;
	}
	item = ASC_NEW(struct UnitLadderItem);
	if (item == NULL) {
		FPRINTF(ASCERR,"  malloc failed in CreateUnitLadderItem for %s: %s %d\n",
			SCP(name),filename,linenum);
		return NULL;
	}
	if (!is_anchor) {
		len = strlen(unitsexpr) + 1;
		ustr = ASC_NEW_ARRAY(char,len);
		if (ustr == NULL) {
			FPRINTF(ASCERR,"  malloc failed in CreateUnitLadderItem for %s: %s %d\n",
				unitsexpr,filename,linenum);
			ascfree(item);
			return NULL;
		}
		strcpy(ustr,unitsexpr);
	}
	item->name = name;
	item->unitsexpr = ustr;
	item->filename = filename;
	item->linenum = linenum;
	item->is_anchor = !!is_anchor;
	return item;
}

void DestroyUnitLadderItem(struct UnitLadderItem *item){
	if (item == NULL) {
		return;
	}
	if (item->unitsexpr != NULL) {
		ascfree((char *)item->unitsexpr);
	}
	item->name = NULL;
	item->unitsexpr = NULL;
	item->filename = NULL;
	ascfree((char *)item);
}

static int UnitsLadderError(CONST char *filename, int linenum, CONST char *fmt, ...){
	char msg[1024];
	va_list args;
	va_start(args,fmt);
	vsnprintf(msg,sizeof(msg),fmt,args);
	va_end(args);
	error_reporter(ASC_USER_ERROR,filename,linenum,NULL,"%s",msg);
	return 1;
}

static void UnitsShiftRanks(long ladder_id, long start_rank, long delta){
	unsigned long c;
	struct Units *p;
	if (delta <= 0) {
		return;
	}
	for (c = 0; c < UNITS_HASH_SIZE; ++c) {
		for (p = g_units_hash_table[c]; p != NULL; p = p->next) {
			if (p->ladder_id == ladder_id && p->ladder_rank >= start_rank) {
				p->ladder_rank += delta;
			}
		}
	}
}

int ProcessUnitLadder(struct gl_list_t *items){
	unsigned long i, len;
	long ladder_id = -1;
	long insert_rank = 0;
	int errors = 0;
	struct UnitLadderItem *item;
	CONST dim_type *ladder_dim = NULL;
	const struct Units *uconst;
	struct Units *u;
	struct gl_list_t *new_units;

	if (items == NULL) {
		return UnitsLadderError(NULL,0,"UNITS LADDER item list is NULL.");
	}
	len = gl_length(items);
	if (len == 0) {
		return UnitsLadderError(NULL,0,"UNITS LADDER must contain at least one item.");
	}

	new_units = gl_create(len);
	item = (struct UnitLadderItem *)gl_fetch(items,1);
	if (item != NULL && item->is_anchor) {
		uconst = LookupUnits(SCP(item->name));
		if (uconst == NULL) {
			errors += UnitsLadderError(item->filename,item->linenum,
				"UNITS LADDER anchor '%s' is not defined.",SCP(item->name));
			gl_destroy(new_units);
			return errors;
		}
		if (UnitsLadderId(uconst) < 0) {
			errors += UnitsLadderError(item->filename,item->linenum,
				"UNITS LADDER anchor '%s' is not a member of any ladder."
				,SCP(item->name)
			);
			gl_destroy(new_units);
			return errors;
		}
		ladder_id = UnitsLadderId(uconst);
		insert_rank = UnitsLadderRank(uconst) + 1;
		ladder_dim = UnitsDimensions(uconst);
	}else{
		ladder_id = g_units_next_ladder_id++;
	}

	for (i = 1; i <= len; ++i) {
		item = (struct UnitLadderItem *)gl_fetch(items,i);
		if (item == NULL) {
			errors += UnitsLadderError(NULL,0,"NULL item in UNITS LADDER.");
			continue;
		}
		if (item->is_anchor) {
			if (i != 1) {
				errors += UnitsLadderError(item->filename,item->linenum,
					"UNITS LADDER anchor '%s' is only allowed as first item."
					,SCP(item->name)
				);
			}
			continue;
		}

		{
			struct UnitDefinition *ud = CreateUnitDef(item->name,item->unitsexpr,item->filename,item->linenum);
			if (ud == NULL) {
				errors += UnitsLadderError(item->filename,item->linenum,
					"Failed to create unit definition for '%s'.",SCP(item->name));
				continue;
			}
			ProcessUnitDef(ud);
			DestroyUnitDef(ud);
		}

		uconst = LookupUnits(SCP(item->name));
		if (uconst == NULL) {
			errors += UnitsLadderError(item->filename,item->linenum,
				"Units '%s' could not be defined for UNITS LADDER.",SCP(item->name));
			continue;
		}
		u = (struct Units *)uconst; /* storage is mutable, API is const-qualified */
		if (UnitsLadderId(u) >= 0) {
			errors += UnitsLadderError(item->filename,item->linenum,
				"Units '%s' already belongs to ladder %ld."
				,SCP(item->name),UnitsLadderId(u)
			);
			continue;
		}
		if (ladder_dim == NULL) {
			ladder_dim = UnitsDimensions(u);
		}else if (!SameDimen(ladder_dim,UnitsDimensions(u))) {
			errors += UnitsLadderError(item->filename,item->linenum,
				"Units '%s' has dimensions incompatible with this UNITS LADDER."
				,SCP(item->name)
			);
			continue;
		}
		gl_append_ptr(new_units,(char *)u);
	}

	len = gl_length(new_units);
	if (len > 0) {
		if (insert_rank < 0) {
			insert_rank = 0;
		}
		UnitsShiftRanks(ladder_id,insert_rank,(long)len);
		for (i = 1; i <= len; ++i) {
			u = (struct Units *)gl_fetch(new_units,i);
			u->ladder_id = ladder_id;
			u->ladder_rank = insert_rank + (long)(i - 1);
		}
	}
	gl_destroy(new_units);
	return errors;
}

/*
 * it is not appropriate to replace this with a pointer hashing
 * function since the string hashed may not be a symchar.
 */
#define UnitsHashFunction(s) hashpjw(s,UNITS_HASH_SIZE)


CONST struct Units *LookupUnits(CONST char *c){
  struct Units *result;
  int str_cmp=1;
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
  unsigned long bucket;
  struct Units *result,*tmp;
  int str_cmp;
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
    result->ladder_id = -1;
    result->ladder_rank = -1;
  }else{
    /* empty bucket */
    g_units_size++;
    result = g_units_hash_table[bucket] =
      (struct Units *)ascmalloc(sizeof(struct Units));
    result->next = NULL;
    result->description = c;
    result->dim = dim;
    result->conversion_factor = conv;
    result->ladder_id = -1;
    result->ladder_rank = -1;
  }
  return result;
}

CONST struct Units *LookupUnitsByLadder(long ladder_id, long ladder_rank){
	unsigned long c;
	struct Units *p;
	for (c = 0; c < UNITS_HASH_SIZE; ++c) {
		for (p = g_units_hash_table[c]; p != NULL; p = p->next) {
			if (p->ladder_id == ladder_id && p->ladder_rank == ladder_rank) {
				return p;
			}
		}
	}
	return NULL;
}


static void SkipStrBlanks(CONST char *c, unsigned long int *CONST pos){
  while(isspace(c[*pos])) (*pos)++;
}


static int AddChar(char ch, unsigned int pos){
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
  unsigned cc;
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
  unsigned count=0;
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
  FRACPART num,denom;
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
  CONST struct Units *lookup;
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
  CONST struct Units *result;

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
  CONST struct Units *result;
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
  unsigned long c;
  struct Units *p;
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

/*----------------------------- units overrides -----------------------------*/

#define UOVR_BUCKETS 257

struct UnitsOverrideEntry{
	char *scope;
	char *name;
	char *units;
	const struct Units *u;
	struct UnitsOverrideEntry *next;
};

struct UnitsOverridesDB{
	struct UnitsOverrideEntry *type_buckets[UOVR_BUCKETS];
	struct UnitsOverrideEntry *name_buckets[UOVR_BUCKETS];
	char *simroot;
	unsigned dirty;
};

static char *uovr_strdup(CONST char *s){
	size_t n;
	char *r;
	if (s == NULL) {
		return NULL;
	}
	n = strlen(s) + 1;
	r = ASC_NEW_ARRAY(char,n);
	if (r == NULL) {
		return NULL;
	}
	memcpy(r,s,n);
	return r;
}

static unsigned long uovr_hash_text(CONST char *s, int fold_case){
	unsigned long h = 0;
	unsigned char ch;
	if (s == NULL) {
		s = "";
	}
	while ((ch = (unsigned char)*s++) != '\0') {
#ifdef _WIN32
		if (ch == '\\') {
			ch = '/';
		}
		if (fold_case) {
			ch = (unsigned char)tolower(ch);
		}
#else
		(void)fold_case;
#endif
		h = ((h * 131UL) + (unsigned long)ch) % UOVR_BUCKETS;
	}
	return h;
}

static int uovr_scope_eq(CONST char *a, CONST char *b){
	if (a == NULL) {
		a = "";
	}
	if (b == NULL) {
		b = "";
	}
#ifdef _WIN32
	unsigned char ca, cb;
	while (*a != '\0' && *b != '\0') {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		if (ca == '\\') {
			ca = '/';
		}
		if (cb == '\\') {
			cb = '/';
		}
		ca = (unsigned char)tolower(ca);
		cb = (unsigned char)tolower(cb);
		if (ca != cb) {
			return 0;
		}
	}
	return *a == '\0' && *b == '\0';
#else
	return strcmp(a,b) == 0;
#endif
}

static unsigned long uovr_hash(CONST char *scope, CONST char *name){
	unsigned long h1 = uovr_hash_text(scope,1);
	unsigned long h2 = uovr_hash_text(name,0);
	return (h1 * 33UL + h2) % UOVR_BUCKETS;
}

static struct UnitsOverrideEntry **uovr_bucket_head(
	struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
){
	unsigned long h;
	if (db == NULL) {
		return NULL;
	}
	h = uovr_hash(scope,name);
	switch (kind) {
	case UNITS_OVERRIDE_TYPE:
		return &db->type_buckets[h];
	case UNITS_OVERRIDE_NAME:
		return &db->name_buckets[h];
	default:
		return NULL;
	}
}

static struct UnitsOverrideEntry **uovr_find_ref(
	struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
){
	struct UnitsOverrideEntry **head;
	struct UnitsOverrideEntry **ref;
	CONST char *scope0 = (scope != NULL) ? scope : "";
	if (name == NULL) {
		return NULL;
	}
	head = uovr_bucket_head(db,kind,scope0,name);
	if (head == NULL) {
		return NULL;
	}
	ref = head;
	while (*ref != NULL) {
		if (uovr_scope_eq((*ref)->scope,scope0) && strcmp((*ref)->name,name) == 0) {
			return ref;
		}
		ref = &(*ref)->next;
	}
	return ref;
}

static void uovr_free_entry(struct UnitsOverrideEntry *e){
	if (e == NULL) {
		return;
	}
	if (e->scope != NULL) {
		ascfree(e->scope);
	}
	if (e->name != NULL) {
		ascfree(e->name);
	}
	if (e->units != NULL) {
		ascfree(e->units);
	}
	ascfree(e);
}

static void uovr_clear_buckets(struct UnitsOverrideEntry **buckets){
	unsigned long i;
	if (buckets == NULL) {
		return;
	}
	for (i = 0; i < UOVR_BUCKETS; ++i) {
		struct UnitsOverrideEntry *e = buckets[i];
		while (e != NULL) {
			struct UnitsOverrideEntry *n = e->next;
			uovr_free_entry(e);
			e = n;
		}
		buckets[i] = NULL;
	}
}

struct UnitsOverridesDB *UnitsOverridesCreate(void){
	struct UnitsOverridesDB *db = ASC_NEW_CLEAR(struct UnitsOverridesDB);
	return db;
}

void UnitsOverridesDestroy(struct UnitsOverridesDB *db){
	if (db == NULL) {
		return;
	}
	uovr_clear_buckets(db->type_buckets);
	uovr_clear_buckets(db->name_buckets);
	if (db->simroot != NULL) {
		ascfree(db->simroot);
		db->simroot = NULL;
	}
	ascfree(db);
}

void UnitsOverridesClear(struct UnitsOverridesDB *db){
	if (db == NULL) {
		return;
	}
	uovr_clear_buckets(db->type_buckets);
	uovr_clear_buckets(db->name_buckets);
	db->dirty = 1;
}

int UnitsOverridesSetSimroot(
	struct UnitsOverridesDB *db,
	CONST char *simroot
){
	if (db == NULL) {
		return 1;
	}
	if (db->simroot != NULL) {
		ascfree(db->simroot);
		db->simroot = NULL;
	}
	if (simroot != NULL && *simroot != '\0') {
		db->simroot = uovr_strdup(simroot);
		if (db->simroot == NULL) {
			return 1;
		}
	}
	db->dirty = 1;
	return 0;
}

int UnitsOverridesSet(struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name,
	CONST char *units
){
	struct UnitsOverrideEntry **ref;
	struct UnitsOverrideEntry *e;
	const struct Units *u;
	unsigned long pos = 0;
	int err = 0;
	CONST char *scope0 = (scope != NULL) ? scope : "";

	if (db == NULL || name == NULL || units == NULL || *name == '\0' || *units == '\0') {
		return 1;
	}
	if (kind != UNITS_OVERRIDE_TYPE && kind != UNITS_OVERRIDE_NAME) {
		return 1;
	}
	if (kind == UNITS_OVERRIDE_NAME && *scope0 == '\0') {
		error_reporter(ASC_USER_WARNING,NULL,0,NULL,
			"Ignoring global name override '%s' (name overrides must be file-scoped)",
			name
		);
		return 1;
	}

	u = FindOrDefineUnits(units,&pos,&err);
	if (u == NULL || err != 0) {
		error_reporter(ASC_USER_ERROR,NULL,0,NULL,
			"Invalid units override for '%s|%s': '%s'",
			scope0,name,units
		);
		return 2;
	}

	ref = uovr_find_ref(db,kind,scope0,name);
	if (ref == NULL) {
		return 3;
	}
	if (*ref != NULL) {
		e = *ref;
		if (e->units != NULL) {
			ascfree(e->units);
		}
		e->units = uovr_strdup(units);
		if (e->units == NULL) {
			return 4;
		}
		e->u = u;
		db->dirty = 1;
		return 0;
	}

	e = ASC_NEW_CLEAR(struct UnitsOverrideEntry);
	if (e == NULL) {
		return 4;
	}
	e->scope = uovr_strdup(scope0);
	e->name = uovr_strdup(name);
	e->units = uovr_strdup(units);
	e->u = u;
	if (e->scope == NULL || e->name == NULL || e->units == NULL) {
		uovr_free_entry(e);
		return 4;
	}
	e->next = NULL;
	*ref = e;
	db->dirty = 1;
	return 0;
}

int UnitsOverridesUnset(struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
){
	struct UnitsOverrideEntry **ref;
	struct UnitsOverrideEntry *e;
	CONST char *scope0 = (scope != NULL) ? scope : "";
	if (db == NULL || name == NULL || *name == '\0') {
		return 1;
	}
	ref = uovr_find_ref(db,kind,scope0,name);
	if (ref == NULL || *ref == NULL) {
		return 1;
	}
	e = *ref;
	*ref = e->next;
	uovr_free_entry(e);
	db->dirty = 1;
	return 0;
}

CONST struct Units *UnitsOverridesLookup(
	struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
){
	struct UnitsOverrideEntry **ref;
	CONST char *scope0 = (scope != NULL) ? scope : "";
	if (db == NULL || name == NULL || *name == '\0') {
		return NULL;
	}
	ref = uovr_find_ref(db,kind,scope0,name);
	if (ref == NULL || *ref == NULL) {
		return NULL;
	}
	return (*ref)->u;
}

static CONST struct Units *uovr_validate_resolved(
	struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name,
	const dim_type *dim
){
	CONST struct Units *u = UnitsOverridesLookup(db,kind,scope,name);
	if (u == NULL) {
		return NULL;
	}
	if (!SameDimen(dim,UnitsDimensions(u))) {
		error_reporter(ASC_USER_ERROR,NULL,0,NULL,
			"Removing invalid units override '%s|%s' -> '%s' (dimension mismatch)",
			(scope != NULL) ? scope : "", name, SCP(UnitsDescription(u))
		);
		UnitsOverridesUnset(db,kind,scope,name);
		return NULL;
	}
	return u;
}

static CONST char *uovr_name_without_simroot(CONST char *qlfdid){
	CONST char *dot;
	if (qlfdid == NULL) {
		return NULL;
	}
	dot = strchr(qlfdid,'.');
	if (dot == NULL || dot[1] == '\0') {
		return qlfdid;
	}
	return dot + 1;
}

CONST struct Units *UnitsOverridesResolve(
	struct UnitsOverridesDB *db,
	CONST char *scope,
	CONST char *type_name,
	CONST char *qlfdid,
	CONST dim_type *dim
){
	CONST struct Units *u;
	CONST char *scope0 = (scope != NULL) ? scope : "";
	if (db == NULL || dim == NULL) {
		return NULL;
	}
	if (IsWild(dim) || CmpDimen(dim,Dimensionless()) == 0) {
		return NULL;
	}
	if (qlfdid != NULL && *qlfdid != '\0' && *scope0 != '\0') {
		u = uovr_validate_resolved(db,UNITS_OVERRIDE_NAME,scope0,qlfdid,dim);
		if (u != NULL) {
			return u;
		}
		{
			CONST char *relname = uovr_name_without_simroot(qlfdid);
			if (relname != qlfdid) {
				u = uovr_validate_resolved(db,UNITS_OVERRIDE_NAME,scope0,relname,dim);
				if (u != NULL) {
					return u;
				}
			}
		}
	}
	if (type_name != NULL && *type_name != '\0') {
		if (*scope0 != '\0') {
			u = uovr_validate_resolved(db,UNITS_OVERRIDE_TYPE,scope0,type_name,dim);
			if (u != NULL) {
				return u;
			}
		}
		u = uovr_validate_resolved(db,UNITS_OVERRIDE_TYPE,"",type_name,dim);
		if (u != NULL) {
			return u;
		}
	}
	return NULL;
}

static char *uovr_ltrim(char *s){
	if (s == NULL) {
		return NULL;
	}
	while (*s != '\0' && isspace((unsigned char)*s)) {
		++s;
	}
	return s;
}

static void uovr_rtrim(char *s){
	size_t n;
	if (s == NULL) {
		return;
	}
	n = strlen(s);
	while (n > 0 && isspace((unsigned char)s[n - 1])) {
		s[--n] = '\0';
	}
}

static char *uovr_parse_section_name(char *line){
	char *s = uovr_ltrim(line);
	size_t n;
	if (s == NULL || *s != '[') {
		return NULL;
	}
	++s;
	n = strlen(s);
	if (n == 0 || s[n - 1] != ']') {
		return NULL;
	}
	s[n - 1] = '\0';
	uovr_rtrim(s);
	s = uovr_ltrim(s);
	if (*s == '\0') {
		return NULL;
	}
	return s;
}

int UnitsOverridesLoad(
	struct UnitsOverridesDB *db,
	CONST char *filename,
	unsigned *loaded,
	unsigned *errors
){
	FILE *fp;
	char buf[4096];
	unsigned nloaded = 0;
	unsigned nerrors = 0;
	char current_scope[4096];
	int have_section = 0;
	unsigned lineno = 0;

	if (loaded != NULL) {
		*loaded = 0;
	}
	if (errors != NULL) {
		*errors = 0;
	}
	if (db == NULL || filename == NULL || *filename == '\0') {
		return 1;
	}

	fp = fopen(filename,"r");
	if (fp == NULL) {
		if (errno == ENOENT) {
			return 0;
		}
		error_reporter(ASC_USER_ERROR,NULL,0,NULL,
			"Unable to read units overrides file '%s': %s",filename,strerror(errno)
		);
		return 1;
	}
	current_scope[0] = '\0';

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		char *line;
		char *eq;
		char *key;
		char *val;
		char *section;
		char *ovrname;
		enum UnitsOverrideKind kind;
		int rc;

		++lineno;
		buf[strcspn(buf,"\r\n")] = '\0';
		line = buf;
		{
			char *hash = strchr(line,'#');
			if (hash != NULL) {
				*hash = '\0';
			}
		}
		uovr_rtrim(line);
		line = uovr_ltrim(line);
		if (*line == '\0') {
			continue;
		}

		if (*line == '[') {
			section = uovr_parse_section_name(line);
			if (section == NULL) {
				error_reporter(ASC_USER_WARNING,NULL,0,NULL,
					"Ignoring unknown units-override section at line %u in '%s'",
					lineno,filename
				);
				++nerrors;
				have_section = 0;
			}else if (strcmp(section,"global") == 0) {
				current_scope[0] = '\0';
				have_section = 1;
			}else{
				snprintf(current_scope,sizeof(current_scope),"%s",section);
				have_section = 1;
			}
			continue;
		}

		if (!have_section) {
			error_reporter(ASC_USER_WARNING,NULL,0,NULL,
				"Ignoring units-override entry outside [global]/[<file>] at line %u in '%s'",
				lineno,filename
			);
			++nerrors;
			continue;
		}

		eq = strchr(line,'=');
		if (eq == NULL) {
			error_reporter(ASC_USER_WARNING,NULL,0,NULL,
				"Ignoring malformed units-override entry at line %u in '%s'",
				lineno,filename
			);
			++nerrors;
			continue;
		}
		*eq = '\0';
		key = uovr_ltrim(line);
		uovr_rtrim(key);
		val = uovr_ltrim(eq + 1);
		uovr_rtrim(val);

		if (strncmp(key,"type.",5) == 0) {
			kind = UNITS_OVERRIDE_TYPE;
			ovrname = key + 5;
		}else if (strncmp(key,"name.",5) == 0) {
			kind = UNITS_OVERRIDE_NAME;
			ovrname = key + 5;
		}else{
			error_reporter(ASC_USER_WARNING,NULL,0,NULL,
				"Ignoring units-override key without type./name. prefix at line %u in '%s'",
				lineno,filename
			);
			++nerrors;
			continue;
		}
		ovrname = uovr_ltrim(ovrname);
		uovr_rtrim(ovrname);
		if (*ovrname == '\0' || *val == '\0') {
			error_reporter(ASC_USER_WARNING,NULL,0,NULL,
				"Ignoring empty units-override key/value at line %u in '%s'",
				lineno,filename
			);
			++nerrors;
			continue;
		}
		if (kind == UNITS_OVERRIDE_NAME && current_scope[0] == '\0') {
			error_reporter(ASC_USER_WARNING,NULL,0,NULL,
				"Ignoring global name override '%s' at line %u in '%s'",
				ovrname,lineno,filename
			);
			++nerrors;
			continue;
		}
		rc = UnitsOverridesSet(
			db,
			kind,
			current_scope,
			ovrname,
			val
		);
		if (rc != 0) {
			++nerrors;
		}else{
			++nloaded;
		}
	}
	fclose(fp);
	if (loaded != NULL) {
		*loaded = nloaded;
	}
	if (errors != NULL) {
		*errors = nerrors;
	}
	db->dirty = 0;
	return 0;
}

static int uovr_scope_exists(struct gl_list_t *scopes, CONST char *scope){
	unsigned long i, len;
	if (scopes == NULL || scope == NULL) {
		return 0;
	}
	len = gl_length(scopes);
	for (i = 1; i <= len; ++i) {
		CONST char *s = (CONST char *)gl_fetch(scopes,i);
		if (s != NULL && uovr_scope_eq(s,scope)) {
			return 1;
		}
	}
	return 0;
}

static void uovr_collect_scopes(struct gl_list_t *scopes, struct UnitsOverrideEntry **buckets){
	unsigned long i;
	for (i = 0; i < UOVR_BUCKETS; ++i) {
		struct UnitsOverrideEntry *e;
		for (e = buckets[i]; e != NULL; e = e->next) {
			if (e->scope != NULL && e->scope[0] != '\0' && !uovr_scope_exists(scopes,e->scope)) {
				char *dup = uovr_strdup(e->scope);
				if (dup != NULL) {
					gl_append_ptr(scopes,dup);
				}
			}
		}
	}
}

static void uovr_write_entries(
	FILE *fp,
	struct UnitsOverrideEntry **buckets,
	CONST char *scope,
	CONST char *prefix
){
	unsigned long i;
	for (i = 0; i < UOVR_BUCKETS; ++i) {
		struct UnitsOverrideEntry *e;
		for (e = buckets[i]; e != NULL; e = e->next) {
			if (uovr_scope_eq(e->scope,scope)) {
				FPRINTF(fp,"%s%s = %s\n",prefix,e->name,e->units);
			}
		}
	}
}

static CONST char *uovr_strip_configured_simroot(
	CONST char *name,
	CONST char *simroot
){
	size_t n;
	if (name == NULL || simroot == NULL || *simroot == '\0') {
		return name;
	}
	n = strlen(simroot);
	if (strncmp(name,simroot,n) == 0 && name[n] == '.' && name[n + 1] != '\0') {
		return name + n + 1;
	}
	return name;
}

static int uovr_scope_name_exists(
	struct UnitsOverrideEntry **buckets,
	CONST char *scope,
	CONST char *name,
	CONST struct UnitsOverrideEntry *skip
){
	struct UnitsOverrideEntry *e;
	unsigned long h;
	if (buckets == NULL || scope == NULL || name == NULL || *name == '\0') {
		return 0;
	}
	h = uovr_hash(scope,name);
	for (e = buckets[h]; e != NULL; e = e->next) {
		if (e != skip && uovr_scope_eq(e->scope,scope) && strcmp(e->name,name) == 0) {
			return 1;
		}
	}
	return 0;
}

static void uovr_write_name_entries(
	FILE *fp,
	struct UnitsOverrideEntry **buckets,
	CONST char *scope,
	CONST char *simroot
){
	unsigned long i;
	for (i = 0; i < UOVR_BUCKETS; ++i) {
		struct UnitsOverrideEntry *e;
		for (e = buckets[i]; e != NULL; e = e->next) {
			if (uovr_scope_eq(e->scope,scope)) {
				CONST char *outname = uovr_strip_configured_simroot(e->name,simroot);
				if (outname != e->name
				 && uovr_scope_name_exists(buckets,scope,outname,e)) {
					continue;
				}
				FPRINTF(fp,"name.%s = %s\n",outname,e->units);
			}
		}
	}
}

static int uovr_mkdir_single(CONST char *path){
#ifdef _WIN32
	if (_mkdir(path) == 0 || errno == EEXIST) {
#else
	if (mkdir(path,0775) == 0 || errno == EEXIST) {
#endif
		return 0;
	}
	return 1;
}

static int uovr_ensure_parent_dir(CONST char *filename){
	char *tmp;
	char *slash;
	char *p;
	if (filename == NULL) {
		return 1;
	}
	tmp = uovr_strdup(filename);
	if (tmp == NULL) {
		return 1;
	}
	slash = strrchr(tmp,'/');
	if (slash == NULL) {
		ascfree(tmp);
		return 0;
	}
	*slash = '\0';
	p = tmp;
	if (*p == '/') {
		++p;
	}
	while (*p != '\0') {
		if (*p == '/') {
			*p = '\0';
			if (tmp[0] != '\0' && uovr_mkdir_single(tmp) != 0) {
				ascfree(tmp);
				return 1;
			}
			*p = '/';
		}
		++p;
	}
	if (tmp[0] != '\0' && uovr_mkdir_single(tmp) != 0) {
		ascfree(tmp);
		return 1;
	}
	ascfree(tmp);
	return 0;
}

int UnitsOverridesSave(
	struct UnitsOverridesDB *db,
	CONST char *filename
){
	FILE *fp;
	unsigned long i;
	struct gl_list_t *scopes;
	if (db == NULL || filename == NULL || *filename == '\0') {
		return 1;
	}
	if (uovr_ensure_parent_dir(filename) != 0) {
		error_reporter(ASC_USER_ERROR,NULL,0,NULL,
			"Unable to create parent directories for '%s'",filename
		);
		return 1;
	}
	fp = fopen(filename,"w");
	if (fp == NULL) {
		error_reporter(ASC_USER_ERROR,NULL,0,NULL,
			"Unable to write units overrides file '%s': %s",filename,strerror(errno)
		);
		return 1;
	}
	FPRINTF(fp,"# ASCEND units overrides\n");
	FPRINTF(fp,"# sections: [global], [<relative-or-absolute-model-file>]\n");
	FPRINTF(fp,"# keys: type.<type_name> = <units>, name.<qlfdid_without_simroot> = <units>\n\n");
	FPRINTF(fp,"[global]\n");
	uovr_write_entries(fp,db->type_buckets,"","type.");
	scopes = gl_create(16L);
	uovr_collect_scopes(scopes,db->type_buckets);
	uovr_collect_scopes(scopes,db->name_buckets);
	for (i = 1; i <= gl_length(scopes); ++i) {
		char *scope = (char *)gl_fetch(scopes,i);
		FPRINTF(fp,"\n[%s]\n",scope);
		uovr_write_entries(fp,db->type_buckets,scope,"type.");
		uovr_write_name_entries(fp,db->name_buckets,scope,db->simroot);
	}
	for (i = 1; i <= gl_length(scopes); ++i) {
		char *scope = (char *)gl_fetch(scopes,i);
		if (scope != NULL) {
			ascfree(scope);
		}
	}
	gl_destroy(scopes);
	fclose(fp);
	db->dirty = 0;
	return 0;
}

char *UnitsOverridesDefaultPath(void){
	CONST char *xdg = getenv("XDG_CONFIG_HOME");
	CONST char *home = getenv("HOME");
	CONST char *appdata = getenv("APPDATA");
	char *out;
	size_t n;
	if (xdg != NULL && *xdg != '\0') {
		n = strlen(xdg) + strlen("/ascend/units-overrides.ini") + 1;
		out = ASC_NEW_ARRAY(char,n);
		if (out == NULL) {
			return NULL;
		}
		snprintf(out,n,"%s/ascend/units-overrides.ini",xdg);
		return out;
	}
	if (home != NULL && *home != '\0') {
		n = strlen(home) + strlen("/.config/ascend/units-overrides.ini") + 1;
		out = ASC_NEW_ARRAY(char,n);
		if (out == NULL) {
			return NULL;
		}
		snprintf(out,n,"%s/.config/ascend/units-overrides.ini",home);
		return out;
	}
	if (appdata != NULL && *appdata != '\0') {
		n = strlen(appdata) + strlen("/ascend/units-overrides.ini") + 1;
		out = ASC_NEW_ARRAY(char,n);
		if (out == NULL) {
			return NULL;
		}
		snprintf(out,n,"%s/ascend/units-overrides.ini",appdata);
		return out;
	}
	return NULL;
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

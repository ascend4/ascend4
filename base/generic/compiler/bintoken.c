/* ex: set ts=8 : */
/*
 *  bintoken.c
 *  By Benjamin A. Allan
 *  Jan 7, 1998.
 *  Part of ASCEND
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: bintoken.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:36 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#if 0
TIMESTAMP = -DTIMESTAMP="\"by `whoami`@`hostname`\""
#endif
/*
 * binary tokens implementation for real relation instances.
 * much of this goes in bintoken.h.
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPrint.h"
#include "utilities/ascSignal.h"
#include "utilities/ascPanic.h"
#include "utilities/ascDynaLoad.h"
#include "general/list.h"
#include "general/dstring.h"
#include "general/pretty.h"
#include "compiler/compiler.h" /* for symchar */
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/statio.h"
#include "compiler/instance_enum.h"
#include "compiler/instquery.h"
#include "compiler/instance_io.h"
#include "compiler/relation_type.h"
#include "compiler/relation_io.h"
#include "compiler/find.h"
#include "compiler/relation.h"
#include "compiler/relation_util.h"
#include "compiler/mathinst.h"
/* last */
#include "compiler/bintoken.h"
#include "compiler/btprolog.h"

#define CLINE(a) FPRINTF(fp,"%s\n",(a))

#if (defined(__HPUX__) || defined(__ALPHA_OSF__) || \
     defined(__WIN32__) || defined(__SUN_SOLARIS__) || \
     defined(__SUN_SUNOS__) || defined(__SGI_IRIX__))
#define HAVE_DL_UNLOAD 1
#endif
/* we don't know about ultrix, aix, and others */

enum bintoken_error {
  BTE_ok,
  BTE_badrel,
  BTE_write,
  BTE_build,
  BTE_load,
  BTE_mem
};

struct bt_table {
  enum bintoken_kind type;
  char *name;
  union TableUnion *tu;
  int btable; /* check id */
  int refcount;	/* total number of relation shares with btable = our number */
  int size; /* may be larger than refcount. */
};

/*
 * slot to manage all the tables from, especially if we'll do
 * dynamic unloading.
 */
struct bt_data {
  struct bt_table *tables;
  int captables;
  int nextid;
  /* loading hooks */
  union TableUnion *newtable;
  int newtablesize;
  char regname[256];
  /* ui set build options */
  char *srcname;
  char *objname;
  char *libname;
  char *buildcommand;
  char *unlinkcommand;
  unsigned long maxrels; /* no more than this many C relations per file */
  int verbose; /* comments in generated code */
  int housekeep; /* if !=0, generated src files are deleted sometimes. */
} g_bt_data = {NULL,0,0,NULL,0,"ERRARCHIVE",NULL,NULL,NULL,NULL,NULL,1,0,0};

/**
	I can't work out how to fix the warning here. In the C++ interface, I need
	the arguments of BinTokenSetOptions to be const char*. But here, new can
	lose its constness in *ptr = new.
*/
static
int bt_string_replace(CONST char *new, char **ptr)
{
  if (*ptr == new) {
    return 0;
  }
  if (new == NULL) {
    if (*ptr != NULL) {
      ascfree(*ptr);
      *ptr = NULL;
    }
  } else {
    if (*ptr != NULL) {
      ascfree(*ptr);
    }
    *ptr = new;
  }
  return 0;
}
/*
 * Set the configurations for building code.
 * The string arguments given are kept.
 * They are freed on the next call which specifies a new string or NULL.
 * strings given should not be allocated from tcl.
 */
int BinTokenSetOptions(CONST char *srcname,
                       CONST char *objname,
                       CONST char *libname,
                       CONST char *buildcommand,
                       CONST char *unlinkcommand,
                       unsigned long maxrels,
                       int verbose,
                       int housekeep)
{
  int err = 0;
  err += bt_string_replace(srcname,&(g_bt_data.srcname));
  err += bt_string_replace(objname,&(g_bt_data.objname));
  err += bt_string_replace(libname,&(g_bt_data.libname));
  err += bt_string_replace(buildcommand,&(g_bt_data.buildcommand));
  err += bt_string_replace(unlinkcommand,&(g_bt_data.unlinkcommand));
  g_bt_data.maxrels = maxrels;
  g_bt_data.verbose = verbose;
  g_bt_data.housekeep = housekeep;
  return err;
}


/*
 * grows the table when need be.
 * note that nextid is the current number of possibly real
 * entries in the table and we need to insure that nextid+1
 * exists because we are running this table from 1 instead of 0.
 */
static
int BinTokenCheckCapacity()
{
  if (g_bt_data.tables == NULL) {
    assert(g_bt_data.captables == 0);
    g_bt_data.tables =
      (struct bt_table *)ascmalloc(20*sizeof(struct bt_table));
    assert(g_bt_data.tables != NULL);
    g_bt_data.captables = 20;
    return 0;
  }
  if (g_bt_data.nextid >= g_bt_data.captables) {
    g_bt_data.tables = (struct bt_table *)ascrealloc(g_bt_data.tables,
           2*sizeof(struct bt_table)*g_bt_data.captables);
    assert(g_bt_data.tables != NULL);
    g_bt_data.captables *= 2;
  }
  return 0;
}

/*
 * frees global memory.
 * should be more careful.
 */
void BinTokenClearTables(void)
{
  if (g_bt_data.tables != NULL) {
    ascfree(g_bt_data.tables);
    g_bt_data.tables = NULL;
  }
  g_bt_data.captables = 0;
  g_bt_data.nextid = 0;
  BinTokenSetOptions(NULL,NULL,NULL,NULL,NULL,1,0,0);
}

/*
 * when all the references expire, unload the library.
 * note there is no AddReference since all the references
 * are made 1 per share at load time.
 */
void BinTokenDeleteReference(int btable)
{
  if (btable < 0 || btable > g_bt_data.nextid ||
      g_bt_data.tables[btable].type == BT_error) {
    return;
    /* relation references a loadfailure library or already deleted
     * or corrupted memory has made its way here.
     */
  }
  g_bt_data.tables[btable].refcount--;
  if (g_bt_data.tables[btable].refcount == 0) {
    /* unload the library if possible here */
#if HAVE_DL_UNLOAD
    Asc_DynamicUnLoad(g_bt_data.tables[btable].name);
#endif /* havedlunload */
    ascfree(g_bt_data.tables[btable].name);
    g_bt_data.tables[btable].name = NULL;
    g_bt_data.tables[btable].tu = NULL;
    g_bt_data.tables[btable].type = BT_error;
  }
}


/*
 * submodule for reducing string form of equations to uniqueness.
 * This portion is independent of the generated language.
 * Ben Allan, 2/98.
 */

struct bintoken_unique_eqn {
  int indexU; /* Unique function index of this string */
  int firstrel; /* index of first relation to have this string.
                 * this can give us a relation list index to refer
                 * to for generating unique gradient code, rather than
                 * differentiating all the eqns in the formal rellist.
                 */
  int refcount;
  int len; /* strlen of the string form */
  /* int-sized hole here on long pointer machines intentional */
  char *str;    /* common string form of the eqn */
};

struct bintoken_eqlist {
  struct gl_list_t *ue; /* list of unique eqn code strings */
  int *rel2U;
  /* array indexed by relindex, giving the corresponding
   * unique equation indexU.
   */
  int nextnew; /* starts at 0. index of the next new unique. */
};


/* return 1 if error, 0 if ok */
static
int InitEQData(struct bintoken_eqlist *eql, int len)
{
  eql->nextnew = 0;
  eql->ue = gl_create(len);
  if (eql->ue == NULL) {
    return 1;
  }
  eql->rel2U = (int *)ascmalloc((len+1)*sizeof(int));
  if (eql->rel2U == NULL) {
    gl_destroy( eql->ue );
    return 1;
  }
  return 0;
}

static
void DestroyEQData(struct bintoken_eqlist *eql)
{
  struct bintoken_unique_eqn *u;
  unsigned long c;
  for (c=gl_length(eql->ue); c > 0; c--) {
    u = (struct bintoken_unique_eqn *)gl_fetch(eql->ue,c);
    if (u != NULL) {
      if (u->str != NULL) {
        ascfree(u->str);
      }
      ascfree(u);
    }
  }
  gl_destroy(eql->ue);
  ascfree(eql->rel2U);
}

/*
 * This function compares first on string len, secondarily on
 * str content. knownas is not considered. This function can be
 * used to search a gl_list of existing unique_eqns to figure
 * out whether to add a new one or simply extend an existing ones
 * knownas list.
 */
static
int CmpUniqueEqn(struct bintoken_unique_eqn *u1, struct bintoken_unique_eqn *u2)
{
  assert(u1!=NULL);
  assert(u2!=NULL);
  assert(u1->len!=0);
  assert(u2->len!=0);
  assert(u1->str!=NULL);
  assert(u2->str!=NULL);
  if (u1==u2) {
    /* should never, ever happen */
    return 0;
  }
  if (u1->len != u2->len) {
    /* I don't know whether this sorts increasing or decreasing len.
     * not that it really matters. we're sorting on len first to avoid
     * strcmp calls.
     */
    if (u1->len < u2->len) {
      return -1;
    } else {
      return 1;
    }
  }
  return strcmp(u1->str,u2->str);
}

/*
 * Finds or inserts a unique eqn in the list eql.
 * Records the unique index U in eql->rel2U[relindex].
 * Returns 1 if added a record to eql referencing str.
 * Returns 0 if str already exists in eql somewhere.
 * len is the length of str.
 * relindex is the index of the instance the string
 * came from in some instance list.
 */
static
int BinTokenAddUniqueEqn(struct bintoken_eqlist *eql, int relindex,
                         char *str, int len)
{
  struct bintoken_unique_eqn test, *new, *old;
  unsigned long pos;
  assert(eql != NULL);
  assert(relindex >= 0);
  assert(str != NULL);

  test.len = len;
  test.str = str;
  pos = gl_search(eql->ue,&test,(CmpFunc)CmpUniqueEqn);
  if (!pos) {
    /* create new unique eqn */
    new = (struct bintoken_unique_eqn *)
             ascmalloc(sizeof(struct bintoken_unique_eqn));
    assert(new!=NULL);
    new->len = test.len;
    new->firstrel = relindex;
    new->refcount = 1;
    eql->rel2U[relindex] = eql->nextnew;
    new->indexU = (eql->nextnew)++;
    new->str = str; /* keep string */
    gl_insert_sorted(eql->ue,new,(CmpFunc)CmpUniqueEqn);
    return 1;
  } else {
    /* saw it already */
    old = (struct bintoken_unique_eqn *)gl_fetch(eql->ue,pos);
    old->refcount++;
    eql->rel2U[relindex] = old->indexU;
    return 0;
  }
}

/*
 * C code specific stuff
 */

/*
 * includes the standard headers and any supporting functions
 * we may require.
 */
static
void WritePrologue(FILE *fp, struct Instance *root,
                   unsigned long len, int verbose)
{
  if (verbose) {
    CLINE("/* BinTokenSharesToC $Revision: 1.12 $");
    FPRINTF(fp," * %lu relations in\n * ",len);
    WriteInstanceName(fp,root,NULL);
    CLINE("\n * (possibly fewer C functions required)\n */");
  }
#ifdef HAVE_ERF
  /* need to define this for btprolog.h to do the right thing */
  CLINE("#define HAVE_ERF");
#endif
  CLINE("#include \"btprolog.h\"");
}

/* this function should be generalized or duplicated to
 * handle other languages. It's almost there now.
 */
static
enum bintoken_error GetResidualString(struct Instance *i,
                                      int nrel,
                                      struct RXNameData *rd,
                                      enum rel_lang_format lang,
                                      int *rellen,
                                      char **streqn)
{
  assert(i!=NULL);
  assert(InstanceKind(i)==REL_INST);

  *streqn = WriteRelationString(i,NULL,(WRSNameFunc)RelationVarXName,
                               rd,lang,rellen);
  if (*streqn==NULL) {
    FPRINTF(ASCERR,"Unable to generate code for (%d):\n",nrel);
    WriteAnyInstanceName(ASCERR,i);
    return BTE_badrel;
  }
  return BTE_ok;
}

/* this function should be generalized or duplicated to
 * handle other languages. Should be ok for most C-like languages.
 * Writes K&R C.
 */
static
enum bintoken_error WriteResidualCode(FILE *fp, struct Instance *i,
                                      int nrel, int verbose,
                                      char *streqn, int timesused)
{
#define C_INDENT 4
#define C_WIDTH 70
  assert(i!=NULL);

  if (streqn==NULL) {
    return BTE_badrel;
  }

  if (verbose) {
    /* put in a little header */
    CLINE("\n/*");
    FPRINTF(fp,"\tRelation used %d times, prototyped from:\n",timesused);
    FPRINTF(fp,"\t");
    /* Use fastest path to a root */
    WriteAnyInstanceName(fp,i);
    CLINE("\n*/");
  }

  CLINE("static");
  FPRINTF(fp, "void r_%d(double *x,double *residual){\n", nrel);
  CLINE("\t*residual =");
#define FMTNORMAL 1
#if FMTNORMAL
  print_long_string(fp,streqn,C_WIDTH,C_INDENT); /* human readable, sort of */
#else
   FPRINTF(fp,"%s",streqn); /* all on one ugly long line */
#endif

  if (verbose) {
    FPRINTF(fp, "  ; /* eqn %d */\n", nrel);
  } else {
    CLINE("  ;");
  }
  CLINE("}");
  return BTE_ok;
}

/*
 * t is the array of function pointers. size is number or
 * relations represented +1 since element 0 is {NULL,NULL}
 * by convention.
 */
int DLEXPORT ExportBinTokenCTable(struct TableC *t,int size)
{
  if (g_bt_data.newtable != NULL || t == NULL || size < 1) {
    return 1;
  }
  g_bt_data.newtable = (union TableUnion *)t;
  g_bt_data.newtablesize = size;
  return 0;
}

struct reusable_rxnd {
  struct RXNameData rd;
  unsigned long cap;
};

/*
 * puts an index list in r->rd which is just the shift by 1
 * so r->rd.indices[j+1] == j.
 */
static
void ResizeIndices(struct Instance *rel, struct reusable_rxnd *r)
{
  unsigned long newlen,j;
  assert(r!=NULL);

  /* free and return if NULL rel */
  if (rel == NULL) {
    if (r->rd.indices != NULL) {
      ascfree(r->rd.indices);
      r->rd.indices = NULL;
      r->cap = 0;
    }
    return;
  }

  /* get desired size */
  newlen = NumberVariables(GetInstanceRelationOnly(rel));
  newlen++; /* gotta remember to allow for indexing from 1 */

  /* skip out if we have it already */
  if (newlen <= r->cap) {
    return;
  }

  if (r->rd.indices != NULL) {
    /* assume we'll grow again and try not to do it often */
    ascfree(r->rd.indices);
    r->rd.indices = NULL;
    newlen *= 2;
  }
  /* require min */
  if (newlen < 100) {
    newlen = 100;
  }
  /* create mem_*/
  r->rd.indices = (int *)ascmalloc(sizeof(int)*newlen);
  if (r->rd.indices == NULL) {
    Asc_Panic(2, "BinTokenSharesToC","out of memory error");
    exit(2);
  }
  /* set up one-less indices */
  for (j = 0; j < newlen; j++) {
    r->rd.indices[j] = (int)j - 1;
  }
  r->cap = newlen;
}

/*
 * generate code for a table of function pointers and the function
 * pointers also in an archive load function.
 * The table is always 1 pair larger than rellist since by convention
 * index 0 has the NULL functions.
 */
static
enum bintoken_error BinTokenSharesToC(struct Instance *root,
                                      struct gl_list_t *rellist,
                                      char *srcname,
                                      int verbose)
{
  int *error;
  FILE *fp;
  struct Instance *i;
  char *str;
  int slen;
  struct bintoken_unique_eqn *eqn;
  struct bintoken_eqlist eql;
  unsigned long c, len;
  int pid;
  int eqns_done;
  struct reusable_rxnd rrd = {{"x[",NULL,"]"},0};

  if (root == NULL ||  rellist == NULL) {
    return BTE_ok;
  }
  len = gl_length(rellist);
  if (!len) {
    return BTE_ok;
  }
  fp = fopen(srcname,"w+");
  if (fp == NULL) {
    return BTE_write;
  }
  eqns_done = 0;
  error = (int *)ascmalloc(len*sizeof(int));
  WritePrologue(fp,root,len,verbose);

/* algorithm to collect eqns:
 * (at the cost of more string memory, since we keep unique strings while
 * determining minimum set of C functions to write).
 * Really, the instantiator could be taking steps to make this less necesssary,
 * but even then the compiler will miss some similarities arising from
 * different Statements.
 */

  if (InitEQData(&eql,(int)len)!= 0) {
    fclose(fp);
    return BTE_mem;
  }

  /* get unique set of code strings. */
  for (c=1; c <= len; c++) {
    i = gl_fetch(rellist,c);
    /* make space and configure for subscript translation from 1 to 0 */
    ResizeIndices(i,&rrd);
    error[c-1] = GetResidualString(i,(int)c,&(rrd.rd),relio_C,&slen,&str);
    if (error[c-1] == BTE_ok) {
      eqns_done++;
      if (BinTokenAddUniqueEqn(&eql,(int)c,str,slen) == 0) {
        ascfree(str);
      } /* else string is kept in eql and killed later */
    }
    /* else { eql.rel2U[c] = -1; } needed? */
  }
  ResizeIndices(NULL,&rrd);
  if (!eqns_done) {
    /* no generable code. clean up and leave. */
    fclose(fp);
    DestroyEQData(&eql);
    return BTE_badrel;
  }
  for (c = gl_length(eql.ue); c > 0; c--) {
    eqn = (struct bintoken_unique_eqn *)gl_fetch(eql.ue,c);
    i = gl_fetch(rellist,eqn->firstrel);
    WriteResidualCode(fp,i,eqn->indexU,verbose,eqn->str,eqn->refcount);
    /* here we could also write gradient code based on i, indexU. */
  }
  /* write the registered function name */
  pid = getpid();
  /** @TODO FIXME win32 has getpid but it is bogus as uniquifier. */
  /* so long as makefile deletes previous dll, windows is ok though */
  sprintf(g_bt_data.regname,"BinTokenArch_%d_%d",++(g_bt_data.nextid),(int)pid);
  FPRINTF(fp,"int DLEXPORT %s(){\n",g_bt_data.regname);
  CLINE("\tint status;");
  FPRINTF(fp,"\tstatic struct TableC g_ctable[%lu] =\n",len+1);
  CLINE("\t\t{ {NULL, NULL},");
  len--; /* to fudge the final comma */
  for (c=1; c <= len; c++) {
    if (error[c-1] == BTE_ok) {
      FPRINTF(fp,"\t\t\t{r_%u, NULL},\n",eql.rel2U[c]);
    } else {
      FPRINTF(fp,"\t\t\t{NULL, NULL},\n");
    }
  }
  len++;
  if (error[len-1] == BTE_ok) {
    FPRINTF(fp,"\t\t\t{r_%u, NULL}\n",eql.rel2U[c]);
  } else {
    FPRINTF(fp,"\t\t\t{NULL, NULL}\n");
  }
  CLINE("\t\t};");
  FPRINTF(fp,"\tstatus = ExportBinTokenCTable(g_ctable,%lu);\n",len+1);
  CLINE("\treturn status;");
  if (verbose) {
    FPRINTF(fp,"\t/* %lu unique equations */\n",gl_length(eql.ue));
    FPRINTF(ASCERR,"C Functions: %lu\n",gl_length(eql.ue));
  }
  CLINE("}");

  ascfree(error);
  DestroyEQData(&eql);
  fclose(fp);
  return BTE_ok;
}

static
enum bintoken_error BinTokenCompileC(char *buildcommand)
{
  int status;
  error_reporter(ASC_PROG_NOTE,NULL,0,"Starting build, command:\n%s",buildcommand);
  status = system(buildcommand);
  if (status) {
    FPRINTF(ASCERR,"\nBUILD returned %d\n",status);
    return BTE_build;
  }
  return BTE_ok;
}

static
void BinTokenResetHooks()
{
  g_bt_data.tables[g_bt_data.nextid].type = BT_error;
  g_bt_data.newtable = NULL;
  g_bt_data.newtablesize = 0;
}

static
void BinTokenHookToTable(int entry, enum bintoken_kind type)
{
  g_bt_data.tables[entry].tu = g_bt_data.newtable;
  g_bt_data.tables[entry].size = g_bt_data.newtablesize;
  g_bt_data.tables[entry].btable = entry;
  g_bt_data.tables[entry].type = type;
  g_bt_data.newtable = NULL;
  g_bt_data.newtablesize = 0;
}

static
enum bintoken_error BinTokenLoadC(struct gl_list_t *rellist,
                                  char *libname,
                                  char *regname)
{
  int status;
  unsigned long c,len;
  BinTokenCheckCapacity();
  status = Asc_DynamicLoad(libname,regname);
  if (status != 0) {
    FPRINTF(ASCERR,"Load failure of %s:%s\n",libname,regname);
    BinTokenResetHooks();
    /*  could do this maybe, but not needed if we want each
     * relation to get one shot only..
     * for (c=1;c <= len; c++) {
     *   RelationSetBinTokens((struct Instance *)gl_fetch(rellist,c),
     *                        g_bt_data.nextid,(int)c);
     * }
     */
    return BTE_load;
  }
  BinTokenHookToTable(g_bt_data.nextid,BT_C);
  len = gl_length(rellist);
  for (c=1;c <= len; c++) {
    RelationSetBinTokens((struct Instance *)gl_fetch(rellist,c),
                         g_bt_data.nextid,(int)c);
  }
  g_bt_data.tables[g_bt_data.nextid].refcount = (int)len;
  g_bt_data.tables[g_bt_data.nextid].name = ascstrdup(libname);
  return BTE_ok;
}

/*
 * this function should be more helpful.
 */
static
void BinTokenErrorMessage(enum bintoken_error err,
                          struct Instance *root,
                          char *filename,
                          char *buildcommand)
{
  char *mess;

  (void)root;
  (void)filename;
  (void)buildcommand;

  switch(err) {
  case BTE_ok:
    mess="A-ok";
    break;
  case BTE_badrel:
    mess="Bad relation found in code generation";
    break;
  case BTE_write:
    mess="Unable to write file";
    break;
  case BTE_build:
    mess="Unable to build binary";
    break;
  case BTE_load:
    mess="Loaded binary does not match code written";
    break;
  case BTE_mem:
    mess="Insufficient memory to write code.";
    break;
  default:
    mess="Unknown error in BinTokenErrorMessage";
    break;
  }
  FPRINTF(ASCERR,"%s: %s\n",__FILE__,mess);
}

void BinTokensCreate(struct Instance *root, enum bintoken_kind method)
{
  struct gl_list_t *rellist;
  char *cbuf;
  enum bintoken_error status;
  char *srcname = g_bt_data.srcname;
  char *objname = g_bt_data.objname;
  char *libname = g_bt_data.libname;
  char *buildcommand = g_bt_data.buildcommand;
  char *unlinkcommand = g_bt_data.unlinkcommand;
  int verbose = g_bt_data.verbose;

  if (g_bt_data.maxrels == 0) {
    return;
  }
  if (srcname == NULL || buildcommand == NULL || unlinkcommand == NULL) {
    FPRINTF(ASCERR,"%sBinaryTokensCreate called with no options set.",
            StatioLabel(3));
    return;
  }

  rellist =
    CollectTokenRelationsWithUniqueBINlessShares(root,g_bt_data.maxrels);
  if (rellist==NULL) {
    FPRINTF(ASCERR,
        "%sBinaryTokensCreate found 0 or too many unique relations\n",
        StatioLabel(2));
    return;
  }

  switch (method) {
  case BT_C:
    /* generate code */
    status = BinTokenSharesToC(root,rellist,srcname,verbose);
    if (status != BTE_ok) {
      BinTokenErrorMessage(status,root,srcname,buildcommand);
      break; /* leave source file there if partial */
    }
    status = BinTokenCompileC(buildcommand);
    if (status != BTE_ok) {
      BinTokenErrorMessage(status,root,objname,buildcommand);
      break; /* leave source file there to debug */
    } else {
      if (g_bt_data.housekeep) {
        /* trash src */
        cbuf = (char *)ascmalloc(strlen(unlinkcommand)+1+strlen(srcname)+1);
        assert(cbuf!=NULL);
        sprintf(cbuf,"%s %s",unlinkcommand,srcname);
        system(cbuf); /* we don't care if the delete fails */
        ascfree(cbuf);
        /* trash obj */
        cbuf = (char *)ascmalloc(strlen(unlinkcommand)+1+strlen(objname)+1);
        assert(cbuf!=NULL);
        sprintf(cbuf,"%s %s",unlinkcommand,objname);
        system(cbuf); /* we don't care if the delete fails */
        ascfree(cbuf);
      }
    
      status = BinTokenLoadC(rellist,libname,g_bt_data.regname);
      if (status != BTE_ok) {
        BinTokenErrorMessage(status,root,libname,buildcommand);
        /* leave source,binary files there to debug */
      }/*else{
        FPRINTF(ASCERR,"BINTOKENLOADC OK\n");
      }*/
    }
    break;
  case BT_F77:
  case BT_SunJAVA:
  case BT_MsJAVA:
  default:
    FPRINTF(ASCERR,"%sBinaryTokensCreate called with\n" /* no comma */
            "  unavailable method %d",StatioLabel(3),(int)method);
    break;
  }
  gl_destroy(rellist);
  return;
}

/*
 * Returns 1 if can't evaluate function.
 * Vars is assumed already filled with values.
 * This function must not malloc or free memory.
 */
int BinTokenCalcResidual(int btable, int bindex, double *vars, double *residual)
{
  if (btable < 1 || bindex < 1) {
    return 1;
  }
  switch (g_bt_data.tables[btable].type) {
  case BT_error:
    return 1; /* expired table! */
  case BT_C: {
      struct TableC *ctable;
      BinTokenFPtr func;
      ctable = (struct TableC *)g_bt_data.tables[btable].tu;
      assert(ctable != NULL);
      if (bindex > g_bt_data.tables[btable].size) {
        return 1;
      }
      func = ctable[bindex].F;
#if 0 /* setting this to 1 is a major performance hit. */
      if (func != NULL) {
        if (setjmp(g_fpe_env)==0) {
          (*func)(vars,residual);
          return 0;
        } else {
          Asc_SignalRecover();
          return 1;
        }
      }
      return 1;
#else
      (*func)(vars,residual);
      return 0;
#endif
    }
  case BT_F77: {
      /* this case needs to be cleaned up to match the C case above. */
      struct TableF *ftable;
      BinTokenSPtr subroutine;
      ftable = (struct TableF *)g_bt_data.tables[btable].tu;
      assert(ftable != NULL);
      if (bindex < 1 || bindex > g_bt_data.tables[btable].size) {
        return 1;
      }
      subroutine = ftable[0].S; /* its all in func 0 */
      if (subroutine != NULL) {
        int ForG,status;
        ForG = BinTokenRESIDUAL;
#ifndef NO_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (setjmp(g_fpe_env)==0) {
#endif /* NO_SIGNAL_TRAPS */
          (*subroutine)(vars,NULL,residual,&ForG,&bindex,&status);
#ifndef NO_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* NO_SIGNAL_TRAPS */
          return status;
#ifndef NO_SIGNAL_TRAPS
        } else {
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
          return 1;
        }
#endif /* NO_SIGNAL_TRAPS */
      }
      return 1;
    }
  case BT_SunJAVA:
  case BT_MsJAVA:
  default:
    return 1;
  }
}

/*
 * Returns nonzero if can't evaluate gradient.
 * Vars is assumed already filled with values.
 */
int BinTokenCalcGradient(int btable, int bindex,double *vars,
                         double *residual, double *gradient)
{
  if (btable == 0) {
    return 1;
  }
  switch (g_bt_data.tables[btable].type) {
  case BT_error:
    return 1; /* expired table! */
  case BT_C: {
      /* signal handling needs to match func above. this is slow here. */
      struct TableC *ctable;
      BinTokenGPtr func;
      ctable = (struct TableC *)g_bt_data.tables[btable].tu;
      assert(ctable != NULL);
      if (bindex < 1 || bindex > g_bt_data.tables[btable].size) {
        return 1;
      }
      func = ctable[bindex].G;
      if (func != NULL) {
#ifndef NO_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (setjmp(g_fpe_env)==0) {
#endif /* NO_SIGNAL_TRAPS */
          (*func)(vars,gradient,residual);
#ifndef NO_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* NO_SIGNAL_TRAPS */
          return 0;
#ifndef NO_SIGNAL_TRAPS
        } else {
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
          return 1;
        }
#endif /* NO_SIGNAL_TRAPS */
      }
      return 1;
    }
  case BT_F77: {
      struct TableF *ftable;
      BinTokenSPtr subroutine;
      ftable = (struct TableF *)g_bt_data.tables[btable].tu;
      assert(ftable != NULL);
      if (bindex < 1 || bindex > g_bt_data.tables[btable].size) {
        return 1;
      }
      subroutine = ftable[0].S; /* its all in func 0 */
      if (subroutine != NULL) {
        int ForG,status;
        ForG = BinTokenGRADIENT;
#ifndef NO_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (setjmp(g_fpe_env)==0) {
#endif /* NO_SIGNAL_TRAPS */
          (*subroutine)(vars,gradient,residual,&ForG,&bindex,&status);
#ifndef NO_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* NO_SIGNAL_TRAPS */
          return status;
#ifndef NO_SIGNAL_TRAPS
        } else {
          status = 1;
        }
        Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
        return status;
#endif /* NO_SIGNAL_TRAPS */
      }
      return 1;
    }
  case BT_SunJAVA:
  case BT_MsJAVA:
  default:
    return 1;
  }
}

#if TESTBT /* this code may be out of date, but should be saved. */

FILE *g_ascend_errors = stderr;
int main() { /* built only if TESTBT defined TRUE in bintoken.c */
  double res;
  char *b[5];
  gl_init_pool();
  g_test_list = gl_create(5);
  gl_append_ptr(g_test_list,(void *)10);
  gl_append_ptr(g_test_list,(void *)20);
  gl_append_ptr(g_test_list,(void *)30);
  gl_append_ptr(g_test_list,(void *)40);
  gl_append_ptr(g_test_list,(void *)50);
  b[0]=(char *)ascmalloc(50);
  b[1]=(char *)ascmalloc(50);
  b[2]=(char *)ascmalloc(50);
  b[4]=(char *)ascmalloc(50);
  b[5]=(char *)ascmalloc(50);
  sprintf(b[0],"/tmp/btsrc.c");
  sprintf(b[1],"/tmp/btsrc.o");
  sprintf(b[2],"/tmp/btsrc.so");
  sprintf(b[3],"make -f foo/Makefile BTTARGET=/tmp/btsrc /tmp/btsrc");
  sprintf(b[4],"/bin/rm");
  BinTokenSetOptions(b[0],b[1],b[2],b[3],b[4],1000,1,0);
  BinTokensCreate((struct Instance *)1, BT_C);
  BinTokenCalcResidual(1,1,&res,&res);
  FPRINTF(ASCERR,"residual 1 = %g\n",res);
  BinTokenClearTables();
  gl_destroy(g_test_list);
  gl_destroy_pool();
  return 0;
}
#endif /* testbt */

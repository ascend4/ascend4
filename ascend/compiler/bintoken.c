/*  ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University
	Copyright (C) 1998 Carnegie Mellon University

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
*//*
	By Benjamin A. Allan
	Jan 7, 1998.
	Last in CVS:$Revision: 1.12 $ $Date: 1998/06/16 16:38:36 $ $Author: mthomas $
*/

#if 0
TIMESTAMP = -DTIMESTAMP="\"by `whoami`@`hostname`\""
#endif
/*
 * binary tokens implementation for real relation instances.
 * much of this goes in bintoken.h.
 */

#include <ascend/utilities/config.h>
#include "bintoken.h"

#include <unistd.h> /* for getpid() */

#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascPrint.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/ascDynaLoad.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>
#include <ascend/general/pretty.h>
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascEnvVar.h>

#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statio.h"
#include "instquery.h"
#include "instance_io.h"
#include "relation_io.h"
#include "find.h"
#include "rel_blackbox.h"
#include "relation.h"
#include "relation_util.h"
#include "mathinst.h"
/* last */

/* why do we include btprolog here? */
#include <ascend/bintokens/btprolog.h>

//#define BINTOKEN_DEBUG
#ifdef BINTOKEN_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

#define C_INDENT 4
#define C_WIDTH 70
#define CLINE(a) FPRINTF(fp,"%s\n",(a))

#define CMDMAX (3*PATH_MAX)

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
  int refcount; /* total number of relation shares with btable = our number */
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
 *  In the C++ interface, the arguments of BinTokenSetOptions need to be
 *  const char*. But here, new can lose its constness in *ptr = new. - JP
*/
static
int bt_string_replace(CONST char *new, char **ptr){
  if(*ptr == new){
    /* no destination specified */
    return 0;
  }
  if(new == NULL){
    /* free the current value */
    if(*ptr != NULL) {
      ASC_FREE(*ptr);
      *ptr = NULL;
    }
  }else{
    /* free then reallocate */
    if(*ptr != NULL){
      ASC_FREE(*ptr);
    }
    *ptr = ASC_NEW_ARRAY(char,strlen(new)+1);
    strcpy(*ptr,new);
  }
  return 0;
}
#if 1
int BinTokenSetOptionsDefault(){
#ifdef WIN32
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented for Windows");
  return 1;
//# error "Not implemented"
#else
  char srcn[PATH_MAX];
  char libn[PATH_MAX];
  snprintf(srcn,PATH_MAX,"/tmp/ascend-btsrc-%d.c",getpid());
  snprintf(libn,PATH_MAX,"/tmp/ascend-btsrc-%d.so",getpid());

#define BINTOK_NOMAKEFILE
  /* this approach calls GCC directly */
#ifdef BINTOK_NOMAKEFILE
  env_import_default(ASC_ENV_BTINC,getenv,Asc_GetEnv,Asc_PutEnv,ASC_DEFAULT_BTINC,0,1);
  env_import_default(ASC_ENV_BTLIB,getenv,Asc_GetEnv,Asc_PutEnv,ASC_DEFAULT_BTLIB,0,1);

  char buildtmpl[CMDMAX];
  snprintf(buildtmpl,CMDMAX
    ,"gcc -shared -fPIC -I$" ASC_ENV_BTINC " -o%s %s -L$" ASC_ENV_BTLIB " -lascend"
    ,libn,srcn
  );

  char *s1 = Asc_GetEnv(ASC_ENV_BTLIB);
  MSG("%s=%s",ASC_ENV_BTLIB,s1);
  ASC_FREE(s1);
#else
  /* this approach uses 'make' call instead. Turned off for now (incomplete implementation)*/

  /* Env var $ASCENDBTINCLUDE gives the location of the Makefile we use
  which will default to eg /usr/include/ascend/bintokens for normal 
  post-install execution, but can be overridden eg when testing. */
  env_import_default(ASC_ENV_BTINC,getenv,Asc_PutEnv,ASC_DEFAULT_BTINC,0);

  struct FilePath *fp1 = ospath_new_expand_env("$"ASC_ENV_BTINC"/Makefile",Asc_GetEnv,1);
  ospath_stat_t st; 
  char *s;
  if(ospath_stat(fp1,&st)){
    s = ospath_str(fp1);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"makefile '%s' does not exist",s);
    ASC_FREE(s);
  }
  ASC_FREE(fp1);
  char buildtmpl[CMDMAX];
  snprintf(buildtmpl,CMDMAX
    ,"make -f $" ASC_ENV_BTINC "/Makefile ASCBT_TARGET=\"%s\" ASCBT_SRC=\"%s\""
    ,libn,srcn
  );
#endif
  char *buildcmd = env_subst(buildtmpl,Asc_GetEnv,1);
  char rmcmd[] = "/bin/rm";
#ifdef BINTOKEN_DEBUG
  int res = BinTokenSetOptions(srcn,NULL,libn,buildcmd,rmcmd,1000,1/*verbose*/,0/*housekeep*/);
#else
  int res = BinTokenSetOptions(srcn,NULL,libn,buildcmd,rmcmd,1000,0/*verbose*/,1/*housekeep*/);
#endif
  ASC_FREE(buildcmd);
  return res;
#endif
}
#endif

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
  /*MSG("...");*/
  int err = 0;
  err += bt_string_replace(srcname,&(g_bt_data.srcname));
  err += bt_string_replace(objname,&(g_bt_data.objname));
  err += bt_string_replace(libname,&(g_bt_data.libname));
  err += bt_string_replace(buildcommand,&(g_bt_data.buildcommand));
  err += bt_string_replace(unlinkcommand,&(g_bt_data.unlinkcommand));
  g_bt_data.maxrels = maxrels;
  g_bt_data.verbose = verbose;
  g_bt_data.housekeep = housekeep;
  MSG("make command = %s",buildcommand);
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
      ASC_NEW_ARRAY(struct bt_table,20);
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
	frees global memory.
*/
void BinTokenClearTables(void)
{
  if (g_bt_data.tables != NULL) {
    ASC_FREE(g_bt_data.tables);
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
  if(btable < 0 || btable > g_bt_data.nextid 
    || g_bt_data.tables[btable].type == BT_error
  ){
    return; /* relation references a loadfailure library or already deleted
    or corrupted memory has made its way here.*/
  }
  g_bt_data.tables[btable].refcount--;
  if(g_bt_data.tables[btable].refcount == 0){
    /* unload the library if possible here */
#if HAVE_DL_UNLOAD
    MSG("Unloading btable=%d: %s",btable,g_bt_data.tables[btable].name);
    Asc_DynamicUnLoad(g_bt_data.tables[btable].name);

    if(g_bt_data.housekeep){
      if(g_bt_data.libname && strlen(g_bt_data.libname)){
          char *cbuf;
          cbuf = ASC_NEW_ARRAY(char,strlen(g_bt_data.unlinkcommand)+1+strlen(g_bt_data.libname)+1);
          assert(cbuf!=NULL);
          sprintf(cbuf,"%s %s",g_bt_data.unlinkcommand,g_bt_data.libname);
          MSG("Deleting bintok shared library: %s",cbuf);
          int rc = system(cbuf); /* we don't care if the delete fails */
	  if(rc){
            MSG("delete failed: %d",rc);
	  }
          ASC_FREE(cbuf);
      }
    }


#else
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Dynamic Unloading not available in this build");
#endif /* havedlunload */
    ASC_FREE(g_bt_data.tables[btable].name);
    g_bt_data.tables[btable].name = NULL;
    g_bt_data.tables[btable].tu = NULL;
    g_bt_data.tables[btable].type = BT_error;
  }else{
    MSG("Deleting one reference...");
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
  eql->rel2U = ASC_NEW_ARRAY(int,len+1);
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
        ASC_FREE(u->str);
      }
      ASC_FREE(u);
    }
  }
  gl_destroy(eql->ue);
  ASC_FREE(eql->rel2U);
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
    new = ASC_NEW(struct bintoken_unique_eqn);
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
    CLINE("/*\n\tAuto-generated code from" __FILE__);
    FPRINTF(fp,"\t%lu relations in instance '",len);
    WriteInstanceName(fp,root,NULL);
    CLINE("'\n\t(possibly fewer C functions required)\n*/");
  }
#ifdef HAVE_ERF
  /* need to define this for btprolog.h to do the right thing */
  CLINE("#define HAVE_ERF");
#endif

  CLINE("#include <ascend/bintokens/btprolog.h>");
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
    int nrel, int verbose, char *streqn, int timesused
){
  assert(i!=NULL);

  if (streqn==NULL) {
    return BTE_badrel;
  }

  if (verbose) {
    MSG("Writing residual code for rel %p",i);

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
#ifdef BINTOKEN_DEBUG
  FPRINTF(fp,"  fprintf(stderr,\"%%s:%%d: residual for '%%s' is %%f.\\n\", __FILE__, __LINE__, \"");
  WriteAnyInstanceName(fp,i);
  FPRINTF(fp,"\", *residual);\n");
#endif
  CLINE("}");
  return BTE_ok;
}

static
enum bintoken_error WriteGradientCode(FILE *fp, struct Instance *i,
    int nrel, int verbose, char *streqn, int timesused
){
  if (verbose) {
    MSG("Writing gradient code for rel %p (NOT IMPLEMENTED)",i);

    /* put in a little header */
    CLINE("\n/*");
    FPRINTF(fp,"\tGradients for\n",timesused);
    FPRINTF(fp,"\t");
    /* Use fastest path to a root */
    WriteAnyInstanceName(fp,i);
    CLINE("\n*/");
  }
  FPRINTF(fp,"void g_%d(double *x, double *resid, double *grad){\n",nrel);
  FPRINTF(fp,"\t*resid =");
  print_long_string(fp,streqn,C_WIDTH,C_INDENT); /* human readable, sort of */
  FPRINTF(fp,";\n");
  fprintf(fp,"\t/* gradient code missing */\n");
  FPRINTF(fp,"}\n\n");
  return BTE_write;
}


/*
 * t is the array of function pointers. size is number or
 * relations represented +1 since element 0 is {NULL,NULL}
 * by convention.
 */
int ExportBinTokenCTable(struct TableC *t,int size){
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
      ASC_FREE(r->rd.indices);
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
    ASC_FREE(r->rd.indices);
    r->rd.indices = NULL;
    newlen *= 2;
  }
  /* require min */
  if (newlen < 100) {
    newlen = 100;
  }
  /* create mem_*/
  r->rd.indices = ASC_NEW_ARRAY(int,newlen);
  if (r->rd.indices == NULL) {
    ASC_PANIC("out of memory error");
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
  error = ASC_NEW_ARRAY(int,len);
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
        ASC_FREE(str);
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
    WriteGradientCode(fp,i,eqn->indexU,verbose,eqn->str,eqn->refcount);
  }
  /* write the registered function name */
  pid = getpid();
  /** @TODO FIXME win32 has getpid but it is bogus as uniquifier. */
  /* so long as makefile deletes previous dll, windows is ok though */
  sprintf(g_bt_data.regname,"BinTokenArch_%d_%d",++(g_bt_data.nextid),(int)pid);
  FPRINTF(fp,"\n\nint ASC_EXPORT %s(){\n",g_bt_data.regname);
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
    MSG("Prepared %lu external C functions.\n",gl_length(eql.ue));
  }
  CLINE("}");

  ASC_FREE(error);
  DestroyEQData(&eql);
  fclose(fp);
  return BTE_ok;
}

static
enum bintoken_error BinTokenCompileC(char *buildcommand)
{
  int status;
  //ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Starting build, command:\n%s\n",buildcommand);
  status = system(buildcommand);
  if (status) {
    MSG("buildcommand: %s",buildcommand);
    MSG("...returned status %d",status);
    return BTE_build;
  }
  MSG("Build command returned OK, status=%d",status);
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
    error_reporter(ASC_PROG_WARNING,libname,0,"Failed to load library (init function %s)",regname);
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
  g_bt_data.tables[g_bt_data.nextid].name = ASC_STRDUP(libname);
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
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s: %s",filename, mess);
}

void BinTokensCreate(struct Instance *root, enum bintoken_kind method){
  struct gl_list_t *rellist;
  char *cbuf;
  enum bintoken_error status;
  char *srcname = g_bt_data.srcname;
  char *objname = g_bt_data.objname;
  char *libname = g_bt_data.libname;
  char *buildcommand = g_bt_data.buildcommand;
  char *unlinkcommand = g_bt_data.unlinkcommand;
  int verbose = g_bt_data.verbose;

  MSG("...");

  if (g_bt_data.maxrels == 0) {
#ifdef BINTOKEN_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_NOTE,"BinTokensCreate disabled (maxrels=0)\n");
#endif
    return;
  }
  if (srcname == NULL || buildcommand == NULL || unlinkcommand == NULL) {
#ifdef BINTOKEN_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"BinaryTokensCreate called with no options set: ignoring");
#endif
    return;
  }

  rellist =
    CollectTokenRelationsWithUniqueBINlessShares(root,g_bt_data.maxrels);
  if(rellist==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_WARNING
        ,"BinaryTokensCreate found 0 or too many unique relations."
    );
    return;
  }

  MSG("Creating bintokens");
  //MSG("buildcommand = %s",buildcommand);

  switch(method){
  case BT_C:
    /* generate code */
    status = BinTokenSharesToC(root,rellist,srcname,verbose);
    if(status != BTE_ok){
      BinTokenErrorMessage(status,root,srcname,buildcommand);
      break; /* leave source file there if partial */
    }
    status = BinTokenCompileC(buildcommand);
    if(status != BTE_ok){
      MSG("Writing error msg");
      BinTokenErrorMessage(status,root,srcname,buildcommand);
      break; /* leave source file there to debug */
    }else{
      MSG("BinTokenCompileC completed OK");
      if(g_bt_data.housekeep){
        /* trash src */
        cbuf = ASC_NEW_ARRAY(char,strlen(unlinkcommand)+1+strlen(srcname)+1);
        assert(cbuf!=NULL);
        sprintf(cbuf,"%s %s",unlinkcommand,srcname);
        int rc = system(cbuf); /* we don't care if the delete fails */
	if(rc){
          MSG("delete failed: %d",rc);
	}
        ASC_FREE(cbuf);
        /* trash obj */
        if(objname && strlen(objname)){
          cbuf = ASC_NEW_ARRAY(char,strlen(unlinkcommand)+1+strlen(objname)+1);
          assert(cbuf!=NULL);
          sprintf(cbuf,"%s %s",unlinkcommand,objname);
          int rc = system(cbuf); /* we don't care if the delete fails */
  	  if(rc){
           MSG("delete failed: %d",rc);
	  }
          ASC_FREE(cbuf);
        }
      }

      status = BinTokenLoadC(rellist,libname,g_bt_data.regname);
      if(status != BTE_ok){
        BinTokenErrorMessage(status,root,libname,buildcommand);
        /* leave source,binary files there to debug */
      }else{
        MSG("BinTokenLoadC completed OK");
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Binary tokens compiled and loaded.\n");
      }
    }
    break;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"BinaryTokensCreate called with unavailable method '%d'",(int)method);
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
int BinTokenCalcResidual(int btable, int bindex
    , double *vars, double *residual
){
  if(btable < 1 || bindex < 1){
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
        if (SETJMP(g_fpe_env)==0) {
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
#ifdef BINTOKEN_WITH_F77
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
#ifdef ASC_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (SETJMP(g_fpe_env)==0) {
#endif /* ASC_SIGNAL_TRAPS */
          (*subroutine)(vars,NULL,residual,&ForG,&bindex,&status);
#ifdef ASC_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* ASC_SIGNAL_TRAPS */
          return status;
#ifdef ASC_SIGNAL_TRAPS
        } else {
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
          return 1;
        }
#endif /* ASC_SIGNAL_TRAPS */
      }
      return 1;
    }
#endif
  default:
    return 1;
  }
}

/*
 * Returns nonzero if can't evaluate gradient.
 * Vars is assumed already filled with values.
 */
int BinTokenCalcGradient(int btable, int bindex
    ,double *vars,double *residual, double *gradient
){
  MSG("Calculating gradient...");
  if(btable == 0){
    MSG("btable is 0");
    return 1;
  }
  switch (g_bt_data.tables[btable].type) {
  case BT_error:
    MSG("expired table");
    return 1; /* expired table! */
  case BT_C: {
      MSG("C bintokens");
      /* signal handling needs to match func above. this is slow here. */
      struct TableC *ctable;
      BinTokenGPtr func;
      ctable = (struct TableC *)g_bt_data.tables[btable].tu;
      assert(ctable != NULL);
      if (bindex < 1 || bindex > g_bt_data.tables[btable].size) {
        return 1;
      }
      func = ctable[bindex].G;
      if(func != NULL){
        MSG("got gradient function");
#ifdef ASC_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (SETJMP(g_fpe_env)==0) {
#endif /* ASC_SIGNAL_TRAPS */
          (*func)(vars,gradient,residual);
#ifdef ASC_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* ASC_SIGNAL_TRAPS */
          return 0;
#ifdef ASC_SIGNAL_TRAPS
        } else {
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
          return 1;
        }
#endif /* ASC_SIGNAL_TRAPS */
      }else{
        MSG("no gradient function available");
      }
      return 1;
    }
#ifdef BINTOKEN_WITH_F77
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
#ifdef ASC_SIGNAL_TRAPS
        Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
        if (SETJMP(g_fpe_env)==0) {
#endif /* ASC_SIGNAL_TRAPS */
          (*subroutine)(vars,gradient,residual,&ForG,&bindex,&status);
#ifdef ASC_SIGNAL_TRAPS
          Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
#endif /* ASC_SIGNAL_TRAPS */
          return status;
#ifdef ASC_SIGNAL_TRAPS
        } else {
          status = 1;
        }
        Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
        return status;
#endif /* ASC_SIGNAL_TRAPS */
      }
      return 1;
    }
#endif
  default:
    return 1;
  }
}

#ifdef UNRELOCATE_TEST_BT
/* this code may be out of date, but should be saved. */
#ifdef RELOCATE_STREAMS
FILE *g_ascend_errors = stderr;
#endif

int main() { /* built only if TESTBT defined TRUE in bintoken.c */
  double res;
  gl_init_pool();
  g_test_list = gl_create(5);
  gl_append_ptr(g_test_list,(void *)10);
  gl_append_ptr(g_test_list,(void *)20);
  gl_append_ptr(g_test_list,(void *)30);
  gl_append_ptr(g_test_list,(void *)40);
  gl_append_ptr(g_test_list,(void *)50);
  BinTokenSetOptions(
    "/tmp/btsrc.c","/tmp/btsrc.o","/tmp/btsrc.so"
    ,"make -f foo/Makefile BTTARGET=/tmp/btsrc /tmp/btsrc"
    ,"/bin/rm"
    ,1000,1,0
  );
  BinTokensCreate((struct Instance *)1, BT_C);
  BinTokenCalcResidual(1,1,&res,&res);
  FPRINTF(ASCERR,"residual 1 = %g\n",res);
  BinTokenClearTables();
  gl_destroy(g_test_list);
  gl_destroy_pool();
  return 0;
}
#endif /*unrelocate test bt*/

/* vim: set ts=2 et: */


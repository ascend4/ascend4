/*
 *  Basic Initializations for Ascend
 *  by Ben Allan
 *  Version: $Revision: 1.20 $
 *  Version control file: $RCSfile: ascCompiler.c,v $
 *  Date last modified: $Date: 2000/01/25 02:25:57 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 *  This module initializes the fundamental data structures used by the rest of
 *  Ascend and pulls in system headers. Largely this means memory management.
 */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include "compiler.h"
#include "ascCompiler.h"
#include <utilities/ascSignal.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "types.h"
#include "name.h"
#include "exprs.h"
#include "sets.h"
#include "stattypes.h"
#include "slist.h"
#include "evaluate.h"
#include "units.h"
#include "symtab.h"
#include "notate.h"
#include "module.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "proc.h"
#include "typedef.h"
#include "value_type.h"
#include "temp.h"
#include "instance_enum.h"
#include "parpend.h"
#include "dump.h"
#include "prototype.h"
#include "library.h"
#include "instance_name.h"
#include "instquery.h"
#include "forvars.h"
#include "setinstval.h"
#include <general/pool.h>
#include "arrayinst.h"
#include "relation_type.h"
#include "relation_io.h"
#include "extfunc.h"
#include "find.h"
#include "relation.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "instantiate.h"
#include "universal.h"
#include "packages.h"
#include "interface.h"
#include "pending.h"
#include "scanner.h"
#include "simlist.h"
#include "numlist.h"
#include "bintoken.h"
#include "childio.h"
#include "redirectFile.h"
#include "ascCompiler.h"

#ifndef lint
static CONST char ascCompilerID[] = "$Id: ascCompiler.c,v 1.20 2000/01/25 02:25:57 ballan Exp $";
#endif


/*
 * this function is passed around by pointer in the compiler.
 */
/*
 * No longer necessary as being handled properly in instance.c. This
 * function was necessary before for the following reasons:
 * Let us assume that there as a simulation sim, and it had a root instance,
 * root. If refine was called on sim->root, and the instance was moved in
 * memory, we would be notified by this function, and so that if:
 * sim->root == old, then sim->root = new. This was only necessary for
 * sims, as they used to be treated *NOT* as instances and so sim->root had
 * 0 parents, with all the associated implications.
 * Now that simulations are proper SIM_INSTances, their root instance has
 * at *least* 1 parent, (possibly more if they are UNIVERSAL !*), and the
 * necessary fixing up is done within the code in instance.c.
 *
 * For the time being it is being left to see if it gets invoked.
 */
static
void InterfaceNotifyProc(char *ptr, struct Instance *old, struct Instance *new)
{
  (void)ptr;    /* stop gcc whine about unused parameter */
  (void)new;    /* stop gcc whine about unused parameter */
  (void)old;
#if 0 /* is, officially, no big deal. baa .*/
  register unsigned long c,len;
  register struct Instance *sptr;
  len = gl_length(g_simulation_list);
  for(c=len;c>=1;c--) {
    sptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationRoot(sptr) == old) {
      FPRINTF(stderr,
              "Simulation %s has been relocated for your information.\n",
              GetSimulationName(sptr));
      FPRINTF(stderr,"If you see this message please report it to\n");
      FPRINTF(stderr,"\t%s\n",ASC_BIG_BUGMAIL);
    }
  }
#endif
}

/*
 * this function is passed around by pointer in the compiler.
 */
/*
 * We don't have a use for this yet. We are being protractive
 * in that any objects that we are referencing directly get
 * cleaned up *before* we call any routines that could
 * potentially move an instance.
 */
static
void InterfacePtrDeleteProc(struct Instance *i, char *ptr)
{
  (void)i;     /* stop gcc whine about unused parameter */
  (void)ptr;   /* stop gcc whine about unused parameter */
}


/* if this function returns nonzero, ascend cannot run and a ton
 * of memory might be leaked while exiting.
 */
int Asc_CompilerInit(int simp)
{

 if (simp !=0){
   g_simplify_relations = 1;
 }
 Asc_RedirectCompilerDefault();
/* Commenting out the call to gl_init will make the system slower but
 * easier to debug for leaks.
 * Move the close-comment to after gl_init if debugging.
 * Please don't ever check in this file with gl_init commented out.
 */
  gl_init();
  gl_init_pool();
  Asc_SignalInit();
  Asc_InitEnvironment(10);
  name_init_pool();
  exprs_init_pool();
  ppe_init_pool();
  sets_init_pool();
  RelationIO_init_pool();
  InitSetManager();
  InitValueManager();
  InitRelInstantiator();
  InitLogRelInstantiator();
  InitPendingPool();
  InitInstanceNanny();

  InitDimenList();
  InitSymbolTable();
  InitBaseTypeNames();
  InitUnitsTable();
  InitNotesDatabase(NULL);
  InitNotesDatabase(LibraryNote());
  InitNotesDatabase(GlobalNote());
  InitExternalFuncLibrary();    /* initialize external func library */
  AddUserFunctions();           /* add user packages */

  InitDump();
  InitializePrototype();
  FPRINTF(ASCERR,"...PROTO INIT\n");
  InitializeLibrary();
  FPRINTF(ASCERR,"...LIBRARY INIT\n");
  SetUniversalTable(CreateUniversalTable());
  FPRINTF(ASCERR,"...UNIV TABLE INIT\n");
  DefineFundamentalTypes();
  FPRINTF(ASCERR,"...FUND TYPES INIT\n");

  InterfaceNotify = InterfaceNotifyProc;
  InterfacePtrDelete = InterfacePtrDeleteProc;
  g_simulation_list = gl_create(10L);
  FPRINTF(ASCERR,"...COMPILER INIT\n");

  return 0;
}

/* This function should not be called while there are any clients
 * with pointers to any compiler structures, including gl_lists.
 */
void Asc_CompilerDestroy(void)
{
  Asc_DestroySimulations();
  InterfaceNotify = NULL;
  InterfacePtrDelete = NULL;

  SetUniversalProcedureList(NULL);
  DestroyUniversalTable(GetUniversalTable());
  SetUniversalTable(NULL);
  EmptyTrash();
  WriteChildMissing(NULL,NULL,NULL);
  DestroyPrototype();
  DestroyLibrary();
  DestroyTemporaryList();
  DestroyNotesDatabase((void *)0x1); /* clear all that may be */
  DestroyUnitsTable();
  DestroyDimenList();
  Asc_DestroyModules((DestroyFunc)DestroyStatementList);
  DestroySymbolTable();
  DestroyStringSpace();
  Asc_DestroyScannerWorkBuffer();
  DestroyExtFuncLibrary();      /* deallocate external function nodes */

  /* some of the following calls are order dependent. see the headers.
   * In general, larger complex objects should be cleared before their
   * smaller constituents. 9 days in 10 you'll get away with mistakes here.
   */

  ClearForVarRecycle();         /* deallocate reused forvar list */
  DestroySetManager();          /* must not be done while Expr exist */
  DestroyValueManager();
  DestroyRelInstantiator();
  DestroyLogRelInstantiator();
  BinTokenClearTables();
  DestroyPendingPool();
  DestroyInstanceNanny();              /* empty the reusable array children */
  Asc_DestroyEnvironment();
  NumpairClearPuddle();
  RelationIO_destroy_pool();
  sets_destroy_pool();
  ppe_destroy_pool();
  exprs_destroy_pool();
  name_destroy_pool();
  Asc_SignalDestroy();
  gl_emptyrecycler();                  /* empty the reused list pool */
  gl_destroy_pool();                   /* empty the reused list head pool */
  ClearRecycleStack();                 /* empty the reused stack list  */
  Asc_DestroyScannerInputBuffer();	/* empty lexer */

}

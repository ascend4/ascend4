/*
 *  parpend.c
 *  by Ben Allan
 *  Jan 5, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: parpend.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:45 $
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

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/pool.h"
#include "compiler/fractions.h"
#include "compiler.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/instance_enum.h"
#include "compiler/parpend.h"


static pool_store_t g_ppe_pool = NULL;
/* global for our memory manager */
/* aim for small chunks including malloc overhead because max use
 * is not large.
 */
#define PPE_LEN 3
#if (SIZEOF_VOID_P == 8)
#define PPE_WID 4
#else
#define PPE_WID 4
#endif
/* retune rpwid if the size of struct name changes */
#define PPE_ELT_SIZE (sizeof(struct parpendingentry))
#define PPE_MORE_ELTS 1
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by PPE_MORE_ELTS*PPE_WID elements at a time.
 */
#define PPE_MORE_BARS 50
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

#define PPMALLOC ((struct parpendingentry *)(pool_get_element(g_ppe_pool)))
/* get a token. Token is the size of the struct struct parpendingentry */
#define PPFREE(p) (pool_free_element(g_ppe_pool,((void *)p)))
/* return a struct parpendingentry */

struct parpendingentry *CreatePPE(void)
{
  struct parpendingentry *new;
  new = PPMALLOC;
  if (new ==NULL) {
    Asc_Panic(2, NULL, "malloc fail in CreatePPE. Bye!\n");
  }
  return new;
}

void DestroyPPE(struct parpendingentry *old)
{
  PPFREE(old);
}

/* This function is called at compiler startup time and destroy at shutdown. */
void ppe_init_pool(void) {
  if (g_ppe_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: ppe_init_pool called twice.\n");
  }
  g_ppe_pool = pool_create_store(PPE_LEN, PPE_WID, PPE_ELT_SIZE,
    PPE_MORE_ELTS, PPE_MORE_BARS);
  if (g_ppe_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: ppe_init_pool unable to allocate pool.\n");
  }
}

void ppe_destroy_pool(void) {
  if (g_ppe_pool==NULL) return;
  pool_clear_store(g_ppe_pool);
  pool_destroy_store(g_ppe_pool);
  g_ppe_pool = NULL;
}

void ppe_report_pool(void)
{
  if (g_ppe_pool==NULL) {
    FPRINTF(ASCERR,"parpendingentrysPool is empty\n");
  }
  FPRINTF(ASCERR,"parpendingentrysPool ");
  pool_print_store(ASCERR,g_ppe_pool,0);
}


/*
 *  watchpt.c: An API to ascend methods
 *  by Benjamin Allan
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: watchpt.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:50 $
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

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPrint.h"
#include "general/list.h"
#include "compiler/instance_enum.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/module.h"
#include "compiler/library.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/watchpt.h"
#include "compiler/proc.h"
#include "compiler/procframe.h"
#include "compiler/types.h"
#include "compiler/initialize.h"

void Asc_SetMethodUserInterrupt(int val)
{
  g_procframe_stop = val;
}

/* If multiple breaks are hit in here with the same stopnum, they are
 * from the same event. We assume stopnum will rollover to 0 when
 * enough events occur. That is, integer overflow is assumed not to
 * throw an exception. On an integer overflow exception machine,
 * compile this file -D_OVERFLOW_EXCEPTION.
 * Not that this matters, as our pointer hashing functions will
 * all break on such a machine.
 */

int Asc_wp_stop_here(wpflags what)
{
  static unsigned long stopnum;
#ifdef _OVERFLOW_EXCEPTION
  if (stopnum > LONG_MAX) {
    stopnum = 0;
  }
#endif
  stopnum++;
  /* call in least specific first order and method/stat then inst order */
  if (what & (WP_LOGMULCALL | WP_UIFMULCALL)) Asc_wp_stop_recall();
  if (what & (WP_LOGEXT | WP_UIFEXT)) Asc_wp_stop_external();
  if (what & (WP_LOGPROC | WP_UIFPROC)) Asc_wp_stop_proc();
  if (what & (WP_LOGMULASGN | WP_UIFMULASGN)) Asc_wp_stop_reassign();
  if (what & (WP_LOGTYPE | WP_UIFTYPE)) Asc_wp_stop_type();
  if (what & (WP_LOGVAR | WP_UIFVAR)) Asc_wp_stop_var();
  if (what & (WP_LOGCOMP | WP_UIFCOMP)) Asc_wp_stop_compiler();
  if (what & (WP_LOGSTAT | WP_UIFSTAT)) Asc_wp_stop_stat();
/* blank line */
  return 0;
}

static
struct anywatch *CreateWatch(VOIDPTR key, unsigned long flags)
{
  struct anywatch *w;
  assert(flags != 0L);
  if (key == NULL) {
    return NULL;
  }
  w = (struct anywatch *)ascmalloc(sizeof(struct anywatch));
  if (w == NULL) {
    return NULL;
  }
  w->key = key;
  w->next = NULL;
  w->data = NULL;
  w->flags = flags;
  return w;
}

int Asc_WatchLeafName(watchlist *wl, char *leaf, enum wpdest wpd)
{
  struct namewatch *w;
  symchar *sc;
  unsigned long flags = WP_NAMEWATCH;
  if (wl == NULL || leaf == NULL) {
    return 1;
  }
  switch (wpd) {
  case wp_log:
    flags |= WP_LOGNAME;
    break;
  case wp_ui:
    flags |= WP_UIFNAME;
    break;
  case wp_both:
    flags |= (WP_UIFNAME | WP_LOGNAME);
    break;
  default:
    return 1;
  }
  sc = AddSymbol(leaf);
  w = (struct namewatch *)CreateWatch((VOIDPTR)sc,flags);
  if (w != NULL) {
    gl_append_ptr(wl,w);
    return 0;
  }
  return 2;
}

int Asc_WatchProc(watchlist *wl, char *type, char *meth, enum wpdest wpd)
{
  symchar *sc;
  struct procwatch *w;
  struct gl_list_t *init;
  struct InitProcedure *proc;
  struct TypeDescription *desc;
  unsigned long flags = WP_PROCWATCH;

  if (wl == NULL || type == NULL || meth == NULL) {
    return 1;
  }
  switch (wpd) {
  case wp_log:
    flags |= WP_LOGPROC;
    break;
  case wp_ui:
    flags |= WP_UIFPROC;
    break;
  case wp_both:
    flags |= (WP_UIFPROC | WP_LOGPROC);
    break;
  default:
    return 1;
  }
  sc = AddSymbol(type);
  desc = FindType(sc);
  if (desc==NULL) {
    return 1;
  }
  init = GetInitializationList(desc);
  if (init==NULL) {
    return 1;
  }
  sc = AddSymbol(meth);
  proc = SearchProcList(init,sc);
  if (proc == NULL) {
    proc = SearchProcList(GetUniversalProcedureList(),sc);
  }
  if (proc == NULL) {
    return 1;
  }
  w = (struct procwatch *)CreateWatch(proc,flags);
  if (w != NULL) {
    gl_append_ptr(wl,(VOIDPTR)w);
    return 0;
  }
  return 2;

}

int Asc_WatchInstance(watchlist *wl, struct Instance *var, enum wpdest wpd)
{
 (void)wl;
 (void)var;
 (void)wpd;
return 1;
}

int Asc_WatchStat(watchlist *wl, char *modname, int line, enum wpdest wpd)
{
 (void)wl;
 (void)modname;
 (void)line;
 (void)wpd;
return 1;
}

int Asc_WatchType(watchlist *wl, char *type, enum wpdest wpd)
{
 (void)wl;
 (void)type;
 (void)wpd;
return 1;
}

int Asc_WatchRefinements(watchlist *wl, char *type, enum wpdest wpd)
{
 (void)wl;
 (void)type;
 (void)wpd;
return 1;
}

int Asc_WatchGeneral(watchlist *wl, wpflags flags)
{
 (void)wl;
 (void)flags;
return 1;
}

void Asc_wp_stop_external(void)
{
  /* does nothing */
}

void Asc_wp_stop_name(void)
{
  /* does nothing */
}

void Asc_wp_stop_proc(void)
{
  /* does nothing */
}

void Asc_wp_stop_stat(void)
{
  /* does nothing */
}

void Asc_wp_stop_type(void)
{
  /* does nothing */
}

void Asc_wp_stop_var(void)
{
  /* does nothing */
}

void Asc_wp_stop_reassign(void)
{
  /* does nothing */
}

void Asc_wp_stop_recall(void)
{
  /* does nothing */
}

void Asc_wp_stop_compiler(void)
{
  /* does nothing */
}

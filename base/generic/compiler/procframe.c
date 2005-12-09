/*
 *  procframe.c: Method interpreter debugger stack frame information.
 *  by Benjamin Allan
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: procframe.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:47 $
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
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/slist.h"
#include "compiler/proc.h"
#include "compiler/instance_enum.h"
#include "compiler/watchpt.h"
#include "compiler/procframe.h"

/* The following goo all goes to slow down the method execution.
 * need to be smarter about these macros.
 */
#define FMALLOC ((struct procFrame *)ascmalloc(sizeof(struct procFrame)))
#define FMFREE(f) ascfree(f)

/* make a name for the path we took to get here. join is either "" or "." */
#define FMNCREATE(new,old,incr,join) \
  (new) = (char *)ascmalloc(((old!=NULL)?strlen(old):0)+strlen(incr)+ \
                            + strlen(join)+1); \
  sprintf((new),"%s%s%s",((old)!=NULL?(old):""),(join),(incr))
/* destroy said name when done with it. */
#define FMNFREE(n) ascfree(n)

/* These are the only legal wpflags if debugging is not active. */
#define WP_LEGALOPTS (WP_STOPONERR | WP_BTUIFSTOP | WP_UIFEXT | WP_UIFCOMP)

/* These are the legal wpflags if debugging is active. */
#define WP_DEBUGOPTS (WP_LEGALOPTS | WP_BTLOGSTOP | WP_LOGEXT | WP_LOGCOMP | \
  WP_LOGMULCALL | WP_UIFMULCALL | WP_LOGMULASGN | WP_UIFMULASGN)
/* other option bits can only be specified through the watchlist */

int g_procframe_stop = 0;

void InitNormalTopProcFrame(struct procFrame *fm, struct Instance *i,
                            char *cname, FILE *err, int options)
{
  assert(i !=NULL);
  assert(cname !=NULL);
  assert(err!=NULL); /* good design? */
  fm->i = i;
  fm->flow = FrameOK;
  fm->ErrNo = Proc_all_ok;
  fm->m = FrameNormal;
  fm->depth = 0;
  fm->cname = cname;
  fm->proc = NULL;
  fm->stat = NULL;
  fm->caller = NULL;
  fm->dbi = NULL;
  fm->locals = NULL;
  fm->gen = options & WP_LEGALOPTS;
  fm->err = err;
}

/* returns number of errors encountered. */
static
int WatchConfigureProcDebug(struct procFrame *fm, struct gl_list_t *wl)
{
  unsigned long c,len;
  struct procDebug *dbi;
  dbi = fm->dbi;
  assert(dbi!=NULL);
  len = gl_length(wl);
  for (c = 1; c <= len; c++) {
/* if something or other, set tab ptr and set flags and if needed
 * set data in hash tables. see watchpt.h and procframe.h.
 */
  }
  dbi->ntab = dbi->namehead;
  dbi->ptab = dbi->prochead;
  dbi->stab = dbi->stathead;
  dbi->ttab = dbi->typehead;
  dbi->vtab = dbi->varshead;
  return 1;
}

void InitDebugTopProcFrame(struct procFrame *fm, struct Instance *i,
                           char *cname, FILE *err, int options,
                           struct procDebug *dbi, struct gl_list_t *watches,
                           FILE *log)
{
  assert(i !=NULL);
  assert(cname !=NULL);
  assert(dbi !=NULL);
  fm->i = i;
  fm->flow = FrameOK;
  fm->ErrNo = Proc_all_ok;
  fm->m = FrameDebug;
  fm->depth = 0;
  fm->cname = cname;
  fm->proc = NULL;
  fm->stat = NULL;
  fm->caller = NULL;
  fm->dbi = dbi;
  fm->dbi->log = log;
  fm->locals = NULL;
  fm->err = err;
  fm->gen = options & WP_DEBUGOPTS;
  WatchConfigureProcDebug(fm,watches);
}

/*
 * Create a frame for a stack. parent should be the
 * containing frame unless this is the first frame in a
 * stack. Context is the instance statements are supposed
 * to be executed in/on. incrname is the name by which
 * we got here from last frame, or global name if there
 * is no parent frame. Proc should be the proc that will
 * be executed using the returned frame.
 * M is the trace mode which should be FrameDebug,
 * FrameNormal, or (IFF parent != NULL) FrameInherit.
 */
struct procFrame *AddProcFrame(struct procFrame *parent,
                               struct Instance *context,
                               char *incrname,
                               struct InitProcedure *proc,
                               enum FrameMode m)
{
  struct procFrame *fm;
  assert(context != NULL);
  assert(incrname != NULL);
  assert(proc != NULL);
  if (parent == NULL) {
    assert(m == FrameNormal || m == FrameDebug);
    fm = FMALLOC;
    assert(fm!=NULL);
    fm->m = m;
    fm->depth = 1;
    fm->err = ASCERR;
    fm->gen = 0; /* tell the user nothing...? */
    FMNCREATE(fm->cname,NULL,incrname,"");
  } else {
    fm = FMALLOC;
    assert(fm!=NULL);
    if (m != FrameInherit) {
      fm->m = m;
    } else {
      fm->m = parent->m;
    }
    fm->err = parent->err;
    fm->depth = parent->depth + 1;
    FMNCREATE(fm->cname,parent->cname,incrname,((context==parent->i)?"":"."));
    fm->gen = parent->gen;
  }
  fm->i = context;
  fm->proc = proc;
  fm->locals = NULL;
  fm->stat = NULL;
  fm->flow = FrameOK;
  fm->ErrNo = Proc_all_ok;
  fm->caller = parent;
  return fm;
}

/* set the statement for the given frame. statement had best be somewhere
 * in the proc list of the frame. i had better be the context the
 * statement is to be evaluated in if it is evaluated.
 */
void UpdateProcFrame(struct procFrame *fm, struct Statement *stat,
                     struct Instance *i)
{
  assert(fm != NULL);
  fm->stat = stat;
  fm->i = i;
}

void DestroyProcFrame(struct procFrame *fm)
{
  assert(fm != NULL);
  fm->m = FrameDestroyed;
  fm->i = NULL;
  fm->depth = -1;
  fm->proc = NULL;
  fm->stat = NULL;
  if (fm->locals != NULL) {
    gl_destroy(fm->locals);
    /* elements of list assumed already destroyed. */
  }
  FMNFREE(fm->cname);
  fm->cname = NULL;
  fm->caller = NULL;
  FMFREE(fm);
}

/* return a string (not caller's to free)  form of the enum given */
char *FrameControlToString(enum FrameControl frc)
{
  switch (frc) {
  case FrameOK:
    return "FrameOK";
  case FrameError:
    return "FrameError";
  case FrameBreak:
    return "FrameBreak";
  case FrameContinue:
    return "FrameContinue";
  case FrameFallthru:
    return "FrameFallthru";
  case FrameReturn:
    return "FrameReturn";
  case FrameLoop:
    return "FrameLoop";
  default:
   return "FrameUnknownEnum";
  }
}
#if 0 /* junk */
/* the following are valid without debug mode active */
#define WP_STOPONERR    0x1	/* stop on errors */
#define WP_BTUIFSTOP    0x4	/* print backtrace to UI file on stopping */
#define WP_UIFEXT      0x20	/* print external calls/returns to UI file */
#define WP_UIFCOMP     0x80	/* print con/destructor calls to UI file */

/* the following require debugging active */
#define WP_BTLOGSTOP    0x8	/* print backtrace to log on stopping */
#define WP_LOGEXT      0x10	/* print external calls/returns to log */
#define WP_LOGCOMP     0x40	/* print con/destructor calls to log */
#define WP_LOGMULCALL 0x100	/* print recalls of any proc/context to log */
#define WP_UIFMULCALL 0x200	/* print recalls of any proc/context to UI */
#define WP_LOGMULASGN 0x400	/* print reassignments of any var to log */
#define WP_UIFMULASGN 0x800	/* print reassignments of any var to UI */

/* the following cannot be set from options, but are computed from watchlist */
#define WP_RESERVED 0xFFFF0000  /* internally computed bits affecting memory. */
#define WP_LOGPROC     0x10000	/* print method entries/returns to log */
#define WP_UIFPROC     0x20000	/* print method entries/returns to UI file */
#define WP_LOGSTAT     0x40000	/* print statement record to log */
#define WP_UIFSTAT     0x80000	/* print statement record to UI file */
#define WP_LOGVAR     0x100000	/* print specific instance assignments to log */
#define WP_UIFVAR     0x200000	/* print specific instance assignments to UI */
#define WP_LOGTYPE    0x400000	/* print assignment to vars of type to log */
#define WP_UIFTYPE    0x800000	/* print assignment of vars of type to UI */
#define WP_NAMEWATCH 0x1000000	/* name watches are defined */
#define WP_PROCWATCH 0x2000000	/* proc watches are defined */
#define WP_STATWATCH 0x4000000	/* stat watches are defined */
#define WP_TYPEWATCH 0x8000000	/* type watches are defined */
#define WP_VARSWATCH 0x10000000	/* single var watches are defined */
#define WP_WATCH2    0x20000000	/* reserved */
#define WP_WATCH4    0x40000000	/* reserved */
#define WP_DEBUGWATCH 0x80000000 /* if set, Asc_wp_stop_here activated */
#endif

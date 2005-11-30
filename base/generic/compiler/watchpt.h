/*
 *  watchpt.h: An API to ascend methods
 *  by Benjamin Allan                          
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: watchpt.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:51 $
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

/** @file
 *  An API to
 *  ascend methods via an interactive or external interface
 *  without knowing about ascend compiler internals.
 *  <pre>
 *  When #including watchpt.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "general/list.h"
 *         #include "compiler/instance_enum.h"
 *  </pre>
 */

#ifndef __WATCHPT_H_SEEN__
#define __WATCHPT_H_SEEN__

/** 
 * The most iterations in a WHILE loop we will tolerate.
 * @todo This limit should go away and a user interrupt facility 
 *       should be installed instead.
 */
#define WP_MAXTRIPS 100000

/** 
 * This is the ErrNo listing of method execution.
 * External binary package calls should return these, probably
 * by stuffing a pointer. Proc_Call* should line up with the
 * tcl convention mainly for convenience.
 */
enum Proc_enum {
  Proc_all_ok = -1,
  Proc_CallOK,
  Proc_CallError,     /** error occured in call */
  Proc_CallReturn,    /** request that caller return. we may ignore it */
  Proc_CallBreak,     /** break out of enclosing loop, if any */
  Proc_CallContinue,  /** skip to next iteration of loop if any */
  /* ascend loop control signals;
   * these do not affect implicit assignment loops.
   */
  Proc_break,
  Proc_continue,
  Proc_fallthru,
  Proc_return,
  Proc_stop,
  Proc_stack_exceeded,
  Proc_stack_exceeded_this_frame,
  Proc_case_matched,
  Proc_case_unmatched,
  /* ascend errors */
  Proc_case_undefined_value,
  Proc_case_boolean_mismatch,
  Proc_case_integer_mismatch,
  Proc_case_symbol_mismatch,
  Proc_case_wrong_index,
  Proc_case_wrong_value,
  Proc_case_extra_values,
  Proc_bad_statement,
  Proc_bad_name,
  Proc_for_duplicate_index,
  Proc_for_set_err,
  Proc_for_not_set,
  Proc_illegal_name_use,
  Proc_name_not_found,
  Proc_instance_not_found,
  Proc_type_not_found,
  Proc_illegal_type_use,
  Proc_proc_not_found,
  Proc_if_expr_error_typeconflict,
  Proc_if_expr_error_nameunfound,
  Proc_if_expr_error_incorrectname,
  Proc_if_expr_error_undefinedvalue,
  Proc_if_expr_error_dimensionconflict,
  Proc_if_expr_error_emptychoice,
  Proc_if_expr_error_emptyintersection,
  Proc_if_expr_error_confused,
  Proc_if_real_expr,
  Proc_if_integer_expr,
  Proc_if_symbol_expr,
  Proc_if_set_expr,
  Proc_if_not_logical,
  Proc_user_interrupt,
  Proc_infinite_loop,
  Proc_declarative_constant_assignment,
  Proc_nonsense_assignment,       /** bogus*/
  Proc_nonconsistent_assignment,  /** inconsistent */
  Proc_nonatom_assignment,
  Proc_nonboolean_assignment,
  Proc_noninteger_assignment,
  Proc_nonreal_assignment,
  Proc_nonsymbol_assignment,
  Proc_lhs_error,
  Proc_rhs_error,
  Proc_unknown_error
};

/**
 * Debugging information.
 */
enum wpdest {
  wp_err,   /** screw up. */
  wp_log,   /** output to go to a log file */
  wp_ui,    /** output to go to the ui-defined output */
  wp_both   /** output to go both log and ui */
};

/** watchlist data structur. */
typedef struct gl_list_t watchlist;

/** Unsigned int assumed to be >= 32 bit. i/o and memory control. */
typedef unsigned int wpflags;

/*
 * Define boolean debug i/o options. watchpoint info wpflags.
 */
#define WP_EVERYTHING 0xFFFFFFFF  /**< really dumb thing to set. most possible
                                       wpeverything can be used to mask off
                                       high bits of a long if needed. */
#define WP_STOPONERR    0x1   /**< stop on errors */
#define WP_BTUIFSTOP    0x4   /**< print backtrace to UI file on stopping */
#define WP_BTLOGSTOP    0x8   /**< print backtrace to log on stopping */
#define WP_LOGEXT      0x10   /**< print external calls/returns to log */
#define WP_UIFEXT      0x20   /**< print external calls/returns to UI file */
#define WP_LOGCOMP     0x40   /**< print con/destructor calls to log */
#define WP_UIFCOMP     0x80   /**< print con/destructor calls to UI file */
#define WP_LOGMULCALL 0x100   /**< print recalls of any proc/context to log */
#define WP_UIFMULCALL 0x200   /**< print recalls of any proc/context to UI */
#define WP_LOGMULASGN 0x400   /**< print reassignments of any var to log */
#define WP_UIFMULASGN 0x800   /**< print reassignments of any var to UI */
/* empty middle bits 0x1000-0x8000 */
/* Add others in the empty middle bits. high bits are not for UI consumption. */
/* specific flags not for use by UI clients */
#define WP_RESERVED 0xFFFF0000  /**< internally computed bits affecting memory. */
#define WP_LOGPROC     0x10000  /**< print method entries/returns to log */
#define WP_UIFPROC     0x20000  /**< print method entries/returns to UI file */
#define WP_LOGSTAT     0x40000  /**< print statement record to log */
#define WP_UIFSTAT     0x80000  /**< print statement record to UI file */
#define WP_LOGVAR     0x100000  /**< print specific instance assignments to log */
#define WP_UIFVAR     0x200000  /**< print specific instance assignments to UI */
#define WP_LOGTYPE    0x400000  /**< print assignment to vars of type to log */
#define WP_UIFTYPE    0x800000  /**< print assignment of vars of type to UI */
#define WP_NAMEWATCH 0x1000000  /**< name watches are defined */
#define WP_PROCWATCH 0x2000000  /**< proc watches are defined */
#define WP_STATWATCH 0x4000000  /**< stat watches are defined */
#define WP_TYPEWATCH 0x8000000  /**< type watches are defined */
#define WP_VARSWATCH 0x10000000 /**< single var watches are defined */
#define WP_LOGNAME   0x20000000 /**< print leaf name assignments to log */
#define WP_UIFNAME   0x40000000 /**< print leaf name assignments to UI */
/* there is no UI call to set DEBUGWATCH. you have to do it from
 * a debugger inside proctype.c!
 */
#define WP_DEBUGWATCH 0x80000000  /**< if set, Asc_wp_stop_here activated */


/**
 * This function sets the value of a global interrupt flag.
 * 0 is the normal running value.
 * Set it to 1 to stop as soon as safe.
 * It is automagically reset to 0 when the interpreter receives
 * the message.<br><br>
 *
 * This value may or may not have an interrupt effect on any external
 * packages being called from ascend methods.
 */
extern void Asc_SetMethodUserInterrupt(int value);


/** 
 * <!--  wl = Asc_CreateWatchList(size);                               -->
 * Return a watchlist *wl of size n.
 * Adding more entries to the list than anticipated at creation is ok.
 */
#define Asc_CreateWatchList(n) (gl_create(n))

/** 
 * <!--  Asc_WatchLeafName(wl,leafname,output);                        -->
 * Add a local name of var to watch for assignments to wl.
 * E.g. to watch all things named T being assigned via
 * that name, Asc_WatchLeafName(wl,"T",wp_log);
 */
extern int Asc_WatchLeafName(watchlist *wl, 
                             char *leafname,
                             enum wpdest output);

/** 
 * <!--  Asc_WatchProc(wl,typename,procname,output);                   -->
 * Add a method named in type named to watch for execution to wl.
 * E.g. to watch all things named T being assigned via
 * that name, Asc_WatchProc(wl,"column","scale",wp_log);
 */
extern int Asc_WatchProc(watchlist *wl, char *type_name,
                         char *procname, enum wpdest output);

/** 
 * <!--  Asc_WatchInstance(wl,i,dest);                                 -->
 * Add a variable instance to watch for assignments to the watchlist.
 */
extern int Asc_WatchInstance(watchlist *wl,
                             struct Instance *i,
                             enum wpdest dest);

/** 
 * <!--  Asc_WatchStat(wl,modulename,linenum,dest);                    -->
 * Add a statement to watch for execution to the watchlist.
 */
extern int Asc_WatchStat(watchlist *wl, char *modulename,
                         int linenum, enum wpdest dest);

/** 
 * <!--  Asc_WatchType(wl,typename);                                   -->
 * Add a type of var to watch for assignments to the watchlist.
 */
extern int Asc_WatchType(watchlist *wl, char *type_name, enum wpdest dest);

/** 
 * <!--  Asc_WatchRefinements(wl,typename);                            -->
 * Add a type of var to watch for assignments of to the watchlist.
 * type and all its refinements are watched.
 */
extern int Asc_WatchRefinements(watchlist *wl, char *type_name, enum wpdest dest);

/**
 * Specify boolean flags for general watchpoint options
 * not requiring specific data.
 * Calls to Asc_WatchGeneral() are cumulative on a watchlist.
 */
extern int Asc_WatchGeneral(watchlist *wl, wpflags flags);

/** macros to watch for redundant method calls generally */
#define Asc_LogRedundantCalls(wl) \
  Asc_WatchGeneral((wl),WP_LOGMULCALL)
#define Asc_UIRedundantCalls(wl) \
  Asc_WatchGeneral((wl),WP_UIFMULCALL)
#define Asc_RedundantCalls(wl) \
  Asc_WatchGeneral((wl),(WP_UIFMULCALL|WP_LOGMULCALL))

/* Other macros are not defined. the 3 above are designed
 * to give ui programmers the general idea for using wpflags.
 */

/*
 * The following functions are called whenever a watch point is hit,
 * so that C debuggers can put a breakpoint at ascend watched events.
 */

/**
 * Asc_wp_stop_here() is called for every watch point event,
 * if the controlling frame wpflags WP_DEBUGWATCH is set.
 * It calls also one or more of the following more specific
 * functions according to the wpflags value given.
 * Normally returns 0. If returns 1, the ascend method
 * being evaluated should return up the stack.
 * The only way for this function to return nonzero is to
 * be manually instructed so by something like gdb.<br><br>
 *
 * When more than one of the specific functions applies,
 * all will be hit approximately in order of increasing specificity.
 */
extern int Asc_wp_stop_here(wpflags flags);

/** Called when an external call is scheduled. */
extern void Asc_wp_stop_external(void);

/** Called when a watched leaf name instance is assigned. */
extern void Asc_wp_stop_name(void);

/** Called when a watched proc is entered. */
extern void Asc_wp_stop_proc(void);

/** Called when a named statement is scheduled. */
extern void Asc_wp_stop_stat(void);

/** Called when a var of watched type is assigned. */
extern void Asc_wp_stop_type(void);

/** Called when a named instance is assigned. */
extern void Asc_wp_stop_var(void);

/** Called when a var is reassigned. */
extern void Asc_wp_stop_reassign(void);

/** Called when a method is recalled on the same instance. */
extern void Asc_wp_stop_recall(void);

/** Called when a constructor/destroyer is scheduled. */
extern void Asc_wp_stop_compiler(void);

#endif /* __WATCHPT_H_SEEN__ */

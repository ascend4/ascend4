/*
 *  procframe.h: Method interpreter debugger stack frame information.
 *  by Benjamin Allan
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: procframe.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:49 $
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
 *  Method interpreter debugger stack frame information.
 *  The data structures in this header should not be passed
 *  through headers except inside gl_lists. This header
 *  should not be accessed except by the interpreter and
 *  interpreter io routines. watchpt.h is the header for
 *  client consumption.
 *  <pre>
 *  When #including procframe.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASC_PROCFRAME_H
#define ASC_PROCFRAME_H

/**	addtogroup compiler Compiler
	@{
*/

enum FrameMode {
  FrameInherit,   /**< Inherit previous mode when used in Add. */
  FrameNormal,    /**< Normal operations. Inheritable. */
  FrameDebug,     /**< Trace operations. Inheritable. */
  FrameStart,     /**< Not yet supported. Initiate something. */
  FrameStep,      /**< Not yet supported. Do next statement any scope. */
  FrameNext,      /**< Not yet supported. Do next instruction this scope. */
  FrameDestroyed  /**< Severe error if seen. */
};

/** 
 * Because error handling is user defined according to watchpt.h
 * the following enum is used internally.
 * External package functions
 * should return Proc_CallOK, Proc_CallError,
 * Proc_CallBreak, or Proc_CallContinue.
 * Error message output is affected by FrameControl.
 */
enum FrameControl {
  FrameOK = 0,    /**< Normal return and entry state. */
  FrameError,     /**< Error return. only the erroneous frame uses this. */
  FrameBreak,     /**< Break from an enclosing loop. If no enclosing loop
                       exists, will be passed up to calling frame. */
  FrameContinue,  /**< Skip to next iteration in an enclosing loop. If no
                       enclosing loop exists, will be passed up to calling frame. */
  FrameFallthru,  /**< Suppress break inside switch processing of cases. */
  FrameReturn,    /**< Frames above a FrameError or FrameReturn stop & return.
              	       Initialize morphs this back to a FrameError for client. */
  FrameLoop       /**< In the scope of a loop. Valid entry state. */
};
/* keep the above in sync with the internals of FrameControlToString below */

/**
 * some watch point structures for internal use only.
 * see watchpt.h for UI watchpoint input structures.
 */
struct anywatch {
  void *key;            /**< all watches hash on some sort of unique ptr key */
  void *next;           /**< all watches are in hash tables. */
  void *data;           /**< everyone wants to know something. */
  unsigned long flags;  /**< control bits */
};

struct namewatch {
  symchar *leafname;
  struct namewatch *next;
  struct gl_list_t *where;
  unsigned long flags;  /**< control bits */
};

struct procwatch {
  symchar *leafname;
  struct procwatch *next;
  struct gl_list_t *where;
  unsigned long flags;  /**< control bits */
};

struct statwatch {
  struct Statement *stat;
  struct statwatch *next;
  struct gl_list_t *where;
  unsigned long flags;  /**< control bits */
};

struct varswatch {
  struct Instance *var;     /**< watched var */
  struct varswatch *next;
  struct gl_list_t *where;  /**< list of statement/context pairs */
  unsigned long flags;      /**< control bits */
};

struct typewatch {
  struct TypeDescription *desc;     /**< watched var type */
  struct typewatch *next;
  struct TypeDescription *ancestor; /**< watched ancestor causing this */
  unsigned long flags;              /**< control bits */
};

/** 
 * Masks are always = 2^N -1 for some N. Such a mask
 * can be used in hashing a pointer into an array of size 2^N.
 * N should be even.
 * Until we run tests, hard to say what the masks should be.
 * Would expect that VWTMASK may need to be much larger when
 * debugging very large variables and monitoring for multiple
 * assignments.
 * The shift for a given mask 2^N-1 should be 32-(N+2).
 * Note that an excessive upper bound for N is probably 16
 * on a 32 bit hardware with memory considered to cost something.
 */
#define NWTMASK 255
#define PWTMASK 255
#define SWTMASK 255
#define TWTMASK 255
#define VWTMASK 255
#define NWTSHIFT 22
#define PWTSHIFT 22
#define SWTSHIFT 22
#define TWTSHIFT 22
#define VWTSHIFT 22
/** The idea here is that pointers (to persistent instances, names, types
 * and methods, but not stack instances) are unique keys during method
 * execution, so these can be hashed for more or less linear cost trace
 * operations. We're hashing things because it's really gross to use
 * interface ptrs.
 */
#define PTRMAGICHASH 1103515245
#define NWTINDEX(p) (((((long) (p))*PTRMAGICHASH) << NWTSHIFT) & NWTMASK)
#define PWTINDEX(p) (((((long) (p))*PTRMAGICHASH) << PWTSHIFT) & PWTMASK)
#define SWTINDEX(p) (((((long) (p))*PTRMAGICHASH) << SWTSHIFT) & SWTMASK)
#define TWTINDEX(p) (((((long) (p))*PTRMAGICHASH) << TWTSHIFT) & TWTMASK)
#define VWTINDEX(p) (((((long) (p))*PTRMAGICHASH) << VWTSHIFT) & VWTMASK)

struct procDebug {
  wpflags what;
  /**< what tells what tables are active. check it, not the table ptrs */
  FILE *log;                /**< where debugger output should go. probably same as err */
  int errcnt;               /**< total error messages issued while processing this stack */
  struct namewatch **ntab;	/**< hash table of leaf names being watched */
  struct procwatch **ptab;	/**< hash table of proc names being watched */
  struct statwatch **stab;	/**< hash table of statements being watched */
  struct typewatch **ttab;	/**< hash table of var types being watched */
  struct varswatch **vtab;	/**< hash table of variables being watched */
  /* above watch tables point at below data spaces unless no break points
   * of the classes involved are set. If a table is not used, its tab
   * pointer is left NULL instead of being assigned to its head.
   */
  struct namewatch *namehead[NWTMASK+1];
  struct procwatch *prochead[PWTMASK+1];
  struct statwatch *stathead[SWTMASK+1];
  struct typewatch *typehead[TWTMASK+1];
  struct varswatch *varshead[VWTMASK+1];
};

/**
 * a procFrame defines the stack information we may carry about while
 * executing a method call from the interface. It is for the
 * internal use of initialize.c only.
 */
struct procFrame {
  enum FrameMode m;           /**< 0 -> no -> rest of frame data empty */
  enum FrameControl flow;     /**< flow of control info */
  enum Proc_enum ErrNo;       /**< last status computed */
  int depth;                  /**< where on the stack. redundant. */
  FILE *err;                  /**< where interactive messages should be sent */
  struct Instance *i;         /**< scope proc is being executed in. */
  char *cname;                /**< name of scope by which we got here. */
  struct InitProcedure *proc; /**< proc being evaluated. */
  struct Statement *stat;     /**< statement being evaluated. */
  struct procFrame *caller;   /**< scope that lead here in execution.
                               * NULL if caller was a user interface.
                               */
  wpflags gen;                /**< some general debug options valid
                               * whether or not dbi == NULL.
                               */
  struct procDebug *dbi;      /**< points to debugging information which
                               * is shared by all frames in a stack since
                               * debugging is a global activity.
                               * The root frame should create this data.
                               * If NULL, no messaging at all.
                               */
  /* not yet supported: */
  struct gl_list_t *locals;   /**< local vars simulation list. */
};

/** 
 * Setting this to 1 causes a STOP at the next sensible moment.
 * UIs access this variable through watchpt.h only.
 */
extern int g_procframe_stop;

/** Return a string (not caller's to free).  Form of the enum given. */
extern char *FrameControlToString(enum FrameControl frc);
/*
 * Doing anything more than output during execution probably calls for
 * a real debugging thread or a most careful inside-out method
 * execution protocol.
 */

/** Init a top procFrame for normal execution with no debugging. */
extern void InitNormalTopProcFrame(struct procFrame *fm,
                                   struct Instance *i,
                                   char *cname,
                                   FILE *eff,
                                   int options);

/** Init a top procFrame for debugging execution. */
extern void InitDebugTopProcFrame(struct procFrame *fm,
                                  struct Instance *i,
                                  char *cname,
                                  FILE *err,
                                  int options,
                                  struct procDebug *dbi,
                                  struct gl_list_t *watches,
                                  FILE *dplog);

/** 
 * Add a frame
 *
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
extern struct procFrame *AddProcFrame(struct procFrame *parent,
                                      struct Instance *context,
                                      char *incrname,
                                      struct InitProcedure *proc,
                                      enum FrameMode m);

/** 
 * Update context info in a frame.
 * Set the statement for the given frame. Statement had best be somewhere
 * in the proc list of the frame. i had better be the context the
 * statement is to be evaluated in if it is evaluated.
 */
extern void UpdateProcFrame(struct procFrame *fm,
                            struct Statement *stat,
                            struct Instance *i);

/** iffy */
extern void DestroyProcFrame(struct procFrame *fm);

/* @} */

#endif  /* ASC_PROCFRAME_H */


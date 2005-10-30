/*
 *  Stack Module
 *  by Kirk A. Abbott
 *  Created December 18, 1994.
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: stack.h,v $
 *  Date last modified: $Date: 1998/06/16 15:47:44 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1994 Kirk Andre Abbott
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
 *  Stack Module.
 *
 *  Stacks are occasionally used in the implementation of a compiler
 *  and/or interpreter. This module (in the spirit of the list module)
 *  attempts to provide a generic stack implementation, based on a
 *  contiquous but expandable representation.
 *  It is much like gl_list, except that push and pop are defined
 *  and much of the list fanciness is eliminated.
 *  <pre>
 *  Requires:
 *        #include "utilities/ascConfig.h"
 *        #include "compiler/compiler.h"
 *  </pre>
 */

#ifndef __stack_h_seen__
#define __stack_h_seen__

/** Stack data structure. */
struct gs_stack_t {
  VOIDPTR	*data;          /**< The data. */
  unsigned long size;     /**< Number of items used. */
  unsigned long capacity; /**< Capacity of the stack. */
};

extern struct gs_stack_t *gs_stack_create(unsigned long capacity);
/**<
 *  Creates a new empty stack having the specified initial capacity.
 *  If the number of stack items later exceeds this size, the
 *  capacity will be expanded.  It is not crucial, but a good
 *  guess will increase the performance of this module.  There
 *  is an implementation-defined minimum stack size, so the actual
 *  initial capacity can be larger than the requested capacity.<br><br>
 *
 *  Destruction of the returned stack is the responsibility of the
 *  caller.  Use gs_destroy() to do this.
 *
 *  @param capacity The desired initial stack capacity.
 *  @return Returns a pointer to the new empty gs_stack_t.
 */

extern void gs_stack_clear(struct gs_stack_t *stack);
/**<
 *  Resets the stack to a *clean* state as if it had just been created.
 *  The stack will retain whatever capacity it had.  The items in the 
 *  stack are NOT deallocated.  The specified stack may not be NULL 
 *  (checked by assertion).<br><br>
 *
 *  @param stack The stack to clear (non-NULL).
 */

extern void gs_stack_destroy(struct gs_stack_t *stack, int dispose);
/**<
 *  Destroys a stack with optional deallocation of the stack items.
 *  The specified stack will be destroyed (its memory is returned to 
 *  free memory).  If dispose is non-zero, the items in the stack will
 *  be deallocated.  This is appropriate when the stack is considered
 *  to be storing pointers to data it owns.  Otherwise, dispose should
 *  be zero and the data pointers will not be deallocated.  In this 
 *  case the stored pointers are no longer available after calling this
 *  function, so copies of the pointers must exist somewhere to
 *  allow deallocation of the data.  The specified stack can be NULL.<br><br>
 *
 *  @param stack   A pointer to the gs_stack_t to destroy.
 *  @param dispose Non-zero for deallocation of stack data items,
 *                 0 to have the data preserved.
 */

extern void gs_stack_push(struct gs_stack_t *stack, VOIDPTR ptr);
/**<
 *  Pushes the specified ptr onto the stack.  The stack capacity will
 *  be expanded if necessary. One can push any pointer (including NULL)
 *  onto the stack by casting appropriately.  The specified stack may 
 *  not be NULL (checked by assertion).<br><br>
 *
 *  Example:                                                     <pre>
 *       struct data_t *item;
 *       struct gs_stack_t = gs_create(100L);
 *       item = (struct data_t)malloc(sizeof(struct data_t));
 *       * ....... various operations on item ........
 *       gs_stack_push(stack,(VOIDPTR)item);                     </pre>
 *
 *  @see gs_stack_pop().
 *  @param stack A pointer to the gs_stack_t to modify (non-NULL).
 *  @param ptr   Pointer to push onto stack.
 */

extern VOIDPTR gs_stack_pop(struct gs_stack_t *stack);
/**<
 *  Pops and returns the top most item from the stack. The stack
 *  size will be appropriately accounted for.  The return value is a
 *  (void *), so will need to be cast to the appropriate type before
 *  use.  If the stack is empty when this function is called, NULL
 *  will be returned.  The specified stack may not be NULL (checked
 *  by assertion).
 *  <pre>
 *  Example:
 *       struct data_t *item, *result;
 *       struct gs_stack_t = gs_create(100L);
 *       item = (struct data_t)malloc(sizeof(struct data_t));
 *       * ....... various operations on item ........
 *
 *       gs_stack_push(stack,(VOIDPTR)item);
 *       result = (struct data_t*)gs_stack_pop(stack);          </pre>
 *
 *  @see gs_stack_push().
 *  @param stack A pointer to the gs_stack_t to modify (non-NULL).
 *  @return Returns the top-most data pointer as a (void *).
 */

extern unsigned long gs_stack_size(CONST struct gs_stack_t *stack);
/**<
 *  Returns the current size of the stack.
 *  Zero is a valid size, meaning that the stack is empty.  The
 *  specified stack may not be NULL (checked by assertion).
 *
 *  @param stack A pointer to the gs_stack_t to query (non-NULL).
 *  @return Returns the number of data items on the stack.
 */

extern int gs_stack_empty(CONST struct gs_stack_t *stack);
/**<
 *  Indicates whether the stack contains any data items.
 *  Returns TRUE (i.e. a nonzero), if the stack is empty,
 *  FALSE (zero) otherwise.  The specified stack may not 
 *  be NULL (checked by assertion).
 *
 *  @param stack A pointer to the gs_stack_t to query (non-NULL).
 *  @return Returns non-zero if the stack is empty, 0 otherwise.
 */

extern void gs_stack_apply(struct gs_stack_t *stack, void (*func) (VOIDPTR));
/**<
 *  Executes the function func on all the members of the stack.
 *  It will always execute the function on the items from the bottom
 *  of the stack (first-in) to the top (last-in).  The function
 *  should handle NULL pointers as input gracefully.  Neither the
 *  specified stack nor the func may be NULL (checked by assertion).
 *
 *  @param stack The stack to apply func to (non-NULL).
 *  @param func The function to execute for each stack item.
 */

#endif /* __stack_h_seen__ */


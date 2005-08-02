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
 *  Stack Module
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

#ifndef __STACK_H_SEEN__
#define __STACK_H_SEEN__

/** Stack data structure. */
struct gs_stack_t {
  VOIDPTR	*data;          /**< The data. */
  unsigned long size;     /**< Number of items used. */
  unsigned long capacity; /**< Capacity of the stack. */
};

extern struct gs_stack_t *gs_stack_create(unsigned long capacity);
/**< 
 *  <!--  struct gs_stack_t *gs_stack_create(capacity);                -->
 *  <!--  unsigned long capacity;                                      -->
 *
 *  This function takes one argument which is the anticipated size of the
 *  stack. This size simply sets the initial capacity of the stack. If the
 *  stack size exceeds the capacity, the stack is expanded. The stack will
 *  be expanded in 50% increments of the capacity when the stack would have
 *  been *overpushed.* If the stack capacity is less than 16, the capacity
 *  will be increased by at least 8 units. All mallocaters have a minimum
 *  overhead, and it makes sense to get a bigger chunk.
 */

extern void gs_stack_clear(struct gs_stack_t *stack);
/**< 
 *  <!--  gs_stack_clear(stack);                                       -->
 *  <!--  struct gs_stack_t *stack;                                    -->
 *  This function accepts a stack and resets it to a clean state, as if the
 *  stack had just been created. If the stack had been automatically expanded,
 *  it will retain that stack capacity.
 */

extern void gs_stack_destroy(struct gs_stack_t *stack, int dispose);
/**< 
 *  <!--  gs_stack_destroy(stack,dispose);                             -->
 *  <!--  struct gs_stack_t *stack;                                    -->
 *  <!--  int dispose;                                                 -->
 *
 *  This procedure takes two arguments.  This first is a stack that is
 *  to be destroyed(whose memory is to be returned to free memory).
 *  items is a boolean type value.  If you want the items in the stack to
 *  also be deallocated then you should set dispose to a true value, otherwise
 *  it should be false.
 */

extern void gs_stack_push(struct gs_stack_t *stack, VOIDPTR ptr);
/**< 
 *  <!--  gs_stack_push(stack,ptr);                                    -->
 *  <!--  struct gs_stack_t *stack;                                    -->
 *  <!--  VOIDPTR ptr;                                                 -->
 *
 *  This function will push the given ptr onto the stack. It will expand
 *  the capacity of the stack if necessary. One can push any pointer onto
 *  the stack by casting appropriately.<br><br>
 *
 *  Example:                                                     <pre>
 *       struct data_t *item;
 *       struct gs_stack_t = gs_create(100L);
 *       item = (struct data_t)malloc(sizeof(struct data_t));
 *       * ....... various operations on item ........
 *       gs_stack_push(stack,(VOIDPTR)item);                     </pre>
 *
 *  The same would apply to the return value. See gs_stack_pop().
 */

extern VOIDPTR gs_stack_pop(struct gs_stack_t *stack);
/**< 
 *  <!--  gs_stack_pop(stack);                                         -->
 *  <!--  struct gs_stack_t *stack;                                    -->
 *
 *  This function will pop the top most item off the stack. The stack
 *  size will be appropriately accounted for. The return value must be
 *  appropriately cast as in the example below.<br><br>
 *
 *  Example:                                                    <pre>
 *       struct data_t *item, *result;
 *       struct gs_stack_t = gs_create(100L);
 *       item = (struct data_t)malloc(sizeof(struct data_t));
 *       * ....... various operations on item ........          
 *
 *       gs_stack_push(stack,(VOIDPTR)item);
 *       result = (struct data_t*)gs_stack_pop(stack);          </pre>
 *
 *  NOTE: If the stack is overpopped, i.e., the stack is empty at the time
 *  of the call of this function, NULL will be returned.
 */

extern unsigned long gs_stack_size(CONST struct gs_stack_t *stack);
/**< 
 *  <!--  unsigned long gs_stack_size(stack);                          -->
 *  <!--  const struct gs_stack_t *stack;                              -->
 *
 *  Returns the current size of the stack. Zero is a valid size, meaning
 *  that the stack is empty.
 */

extern int gs_stack_empty(CONST struct gs_stack_t *stack);
/**< 
 *  <!--  int gs_stack_empty(stack);                                   -->
 *  <!--  const struct gs_stack_t *stack;                              -->
 *
 *  Returns TRUE (i.e. a nonzero), if the stack is empty.
 *  Returns FALSE (zero) otherwise.
 */

extern void gs_stack_apply(struct gs_stack_t *stack, void (*func) (VOIDPTR));
/**<
 *  <!--  void gs_stack_apply(stack,func)                              -->
 *  <!--  struct gs_stack_t *stack;                                    -->
 *  <!--  void (*func) (VOIDPTR);                                      -->
 *
 *  This function will execute the function func on all items currently on
 *  the stack. It will execute the function on the stack items from the
 *  bottom of the stack to the top. The function must be able to handle
 *  NULL pointers as input gracefully. This is potentially useful for
 *  debugging.
 */

#endif /* __STACK_H_SEEN__ */


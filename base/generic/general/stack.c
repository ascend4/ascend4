/*
 *  Implementation of Stack Module
 *  Kirk A. Abbott
 *  Created Dec 20, 1994
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: stack.c,v $
 *  Date last modified: $Date: 1997/07/18 11:35:37 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#include<stdio.h>
#include<stdlib.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/stack.h"
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define STACKDEBUG 0
/* if STACKDEBUG != 0,  we will do a bunch of initializing to 0 */
static const unsigned long LOWCAPACITY = 2;
/* Shallowest stack we will allow. It must not be 0! */
static const unsigned long MIN_INCREMENT = 8;
/* Minimum number of elements to increase when expanding stack. */

struct gs_stack_t *gs_stack_create(unsigned long capacity)
{
  struct gs_stack_t *result;
  capacity = MAX(LOWCAPACITY, capacity);
  result = (struct gs_stack_t *)ascmalloc(sizeof(struct gs_stack_t));
  if (result) {
    result->size = 0;
    result->capacity = capacity;
    result->data = (VOIDPTR *)ascmalloc(capacity*sizeof(VOIDPTR));
    return result;
  }
  else{
    FPRINTF(ASCERR,"Unable to allocate memory for stack\n");
    return NULL;
  }
}

void gs_stack_clear(struct gs_stack_t *stack)
{
  asc_assert(stack!=NULL);
#if STACKDEBUG
  memset(stack->data,0,sizeof(void *)*stack->size);
#endif
  stack->size = 0;
}

void gs_stack_destroy(struct gs_stack_t *stack,int dispose)
{
  unsigned long c;
  unsigned long size;
  if (stack == NULL) return;
  if (dispose) {
    size = stack->size;
    for (c=0;c<size;c++) {
      ascfree(stack->data[c]);
#if STACKDEBUG
      stack->data[c] = NULL;
#endif
    }
  }
  ascfree(stack->data);
#if STACKDEBUG
  stack->data = NULL;
  stack->size = stack->capacity = 0;
#endif
  ascfree(stack);
}

static void gs_stack_expand(struct gs_stack_t *stack)
{
  unsigned long increment;
  increment = (stack->capacity*50)/100;
  if (increment < MIN_INCREMENT) increment = MIN_INCREMENT;
  stack->capacity += increment;
  if (!stack->capacity) {
    stack->capacity += increment;
    stack->data = (VOIDPTR *)ascmalloc(increment*sizeof(VOIDPTR));
  } else {
    stack->capacity += increment;
    stack->data =
      (VOIDPTR *)ascrealloc(stack->data,stack->capacity*sizeof(VOIDPTR));
  }
  asc_assert(stack->data!=NULL);
}

void gs_stack_push(struct gs_stack_t *stack, VOIDPTR ptr)
{
  asc_assert(NULL != stack);
  if (++(stack->size) > stack->capacity) /* expand stack capacity */
    gs_stack_expand(stack);
  stack->data[stack->size-1] = ptr;
}

VOIDPTR gs_stack_pop(struct gs_stack_t *stack)
{
  VOIDPTR result;
  asc_assert(NULL != stack);
  if (stack->size > 0) {
    result = stack->data[stack->size-1];
    stack->size--;
    return result;
  }
  else{	/* overpopped */
    FPRINTF(ASCERR,"gs_stack_pop called too often.\n");
    return NULL;
  }
}

unsigned long gs_stack_size(CONST struct gs_stack_t *stack)
{
  asc_assert(NULL != stack);
  return stack->size;
}

int gs_stack_empty(CONST struct gs_stack_t *stack)
{
  asc_assert(NULL != stack);
  return (stack->size==0);
}

void gs_stack_apply(struct gs_stack_t *stack, void (*func) (VOIDPTR))
{
  unsigned long size,c;
  asc_assert(NULL != stack);
  asc_assert(NULL != func);
  size = stack->size;
  for (c=0;c<size;c++) (*func)(stack->data[c]);
}

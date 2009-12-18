/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg, Thomas Guthrie Weidner Epperly
	Copyright (C) 1993 Joseph James Zaher
	Copyright (C) 1993, 1994 Benjamin Andrew Allan, Joseph James Zaher
	Copyright (C) 1996 Benjamin Andrew Allan, Kenneth Tyner
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Reverse Automatic Differentiation (AD) routines for ASCEND
	This module implements routines for Exact
	Automatic Differentiation support in Ascend.

	The ideas for methods and structures have been adapted from the book:

	  Andreas Griewank, "Evaluating Derivatives - Principles and Techniques of
	  Algorithmic Differentiation", SIAM, 2000.
*//*
	Author: Mahesh Narayanamurthi
	Written as part of GSOC 2009.
	http://ascendwiki.cheme.cmu.edu/User:Mnm87
*/


#include <math.h>
#include <ascend/utilities/ascMalloc.h>
#include <ascend/general/mathmacros.h>
#include <ascend/general/ltmatrix.h>
#include <ascend/general/dstring.h>
#include <ascend/utilities/ascPanic.h>
#include <ascend/utilities/ascConfig.h>
#include "reverse_ad.h"
#include "fractions.h"
#include "compiler.h"
#include "dimen.h"
#include "expr_types.h"
#include "symtab.h"
#include "relation_util.h"
#include "mathinst.h"
#include "relation_util.h"
#include "instquery.h"
#include "exprio.h"


/*----------- static function declarations & defines --------------*/
static void TapeList_set_current(TapeList *tapes, unsigned index);
static Element *TapeList_get_active_tape(TapeList *tapes);
static void TapeList_set_element(TapeList *tapes, Element *element);
static Element *TapeList_current_tape_insert(TapeList *tapes);
static Element *Element_insert(Element **element);
static Redouble MakeIndepv(double value,unsigned long sindex,Element *tape,enum Expr_enum expr_type);
static int ReturnSweep(Element *trace,int both_sides);
static int AccumulateAdjoints(Element *trace, double* gradients);
static int ReturnSweepSafe(Element *trace, int both_sides,enum safe_err *serr);
static int AccumulateAdjointsSafe(Element *trace,double *gradients,enum safe_err *serr);
static int check_inst_and_res(struct Instance *i, double *res);
static void PrintTape(Element *trace);
static Element* TapeRewind(Element *trace);
static Element* TapeRewindInitFwd(Element *trace,unsigned long var_index);
static int ReturnSweep2ndDeriv(Element *trace);
static int AccumulateDeriv2nd(Element *trace,double *deriv2nd,unsigned long num_var,int hessian_calc);
static int ReturnSweep2ndDerivSafe(Element *trace,enum safe_err *serr);
static int AccumulateDeriv2ndSafe(Element *trace,double *deriv2nd,unsigned long num_var,int hessian_calc,enum safe_err *serr);
static int TapeFree(Element* head);
/* extra stuff from relation_util.c, just copied temporarily -- JP */
#ifndef NDEBUG
# define CHECK_INST_RES(i,res,retval) if(!check_inst_and_res(i,res)){return retval;}
#else
# define CHECK_INST_RES(i,res,retval) ((void)0)
#endif

#define res_stack(s)    stacks[(s)]
#define res_copy(s,v)   s.ref=v.ref
#define res_stack_ref(s) (stacks[(s)].ref)
#define res_stack_val(s) (res_stack_ref(s)->val.val)


/*----------- externally callable stuff --------------------------------------*/

/*
	Regulate gradient and residual by Reverse AD. Not FPE safe.
*/
ASC_DLLSPEC Element* RelationEvaluateResidualGradientRev(CONST struct relation *r
		,double *residual
		,double *gradient
		,int second_deriv)
{
	unsigned long t;       /* the current term in the relation r */
	unsigned long num_var; /* the number of variables in the relation r */
	int lhs;               /* looking at left(=1) or right(=0) hand side of r */
	unsigned long stack_height; /* height of each stack */
	long s = -1;           /* the top position in the stacks */
	unsigned long length_lhs, length_rhs;
	CONST struct relation_term *term;
	CONST struct Func *fxnptr;
	unsigned long i;
	 
	TapeList tapes;
	Redouble *stacks;        /* the memory for the stacks */
	  
	Redouble temp;
	  
	Element *temp_element=NULL;

	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateResidualGradientRev");

	for(i=0; i<MAX_TAPE_COUNT;i++){
		tapes.tape[i]=NULL;
	}


	TapeList_set_current(&tapes,0);


	num_var = NumberVariables(r);
	length_lhs = RelationLength(r, 1);
	length_rhs = RelationLength(r, 0);

		
	/* initialize all derivatives to zero */
	if(gradient != NULL){
		for( i = 0; i < num_var; i++ ) gradient[i] = 0.0; // May not be needed as the array
	}											// is cleared befored being passed on

	if( (length_lhs + length_rhs) == 0 ) {
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Relation with no LHS and no RHS: returning residual 0");
		*residual = 0.0;
		return 0;
	}
	else {
		stack_height = 1 + MAX(length_lhs,length_rhs);
	}


	/* create the stacks */
	stacks = ASC_NEW_ARRAY(Redouble,stack_height);  
	if( stacks == NULL ) return NULL; //FIXME What to do on error?

	lhs = 1;
	t = 0;
	while(1) {
		if( lhs && (t >= length_lhs) ) {
			/* need to switch to the right hand side--if it exists */
			if( length_rhs ) {
				lhs = t = 0;
			}
			else {
				/* Set the pointers we were passed to the tops of the stacks.
				* We do not need to check for s>=0, since we know that
				* (length_lhs+length_rhs>0) and that (length_rhs==0), the
				* length_lhs must be > 0, thus s>=0
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = res_stack_val(s);
				temp_element->bar.val = 1.0;
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				*residual = temp_element->val.val;		
				break;
			}
		}
		else if( (!lhs) && (t >= length_rhs) ) {
			/* we have processed both sides, quit */
			if( length_lhs ) {
				/* Set the pointers we were passed to lhs - rhs
				* We know length_lhs and length_rhs are both > 0, since if
				* length_rhs == 0, we would have exited above.
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = res_stack_val(s-1)-res_stack_val(s);
				temp_element->bar.val = 1.0;
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				*residual = temp_element->val.val;
				break;
			}else{
				/* Set the pointers we were passed to -1.0 * top of stacks.
				* We do not need to check for s>=0, since we know that
				* (length_lhs+length_rhs>0) and that (length_lhs==0), the
				* length_rhs must be > 0, thus s>=0
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = -res_stack_val(s);
				temp_element->bar.val = 1.0;		// FIXME will this always be 1? or should it be -1 here?
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				*residual = temp_element->val.val;
				break;
			}
		}

		term = NewRelationTerm(r, t++, lhs);
		switch(RelationTermType(term)){
			case e_zero:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(0.0, -1,temp_element,e_zero);
				res_copy(res_stack(s),temp);
				break;
			case e_real:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermReal(term), -1,temp_element,e_real);
				res_copy(res_stack(s),temp);
				break;
			case e_int:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermInteger(term), -1,temp_element,e_int);
				res_copy(res_stack(s),temp);
				break;
			case e_var:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermVariable(r,term),TermVarNumber(term)-1,temp_element,e_var);
				res_copy(res_stack(s),temp);
				break;
			case e_plus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = res_stack_val(s-1)+res_stack_val(s);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_plus;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_minus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = res_stack_val(s-1)-res_stack_val(s);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_minus;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_times:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = res_stack_val(s-1)*res_stack_val(s);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_times;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_divide:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = res_stack_val(s-1)*(1/res_stack_val(s));
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_divide;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_uminus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = -res_stack_val(s);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_uminus;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				res_copy(res_stack(s), temp);
				break;
			case e_power:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = pow(res_stack_val(s-1),res_stack_val(s));
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_power;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_ipower:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = asc_ipow(res_stack_val(s-1),((int)res_stack_val(s)));
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_ipower;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_func:
				fxnptr = TermFunc(term);
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = FuncEval(fxnptr,res_stack_val(s));
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_func;
				temp_element->fxnptr = (struct Func*)fxnptr;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				res_copy(res_stack(s), temp);
				break;
			default:
				ASC_PANIC("Unknown relation term type");
				break;
		}
	}


	ReturnSweep(TapeList_get_active_tape(&tapes),length_lhs && length_rhs);

	if(stacks)
		ASC_FREE(stacks);	
	
	if(!second_deriv && gradient!=NULL){
		AccumulateAdjoints(TapeList_get_active_tape(&tapes),gradient);
	
// 		while(temp_element!=NULL){
// 			temp_element=TapeList_get_active_tape(&tapes)->next;
// 			ASC_FREE(TapeList_get_active_tape(&tapes));
// 			TapeList_set_element(&tapes, temp_element);
// 		}
		TapeFree(temp_element);
	}
	else{
		return TapeList_get_active_tape(&tapes);	
	}
	return NULL;
}


/*
	Evaluation of residual and gradient. FPE-safe version.
*/
ASC_DLLSPEC Element* RelationEvaluateResidualGradientRevSafe(CONST struct relation *r,
		double *residual,
		double *gradient,
  		int second_deriv,
		enum safe_err *serr)
{
	unsigned long t;       /* the current term in the relation r */
	unsigned long num_var; /* the number of variables in the relation r */
	int lhs;               /* looking at left(=1) or right(=0) hand side of r */
	unsigned long stack_height; /* height of each stack */
	long s = -1;           /* the top position in the stacks */
	unsigned long length_lhs, length_rhs;
	CONST struct relation_term *term;
	CONST struct Func *fxnptr;
	unsigned long i;
	 
	TapeList tapes;
	Redouble *stacks;        /* the memory for the stacks */
	  
	Redouble temp;
	  
	Element *temp_element=NULL;

	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateResidualGradientRevSafe");

	for(i=0; i<MAX_TAPE_COUNT;i++){
		tapes.tape[i]=NULL;
	}


	TapeList_set_current(&tapes,0);


	num_var = NumberVariables(r);
	length_lhs = RelationLength(r, 1);
	length_rhs = RelationLength(r, 0);
	

	/* initialize all derivatives to zero */
	if(gradient != NULL){
		for( i = 0; i < num_var; i++ ) gradient[i] = 0.0; // May not be needed as the array
	}											// is cleared befored being passed on
	
	if( (length_lhs + length_rhs) == 0 ) {
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Relation with no LHS and no RHS: returning residual 0");
		*residual = 0.0;
		return 0;
	}
	else {
		stack_height = 1 + MAX(length_lhs,length_rhs);
	}


	/* create the stacks */
	stacks = ASC_NEW_ARRAY(Redouble,stack_height); 
	if( stacks == NULL ) return NULL;	//FIXME What to do on error?

	lhs = 1;
	t = 0;
	while(1) {
		if( lhs && (t >= length_lhs) ) {
			/* need to switch to the right hand side--if it exists */
			if( length_rhs ) {
				lhs = t = 0;
			}
			else {
				/* Set the pointers we were passed to the tops of the stacks.
				* We do not need to check for s>=0, since we know that
				* (length_lhs+length_rhs>0) and that (length_rhs==0), the
				* length_lhs must be > 0, thus s>=0
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = res_stack_val(s);
				temp_element->bar.val = 1.0;
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				*residual = temp_element->val.val;		
				break;
			}
		}
		else if( (!lhs) && (t >= length_rhs) ) {
			/* we have processed both sides, quit */
			if( length_lhs ) {
				/* Set the pointers we were passed to lhs - rhs
				* We know length_lhs and length_rhs are both > 0, since if
				* length_rhs == 0, we would have exited above.
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = safe_sub_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 1.0;
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				*residual = temp_element->val.val;
				break;
			}
			else {
				/* Set the pointers we were passed to -1.0 * top of stacks.
				* We do not need to check for s>=0, since we know that
				* (length_lhs+length_rhs>0) and that (length_lhs==0), the
				* length_rhs must be > 0, thus s>=0
				*/
				temp_element = TapeList_current_tape_insert(&tapes);
				temp_element->val.val = -res_stack_val(s);
				temp_element->bar.val = 1.0;		// will this always be 1? or should it be -1 here?
				temp_element->expr_type = e_equal;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				*residual = temp_element->val.val;
				break;
			}
		}

		term = NewRelationTerm(r, t++, lhs);
		switch( RelationTermType(term) ) {
			case e_zero:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(0.0, -1,temp_element,e_zero);
				res_copy(res_stack(s),temp);
				break;
			case e_real:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermReal(term), -1,temp_element,e_real);
				res_copy(res_stack(s),temp);
				break;
			case e_int:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermInteger(term), -1,temp_element,e_int);
				res_copy(res_stack(s),temp);
				break;
			case e_var:
				s++;
				temp_element = TapeList_current_tape_insert(&tapes);
				temp = MakeIndepv(TermVariable(r,term),TermVarNumber(term)-1,temp_element,e_var);
				res_copy(res_stack(s),temp);
				break;
			case e_plus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_add_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_plus;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_minus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_sub_D0(res_stack_val(s-1), res_stack_val(s), serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_minus;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_times:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_mul_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_times;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_divide:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_div_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_divide;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_uminus:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = -res_stack_val(s);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_uminus;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				res_copy(res_stack(s), temp);
				break;
			case e_power:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_pow_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_power;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_ipower:
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = safe_ipow_D0(res_stack_val(s-1),res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_ipower;
				temp_element->arg1 = res_stack_ref(s-1);
				temp_element->arg2 = res_stack_ref(s);
				res_copy(res_stack(s-1), temp);
				s--;
				break;
			case e_func:
				fxnptr = TermFunc(term);
				temp_element = TapeList_current_tape_insert(&tapes);
				temp.ref = temp_element;
				temp_element->val.val = FuncEvalSafe(fxnptr,res_stack_val(s),serr);
				temp_element->bar.val = 0.0;
				temp_element->expr_type = e_func;
				temp_element->fxnptr = fxnptr;
				temp_element->arg1 = res_stack_ref(s);
				temp_element->arg2 = NULL;
				res_copy(res_stack(s), temp);
				break;
			default:
				ASC_PANIC("Unknown relation term type");
				break;
		}
	}

	ReturnSweepSafe(TapeList_get_active_tape(&tapes),length_lhs && length_rhs,serr);

	if(stacks)
		ASC_FREE(stacks);
	
	if(!second_deriv && gradient!=NULL){	
		AccumulateAdjointsSafe(TapeList_get_active_tape(&tapes),gradient,serr);
		
// 		while(temp_element!=NULL){
// 			temp_element=TapeList_get_active_tape(&tapes)->next;
// 			ASC_FREE(TapeList_get_active_tape(&tapes));
// 			TapeList_set_element(&tapes, temp_element);
// 		}
		TapeFree(temp_element);
	}
	else{
		return TapeList_get_active_tape(&tapes);
	}	
	return NULL;
}

/*--------------------- internal-use functions -------------------------------*/

/**
	Set the active tape in a list of tapes. If an invalid index is requested,
	ASCEND will abort.

	@param index integer - the index of the active tape
	@param tapes TapeList* - the list of avaiable tape pointers
*/
static void TapeList_set_current(TapeList *tapes, unsigned index){
	ASC_ASSERT_RANGE(index, 0, MAX_TAPE_COUNT);
	
	tapes->active=index; /* any addition or deletion of tape data will happen 
			at this index */
}

/**
	Get the active tape in a list of tapes

	@param tapes TapeList* - the list of available tape pointers
	@return the index of the active tape between 0 and MAX_TAPE_COUNT
*/
static Element *TapeList_get_active_tape(TapeList *tapes){
	return tapes->tape[tapes->active];
}

static void TapeList_set_element(TapeList *tapes, Element *element){
	tapes->tape[tapes->active] = element;
}

/**
	Insert a new element at start of stack, return the new element so that it
	can be filled with data.
*/
static Element *Element_insert(Element **element){
	Element *e = ASC_NEW(Element);
	e->next = *element;
	e->prev = NULL;
	if(*element != NULL){
		(*element)->prev = e;
	}
	*element = e;
	return e;
}

static Element *TapeList_current_tape_insert(TapeList *tapes){
	Element **head = tapes->tape + tapes->active;
	return Element_insert(head);
}

/** 
	Makes independent variables and constants to be pushed onto the stack 
	which are later used during the evaluation phase. The return value is pushed
	onto the stack and used for further function evaluation.

	@param value is the value of the variable which is got from the hashmap
	@param sindex field is included to distinguish one variable from another
	and is later used during accumulation of partial adjoints. 
	@return an object of type redouble which is pushed onto the 
	stack and used during the evaluation phase.
*/
static Redouble MakeIndepv(double value,unsigned long sindex,Element *tape
		,enum Expr_enum expr_type
){
	Redouble temp;
	temp.ref = tape;
	tape->val.val = value;
	tape->bar.val = 0.0;
	tape->expr_type = expr_type;
	tape->sindex = sindex; 
	tape->arg1 = NULL;
	tape->arg2 = NULL;
	return temp;
}



/**
	Calculate the adjoints during the return sweep of the 
	trace information that has been built

	@param trace is the pointer to the tape on which the trace has been logged
	@return Not Significant
*/
static int ReturnSweep(Element* trace, int both_sides){
	double deriv1;
	double deriv2;
	double arg1val;
	double arg2val;
	double combibar;
	Element* tracer = trace;

	while(tracer!=NULL){
		switch(tracer->expr_type){
			case e_plus:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val += combibar;
				tracer->arg2->bar.val += combibar;
				break;
			case e_minus:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val += combibar;
				tracer->arg2->bar.val -= combibar;
				break;
			case e_times:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				tracer->arg1->bar.val += combibar * arg2val;
				tracer->arg2->bar.val += combibar * arg1val;
				break;
			case e_divide:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				deriv1 = 1/arg2val;
				deriv2 = -arg1val / pow(arg2val,2);
				tracer->arg1->bar.val += combibar * deriv1;
				tracer->arg2->bar.val += combibar * deriv2;
				break;
			case e_uminus:
				combibar = tracer->bar.val;
				(tracer->arg1->bar.val) -= combibar;
				break;
			case e_power:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				deriv1 = arg2val*pow(arg1val,arg2val-1);
				deriv2 = pow(arg1val,arg2val) * log(arg1val);
				tracer->arg1->bar.val += combibar * deriv1;
				tracer->arg2->bar.val += combibar * deriv2;
				break;
			case e_ipower:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val=(int)(tracer->arg2->val.val);
				deriv1 = arg2val * pow(arg1val, arg2val - 1);
				deriv2 = pow(arg1val,arg2val) * log(arg1val);
				tracer->arg1->bar.val += combibar * deriv1;
				tracer->arg2->bar.val += combibar * deriv2;
				break;
			case e_func:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				deriv1 = FuncDeriv(tracer->fxnptr, arg1val); // Look up function
				tracer->arg1->bar.val += combibar * deriv1;
				break;
			case e_equal:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val += combibar;
				if(both_sides)   // relation length is non zero for both lhs and rhs
					tracer->arg2->bar.val -= combibar;
				break;
			default:
				break;
		}
		tracer=tracer->next;
	}
	return 0;
}

/**
	Accumulates the adjoints of a particular independent variables
	@param trace is the tape on which the Adjoint information is recorded
	@param gradients is the array of gradients 
	@return Not Significant
*/
static int AccumulateAdjoints(Element* trace,double* gradients){
	Element* tracer = trace;
	if(gradients==NULL){
		//CONSOLE_DEBUG("Gradients is NULL");
		return 1;
	}
	
	if(trace==NULL){
		//CONSOLE_DEBUG("Trace is NULL");
		return 1;	  }
	
		while(tracer!=NULL){
			switch(tracer->expr_type){
				case e_var:
					gradients[tracer->sindex]+=tracer->bar.val;
					break;
				default:
					break;
			}
			tracer=tracer->next;
		}
		return 0;
}

/**
	Calculate the adjoints during the return sweep of the 
	trace information that has been built

	@param trace is the pointer to the tape on which the trace has been logged
	@return Not Significant

	@note Safe Version
*/
static int ReturnSweepSafe(Element* trace, int both_sides,enum safe_err *serr){
	double deriv1;
	double deriv2;
	double arg1val;
	double arg2val;
	double safe_temp1;
	double safe_temp2;
	double combibar;

	Element* tracer = trace;


	while(tracer!=NULL){
		switch(tracer->expr_type){
			case e_plus:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val,combibar,serr);
				tracer->arg2->bar.val = safe_add_D0(tracer->arg2->bar.val,combibar,serr);
				break;
			case e_minus:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val,combibar,serr);
				tracer->arg2->bar.val = safe_sub_D0(tracer->arg2->bar.val,combibar,serr);
				break;
			case e_times:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				safe_temp1 = safe_mul_D0(combibar,arg2val,serr);
				safe_temp2 = safe_mul_D0(combibar,arg1val,serr);
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val,safe_temp1,serr);
				tracer->arg2->bar.val = safe_add_D0(tracer->arg2->bar.val,safe_temp2,serr);
				break;
			case e_divide:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				deriv1 = safe_rec(arg2val,serr);
				deriv2 = safe_div_D0((-arg1val),(safe_pow_D0(arg2val,2,serr)),serr);
				safe_temp1 = safe_mul_D0(combibar,deriv1,serr);
				safe_temp2 = safe_mul_D0(combibar,deriv2,serr);
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val,safe_temp1,serr);
				tracer->arg2->bar.val = safe_add_D0(tracer->arg2->bar.val,safe_temp2,serr);
				break;
			case e_uminus:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val = safe_sub_D0(tracer->arg1->bar.val, combibar,serr);
				break;
			case e_power:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val = tracer->arg2->val.val;
				deriv1 = safe_mul_D0(arg2val,safe_pow_D0(arg1val,arg2val-1,serr),serr);
				deriv2 = safe_mul_D0(safe_pow_D0(arg1val,arg2val,serr),safe_ln_D0(arg1val,serr),serr);
				safe_temp1 = safe_mul_D0(combibar,deriv1,serr);
				safe_temp2 = safe_mul_D0(combibar,deriv2,serr);
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val, safe_temp1, serr);
				tracer->arg2->bar.val = safe_add_D0(tracer->arg2->bar.val, safe_temp2, serr);
				break;
			case e_ipower:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				arg2val=(int)(tracer->arg2->val.val);
				deriv1 = safe_mul_D0(arg2val,safe_pow_D0(arg1val,arg2val-1,serr),serr);
				deriv2 = safe_mul_D0(safe_pow_D0(arg1val,arg2val,serr),safe_ln_D0(arg1val,serr),serr);
				safe_temp1 = safe_mul_D0(combibar,deriv1,serr);
				safe_temp2 = safe_mul_D0(combibar,deriv2,serr);
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val, safe_temp1, serr);
				tracer->arg2->bar.val = safe_add_D0(tracer->arg2->bar.val, safe_temp2, serr);
				break;
			case e_func:
				combibar = tracer->bar.val;
				arg1val = tracer->arg1->val.val;
				deriv1 = FuncDerivSafe(tracer->fxnptr,arg1val,serr); // Look up function
				safe_temp1 = safe_mul_D0(combibar,deriv1,serr);
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val, safe_temp1,serr);
				break;
			case e_equal:
				combibar = tracer->bar.val;
				tracer->arg1->bar.val = safe_add_D0(tracer->arg1->bar.val,combibar,serr);
				if(both_sides)   // relation length is non zero for both lhs and rhs
					tracer->arg2->bar.val = safe_sub_D0(tracer->arg2->bar.val,combibar,serr);
				break;
			default:
				break;
		}
		tracer=tracer->next;
	}
	return 0;
}


/**
	Accumulate the adjoins that have been calculated during ReturnSweep

	@param trace is the pointer to the tape on which the trace has been logged
	@param derivative is the array of derivatives
	@param serr is the safe_err object
	@return not significant yet
	@note safe version
*/
static int AccumulateAdjointsSafe(Element* trace,double* gradients,enum safe_err *serr){
	Element* tracer = trace;
	if(gradients==NULL){
		//CONSOLE_DEBUG("Gradients is NULL");
		return 1;
	}
	
	if(tracer==NULL){
		//CONSOLE_DEBUG("Trace is NULL");
		return 1;	  
	}
	
	while(tracer!=NULL){
		switch(tracer->expr_type){
			case e_var:
				gradients[tracer->sindex] = safe_add_D0(gradients[tracer->sindex],(((Element*)(tracer))->bar.val),serr);
				break;
			default:
				break;
		}
		tracer=tracer->next;
	}

	return 0;
}

/**
	Prints the contents of the tape for Debugging Purposes.
	@param trace is the tape on which the trace has been logged.
*/
static void PrintTape(Element *trace){
	Element *tracer = trace;
	while(tracer!=NULL){
		CONSOLE_DEBUG("-------------------------------------------");
		CONSOLE_DEBUG("Expression Type : %s", ExprEnumName(tracer->expr_type)); 
		CONSOLE_DEBUG("Element ID: %p",tracer);
		CONSOLE_DEBUG("SINDEX : %lu",tracer->sindex);
		CONSOLE_DEBUG("Val.Val = %g, Bar.Val = %g",tracer->val.val,tracer->bar.val);
		CONSOLE_DEBUG("Val.Dot = %g, Bar.Dot = %g",tracer->val.dot,tracer->bar.dot);
		if (tracer->arg1!=NULL)
		{
			CONSOLE_DEBUG("Argument 1 : %g", tracer->arg1->val.val);
			CONSOLE_DEBUG("Argument 1 ID: %p",tracer->arg1);
		}
		if (tracer->arg2!=NULL)
		{
			CONSOLE_DEBUG("Argument 2 : %g", tracer->arg2->val.val);
			CONSOLE_DEBUG("Argument 2 ID: %p",tracer->arg2);
		}
		CONSOLE_DEBUG("-------------------------------------------");
		tracer = tracer->next;
	}
}

/**----------------------Second Derivative Routines -----------------------------*/

int RelationEvaluateSecondDeriv(CONST struct relation *r,
								double *deriv2nd,
								unsigned long var_index,
  								int hessian_calc,
  								Element* tape)
{
	unsigned long num_var; /* the number of variables in the relation r */
	unsigned long i;
	double residual;
	Element	*grad_tape,*temp_tape;
	CONST struct Func *fxnptr;

	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateSecondDeriv");

	if(!hessian_calc && tape==NULL){
		grad_tape = RelationEvaluateResidualGradientRev(r
														,&residual
														,NULL
														,1);
		
//		CONSOLE_DEBUG("Printing the Contents of the Tape after evaluation of Gradients, Row Index : %lu",var_index);
	
//		PrintTape(grad_tape);

		if(grad_tape==NULL){
			ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Gradient Tape is NULL");
		}
		num_var = NumberVariables(r);
	}
	else{
		grad_tape = tape;
		num_var = var_index + 1;
	}
	
	for( i = 0; i < num_var; i++ ) deriv2nd[i] = 0.0;
	
	/** Repostion Tape here to the end of the tape list*/
	
	temp_tape = TapeRewindInitFwd(grad_tape,var_index);
	if(temp_tape == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Gradient Tape is NULL");
	}
	 
	
	/** Need to Initialize all Dot Components and Bar.Dot Components to zero*/
#define u temp_tape->arg1->val.val
#define v temp_tape->arg2->val.val
#define du temp_tape->arg1->val.dot
#define dv temp_tape->arg2->val.dot
	while(temp_tape != NULL) {
		switch(temp_tape->expr_type) {
			case e_zero:
			case e_real:
			case e_int:
			case e_var:
				break;
			case e_plus:
				/* d(u+v) = du + dv */
				temp_tape->val.dot = du + dv;
				break;
			case e_minus:
				/* d(u-v) = du - dv */
				temp_tape->val.dot = du - dv;
				break;
			case e_times:
				/* d(u*v) = u*dv + v*du */
				temp_tape->val.dot = (u * dv) + (v * du);
				break;
			case e_divide:
				/*  d(u/v) = du/v - u*dv/(v^2) = (1/v) * [du - (u/v)*dv]  */
				temp_tape->val.dot = (1/v) * (du - (u/v)*(dv));
				break;
			case e_uminus:
				temp_tape->val.dot = -(du);
				break;
			case e_power:
				/*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
				temp_tape->val.dot = v * pow(u,(v-1)) * du + FuncEval(LookupFuncById(F_LN),u) * pow(u,v) * dv;	
				break;
			case e_ipower:
				/*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
				/*  Finally, compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
				temp_tape->val.dot =  asc_d1ipow(u,((int)v)) * du + FuncEval(LookupFuncById(F_LN),u) * asc_ipow(u,v) * dv; // was multiplying yet another time by v arghh...
				break;
			case e_func:
      /*
				funcptr = TermFunc(term);
				for (v = 0; v < num_var; v++) {
				grad_stack(v,s) = FuncDeriv(funcptr, grad_stack(v,s));
		}
				res_stack(s) = FuncEval(funcptr, res_stack(s));  */
				fxnptr = temp_tape->fxnptr;
				temp_tape->val.dot = FuncDeriv(fxnptr, u) * du;
				break;
			case e_equal:
				if(temp_tape->arg2!=NULL){
					temp_tape->val.dot = du - dv;
				}
				else{
					temp_tape->val.dot = du;
				}
				break;
			default:
				ASC_PANIC("Unknown relation term type");
				break;
		}
		temp_tape = temp_tape->prev;
	}
#undef u
#undef v
#undef du
#undef dv
	
	ReturnSweep2ndDeriv(grad_tape);
	
	AccumulateDeriv2nd(grad_tape,deriv2nd,num_var,hessian_calc);

//	CONSOLE_DEBUG("Printing the Contents of the Tape after evaluation of Second Derivatives, Row Index : %lu",var_index);

//	PrintTape(grad_tape);	

	
	if(!hessian_calc && tape==NULL){
/*		while(grad_tape!=NULL){
			temp_tape = grad_tape;
			grad_tape =  grad_tape->next;
			ASC_FREE(temp_tape);
		}*/
		TapeFree(grad_tape);
	}
	
	return 0;
}

int RelationEvaluateSecondDerivSafe(CONST struct relation *r,
									double *deriv2nd,
									unsigned long var_index,
  									int hessian_calc,
  									Element* tape,
								    enum safe_err *serr)
{
	unsigned long num_var; /* the number of variables in the relation r */
	unsigned long i;
	double residual;
	Element	*grad_tape,*temp_tape;
	CONST struct Func *fxnptr;

	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateSecondDerivSafe");

	if(!hessian_calc && tape==NULL){
		grad_tape = RelationEvaluateResidualGradientRevSafe(r
															,&residual
															,NULL
															,1
														   	,serr);
		safe_error_to_stderr(serr);
	
//		CONSOLE_DEBUG("Printing the Contents of the Tape after evaluation of Gradients, Row Index : %lu",var_index);
	
//		PrintTape(grad_tape);

		if(grad_tape==NULL){
			ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Gradient Tape is NULL");
		}
		num_var = NumberVariables(r);
	}
	else{
		grad_tape = tape;
		num_var = var_index + 1;
	}
	
	for( i = 0; i < num_var; i++ ) deriv2nd[i] = 0.0;
	
	/** Repostion Tape here to the end of the tape list*/
	/** Need to Initialize all Dot Components and Bar.Dot Components to zero*/

	temp_tape = TapeRewindInitFwd(grad_tape,var_index);
	if(temp_tape == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Gradient Tape is NULL");
	}
	 
	
#define u temp_tape->arg1->val.val
#define v temp_tape->arg2->val.val
#define du temp_tape->arg1->val.dot
#define dv temp_tape->arg2->val.dot
	while(temp_tape != NULL) {
		switch(temp_tape->expr_type) {
			case e_zero:
			case e_real:
			case e_int:
			case e_var:
				break;
			case e_plus:
				/* d(u+v) = du + dv */
				temp_tape->val.dot = safe_add_D0(du,dv,serr);
				break;
			case e_minus:
				/* d(u-v) = du - dv */
				temp_tape->val.dot = safe_sub_D0(du,dv,serr);
				break;
			case e_times:
				/* d(u*v) = u*dv + v*du */
				temp_tape->val.dot = safe_add_D0(safe_mul_D0(u,dv,serr),safe_mul_D0(v,du,serr),serr);
				break;
			case e_divide:
				/*  d(u/v) = du/v - u*dv/(v^2) = (1/v) * [du - (u/v)*dv]  */
				temp_tape->val.dot = safe_mul_D0(safe_rec(v,serr),safe_sub_D0(du,safe_mul_D0(safe_div_D0(u,v,serr),dv,serr),serr),serr);
				break;
			case e_uminus:
				temp_tape->val.dot = -(du);
				break;
			case e_power:
				/*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
				temp_tape->val.dot = safe_add_D0(safe_mul_D0(safe_mul_D0(v,safe_pow_D0(u,v-1,serr),serr),du,serr), safe_mul_D0(safe_mul_D0(safe_ln_D0(u,serr),safe_pow_D0(u,v,serr),serr),dv,serr),serr);	
				break;
			case e_ipower:
				/*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
				/*  Finally, compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
				temp_tape->val.dot = safe_add_D0(safe_mul_D0(safe_mul_D0(v,safe_ipow_D0(u,v-1,serr),serr),du,serr), safe_mul_D0(safe_mul_D0(safe_ln_D0(u,serr),safe_ipow_D0(u,v,serr),serr),dv,serr),serr);	// Check for the samething here. are you twice multiplying by v as in the non-safe routine.
				break;
			case e_func:
      /*
				funcptr = TermFunc(term);
				for (v = 0; v < num_var; v++) {
				grad_stack(v,s) = FuncDeriv(funcptr, grad_stack(v,s));
		}
				res_stack(s) = FuncEval(funcptr, res_stack(s));  */
				fxnptr = temp_tape->fxnptr;
				temp_tape->val.dot = safe_mul_D0(FuncDerivSafe(fxnptr, u,serr),du,serr);
				break;
			case e_equal:
				if(temp_tape->arg2!=NULL){
					temp_tape->val.dot = safe_sub_D0(du,dv,serr);
				}
				else{
					temp_tape->val.dot = du;
				}
				break;
			default:
				ASC_PANIC("Unknown relation term type");
				break;
		}
		temp_tape = temp_tape->prev;
	}
#undef u
#undef v
#undef du
#undef dv
	
	ReturnSweep2ndDerivSafe(grad_tape,serr);
	
	AccumulateDeriv2ndSafe(grad_tape,deriv2nd,num_var,hessian_calc,serr);

//	CONSOLE_DEBUG("Printing the Contents of the Tape after evaluation of Second Derivatives, Row Index : %lu",var_index);

//	PrintTape(grad_tape);	

	if(!hessian_calc && tape==NULL){
// 		while(grad_tape!=NULL){
// 			temp_tape = grad_tape;
// 			grad_tape =  grad_tape->next;
// 			ASC_FREE(temp_tape);
// 		}
		TapeFree(grad_tape);
	}
	
	return 0;
}

/**
	This routine calculates the bar.dot fields. This is a tangent-of-adjoint mode.
	@param trace is the tape on which the trace has been logged.
	@return Not Significant
*/
static int ReturnSweep2ndDeriv(Element *trace){
	Element* tracer = trace;
	double deriv1;
	double deriv2;
	double deriv1_2nd;
	double deriv1_2_2nd;
	double deriv2_2nd;
#define y tracer
#define yv (y->val.val)
#define yd (y->val.dot)
#define yb (y->bar.val)
#define ybd (y->bar.dot)
#define a1 (y->arg1)
#define a2 (y->arg2)
#define a1v (a1->val.val)
#define a2v (a2->val.val)
#define ia2v ((int)(a2v))
#define a1d (a1->val.dot)
#define a2d (a2->val.dot)
#define a1b (a1->bar.val)
#define a2b (a2->bar.val)
#define a1bd (a1->bar.dot)
#define a2bd (a2->bar.dot)
#define fxn (y->fxnptr)

	while(tracer!=NULL){
		switch(tracer->expr_type){
			case e_plus:
				a1bd += ybd; 
				a2bd += ybd;
				break;
			case e_minus:
				a1bd += ybd;
				a2bd -= ybd;
				break;
			case e_times:
				a1bd += (ybd) * (a2v) + (yb) * (a2d);
				a2bd += (ybd) * (a1v) + (yb) * (a1d);
				break;
			case e_divide:
				deriv1 = (1.0/a2v);
				deriv2 = (-a1v/asc_ipow(a2v,2));
				deriv1_2nd = 0.0;
				deriv2_2nd = (2 * a1v)/(asc_ipow(a2v,3));
				deriv1_2_2nd = (-1.0/asc_ipow(a2v,2));
				a1bd += (ybd) * (deriv1) + (yb) * (deriv1_2_2nd * (a2d));
				a2bd += (ybd) * (deriv2) + (yb) * (deriv1_2_2nd * (a1d) + deriv2_2nd * (a2d));
				break;
			case e_uminus:
				a1bd -= (ybd);
				break;
			case e_power:
				deriv1 = (a2v) * pow(a1v,a2v-1);
				deriv2 = pow(a1v,a2v) * log(a1v);
				deriv1_2nd = (a2v) * (a2v-1) * pow(a1v,a2v-2); 
				deriv2_2nd = pow(a1v,a2v) * asc_ipow(log(a1v),2);
				deriv1_2_2nd = pow(a1v,a2v-1) + (a2v) * pow(a1v,a2v-1) * log(a1v);
				a1bd += (ybd) * (deriv1) + (yb) * (deriv1_2nd * (a1d) + deriv1_2_2nd * (a2d));
				a2bd += (ybd) * (deriv2) + (yb) * (deriv1_2_2nd * (a1d) + deriv2_2nd * (a2d));
				break;
			case e_ipower:
				// x^n
				deriv1 = ia2v * asc_ipow(a1v,a2v-1);
				deriv2 = asc_ipow(a1v,a2v) * log(a1v);
				deriv1_2nd = (a2v) * (a2v-1) * asc_ipow(a1v,a2v-2); 
				a1bd += (yb) * deriv1_2nd * (a1d) + (ybd) * deriv1 ;	//a2 is e_int hence a2d = 0		
				a2bd += (ybd) * deriv2 ;
				break;
			case e_func:
				deriv1 = FuncDeriv(fxn,a1v);
				deriv1_2nd = FuncDeriv2(fxn,a1v); 
				a1bd += (yb) * deriv1_2nd * (a1d) + (ybd) * deriv1 ;
				break;
			case e_equal:
				a1bd += ybd;
				if(a2!=NULL)   // relation length is non zero for both lhs and rhs
					a2bd -= ybd;
				break;
			default:
				break;
		}
		tracer=tracer->next;
	}
	return 0;
#undef y 
#undef yv
#undef yd
#undef yb
#undef ybd
#undef a1 
#undef a2 
#undef a1v
#undef a2v
#undef ia2v
#undef a1d 
#undef a2d 
#undef a1b 
#undef a2b 
#undef a1bd
#undef a2bd
#undef fxn 
}


/**
	This routine calculates the bar.dot fields. This is a tangent-of-adjoint mode.
	@param trace is the tape on which the trace has been logged.
	@return Not Significant
	@note Safe Version
*/
static int ReturnSweep2ndDerivSafe(Element *trace,enum safe_err *serr){
	Element* tracer = trace;
	double deriv1;
	double deriv2;
	double deriv1_2nd;
	double deriv2_2nd;
	double deriv1_2_2nd;
#define y tracer
#define yv (y->val.val)
#define yd (y->val.dot)
#define yb (y->bar.val)
#define ybd (y->bar.dot)
#define a1 (y->arg1)
#define a2 (y->arg2)
#define a1v (a1->val.val)
#define a2v (a2->val.val)
#define ia2v ((int)(a2v))
#define a1d (a1->val.dot)
#define a2d (a2->val.dot)
#define a1b (a1->bar.val)
#define a2b (a2->bar.val)
#define a1bd (a1->bar.dot)
#define a2bd (a2->bar.dot)
#define fxn (y->fxnptr)
	while(tracer!=NULL){
		switch(tracer->expr_type){
			case e_plus:
				a1bd = safe_add_D0(a1bd,ybd,serr);
				a2bd = safe_add_D0(a2bd,ybd,serr);
				break;
			case e_minus:
				a1bd = safe_add_D0(a1bd,ybd,serr);
				a2bd = safe_sub_D0(a2bd,ybd,serr);
				break;
			case e_times:
				a1bd = safe_add_D0(safe_add_D0(safe_mul_D0(ybd,a2v,serr),safe_mul_D0(yb,a2d,serr),serr),a1bd,serr);
				a2bd = safe_add_D0(safe_add_D0(safe_mul_D0(ybd,a1v,serr),safe_mul_D0(yb,a1d,serr),serr),a2bd,serr);
				break;
			case e_divide:
				deriv1 = safe_rec(a2v,serr);
				deriv2 = safe_mul_D0(-(safe_rec(a2v,serr)),safe_div_D0(a1v,a2v,serr),serr);
				deriv2_2nd = safe_div_D0(safe_mul_D0(2,a1v,serr),safe_ipow_D0(a2v,3,serr),serr);
				deriv1_2_2nd = safe_rec(-safe_ipow_D0(a2v,2,serr),serr);
				a1bd = safe_add_D0(safe_add_D0(safe_mul_D0((ybd),(deriv1),serr),safe_mul_D0((yb),safe_mul_D0(deriv1_2_2nd,(a2d),serr),serr),serr),a1bd,serr);
				a2bd += (ybd) * (deriv2) + (yb) * (deriv1_2_2nd * (a1d) + deriv2_2nd * (a2d));
				break;
			case e_uminus:
				a1bd = safe_sub_D0(a1bd,ybd,serr);
				break;
			case e_power:
				deriv1 = safe_mul_D0(a2v,safe_pow_D0(a1v,a2v-1,serr),serr);
				deriv2 = safe_mul_D0(safe_pow_D0(a1v,a2v,serr),safe_ln_D0(a1v,serr),serr);
				deriv1_2nd = safe_mul_D0(safe_mul_D0(a2v,a2v-1,serr),safe_pow_D0(a1v,a2v-2,serr),serr); 
				deriv2_2nd = safe_mul_D0(safe_pow_D0(a1v,a2v,serr),safe_pow_D0(safe_ln_D0(a1v,serr),2,serr),serr);
				a1bd = safe_add_D0(a1bd,safe_add_D0(safe_mul_D0(safe_mul_D0(yb,deriv1_2nd,serr),a1d,serr),safe_mul_D0(ybd,deriv1,serr),serr),serr);
				a2bd = safe_add_D0(a2bd,safe_add_D0(safe_mul_D0(safe_mul_D0(yb,deriv2_2nd,serr),a2d,serr),safe_mul_D0(ybd,deriv2,serr),serr),serr);
				break;
			case e_ipower:
				deriv1 = safe_mul_D0(ia2v,safe_ipow_D0(a1v,a2v-1,serr),serr);
				deriv2 = safe_mul_D0(safe_ipow_D0(a1v,a2v,serr),safe_ln_D0(a1v,serr),serr);
				deriv1_2nd = safe_mul_D0(safe_mul_D0(ia2v,ia2v-1,serr),safe_ipow_D0(a1v,a2v-2,serr),serr); 
				deriv2_2nd = safe_mul_D0(safe_ipow_D0(a1v,a2v,serr),safe_pow_D0(safe_ln_D0(a1v,serr),2,serr),serr);
				a1bd = safe_add_D0(a1bd,safe_add_D0(safe_mul_D0(safe_mul_D0(yb,deriv1_2nd,serr),a1d,serr),safe_mul_D0(ybd,deriv1,serr),serr),serr);
				a2bd = safe_add_D0(a2bd,safe_add_D0(safe_mul_D0(safe_mul_D0(yb,deriv2_2nd,serr),a2d,serr),safe_mul_D0(ybd,deriv2,serr),serr),serr);
				break;
			case e_func:
				deriv1 = FuncDerivSafe(fxn,a1v,serr);
				deriv1_2nd = FuncDeriv2Safe(fxn,a1v,serr); 
				a1bd = safe_add_D0(a1bd,safe_add_D0(safe_mul_D0(safe_mul_D0(yb,deriv1_2nd,serr),a1d,serr),safe_mul_D0(ybd,deriv1,serr),serr),serr);
				break;
			case e_equal:
				a1bd = safe_add_D0(a1bd,ybd,serr);
				if(a2!=NULL)   // relation length is non zero for both lhs and rhs
					a2bd = safe_sub_D0(a2bd,ybd,serr);
				break;
			default:
				break;
		}
		tracer=tracer->next;
	}
	return 0;
#undef y 
#undef yv
#undef yd
#undef yb
#undef ybd
#undef a1 
#undef a2 
#undef a1v
#undef a2v
#undef ia2v
#undef a1d 
#undef a2d 
#undef a1b 
#undef a2b 
#undef a1bd
#undef a2bd
#undef fxn 
}

/**------------------------------------------------------------------------------- */
/**
	Rewinds the tape
	@param trace is the tape on which trace information has been recorded
*/
static Element* TapeRewind(Element* trace){
	Element* head = trace;
	asc_assert(head!=NULL);
	while(head->next!=NULL){
		head = head->next;
	}
	return head;
}

/**
	Rewinds the tape & prepares it for a forward sweep by clearing values for
	val.dot and bar.dot fields corresponding to the variable w.r.t which
	the row of the hessian is being evaluated.
	@param trace is the tape on which trace information has been recorded
	@param var_index is the index of the variable wrt which the row is calculated
*/
static Element* TapeRewindInitFwd(Element* trace,unsigned long var_index){
	Element* head = trace;
	if(head == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Pointer to Tape is NULL");
	}
	
	while(1){
		head->bar.dot = 0.0;

		if(head->expr_type == e_var && head->sindex==var_index){
			head->val.dot = 1.0;
		}
		else{
			head->val.dot = 0.0;
		}

		if(head->next!=NULL){
			head = head->next;
		}
		else{
			return head;
		}

	}

	return NULL;
}


/**
	Accumulates the Second Partial Value for each row of the Hessian.
	@param trace is the tape on which trace information is logged
	@param deriv2nd is the array on which the second partials are accumulated
	@param hessian_calc indicates if it is a Full hessian or LT or UT Matrix Hessian
						1 indicates LT or UT
						0 indicates Full Hessian (i.e.) full row
	@return Not significant
*/
static int AccumulateDeriv2nd(Element *trace,double *deriv2nd,unsigned long num_var,int hessian_calc)
{
	Element *head = trace;
	while(head!=NULL){
		if(head->expr_type == e_var){
			if(hessian_calc && head->sindex<num_var){
				deriv2nd[head->sindex] += head->bar.dot;
			}
			else if (!hessian_calc){
				deriv2nd[head->sindex] += head->bar.dot;
			}
		}
		head = head->next;
	}
	return 0;
}


/**
	Accumulates the Second Partial Value for each row of the Hessian.
	@param trace is the tape on which trace information is logged
	@param deriv2nd is the array on which the second partials are accumulated
	@param hessian_calc indicates if it is a Full hessian or LT or UT Matrix Hessian
						1 indicates LT or UT
						0 indicates Full Hessian (i.e.) full row
	@return Not significant
	@note Safe Version
*/
static int AccumulateDeriv2ndSafe(Element *trace,double *deriv2nd,unsigned long num_var,int hessian_calc,enum safe_err *serr)
{
	Element *head = trace;
	while(head!=NULL){
		if(head->expr_type == e_var){
			if (hessian_calc && head->sindex<num_var){
				deriv2nd[head->sindex] = safe_add_D0(deriv2nd[head->sindex],head->bar.dot,serr);
			}
			else if (!hessian_calc){
				deriv2nd[head->sindex] = safe_add_D0(deriv2nd[head->sindex],head->bar.dot,serr);
			}
		}
		head = head->next;
	}
	return 0;
}

/**---------------------Hessian Calculation Routines--------------------------*/

/**
	TODO 
	Change the second derivative routines to calculate all the second
	derivatives, and pick the values from the array suitably

	Not much of performance degradation here. 
	But gives uniform method signature

*/


int RelationEvaluateHessianMtx(CONST struct relation *r,
							   	hessian_mtx *hess_mtx,
		  						unsigned long dimension)
{
	Element* hess_tape;
	Element* temp_tape;
	double residual;
	double *row_pointer=NULL;
	unsigned long i;
	
	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateHessianMtx");

	if(r==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Relation instance is NULL");
	}
	
	hess_tape = RelationEvaluateResidualGradientRev(r,&residual,NULL,1);
	
	for(i=0;i<dimension;i++){
		/** TODO When the Matrix is full or Upper triagnfular,
			Need to pass in information about the length and 
			for which indices accumulation should occur in 
			AccumulateDeriv2nd	
		*/
		row_pointer = Hessian_Mtx_get_row_pointer(hess_mtx,i);
		RelationEvaluateSecondDeriv(r,row_pointer,i,1,hess_tape); //FIXME works currently only for LT. Refer TODO above
	}
	
// 	while(hess_tape!=NULL){
// 		temp_tape = hess_tape;
// 		hess_tape =  hess_tape->next;
// 		ASC_FREE(temp_tape);
// 	}

	TapeFree(hess_tape);
	
	return 0;
}

int RelationEvaluateHessianMtxSafe(CONST struct relation *r,
								   	hessian_mtx *hess_mtx,
		   							unsigned long dimension,
	 								enum safe_err *serr)
{
	Element* hess_tape;
	Element* temp_tape;
	double residual;
	double *row_pointer;
	unsigned long i;
	
	//CONSOLE_DEBUG("IN FUNCTION RelationEvaluateHessianMtxSafe");	

	if(r==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Relation instance is NULL");
	}
	
	hess_tape = RelationEvaluateResidualGradientRevSafe(r,&residual,NULL,1,serr);
	
	

	for(i=0;i<dimension;i++){
		/** TODO When the Matrix is full or Upper triagnfular,
			Need to pass in information about the length and 
			for which indices accumulation should occur in 
			AccumulateDeriv2nd	
		*/
		row_pointer = Hessian_Mtx_get_row_pointer(hess_mtx,i);
		RelationEvaluateSecondDerivSafe(r,row_pointer,i,1,hess_tape,serr); //FIXME works currently only for LT. Refer TODO above
	}
	
// 	while(hess_tape!=NULL){
// 		temp_tape = hess_tape;
// 		hess_tape =  hess_tape->next;
// 		ASC_FREE(temp_tape);
// 	}

	TapeFree(hess_tape);
	
	return 0;
}

int TapeFree(Element* head){
	Element* temp_tape;
	while(head!=NULL){
		temp_tape = head;
		head =  head->next;
		temp_tape->val.val=0.0;
		temp_tape->val.dot=0.0;
		temp_tape->bar.val=0.0;
		temp_tape->bar.dot=0.0;
		ASC_FREE(temp_tape);
		temp_tape=NULL;
	}
	return 0;
}

/**---------------------------------------------------------------------------*/
/* extra stuff from relation_util.c, just copied temporarily? -- JP */
#ifndef NDEBUG
/**
	Utility function to perform debug checking of (input) instance and residual
	(or gradient) (output) pointers in the various functions in this file.

	@return 1 on all-ok
*/
static int check_inst_and_res(struct Instance *i, double *res){
# ifdef RELUTIL_CHECK_ABORT
	if(i==NULL){
		ASC_PANIC("NULL instance");
	}else if (res==NULL){
		ASC_PANIC("NULL residual pointer");
	}else if(InstanceKind(i)!=REL_INST){
		ASC_PANIC("Not a relation");
	}
# else
  if( i == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL instance");
    return 0;
  }else if (res == NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL residual ptr");
    return 0;
  }else if( InstanceKind(i) != REL_INST ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"not relation");
    return 0;
  }
# endif
  return 1;
	return 1;
}

#endif

/*	ASCEND modelling environment
	Copyright (C) 2017 John Pye
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 1994, 1995 Kirk Andre' Abbott
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file Error-reporting functions for the compiler
The `rel_errorlist` type will allow us to feed back more information on the
source of errors found in the compiler. Currently it is difficult/impossible
to pass back the name of the variable which has causes a problem. This new
structure should facilitate that -- JP 4 Apr 2017.
*/
#ifndef ASC_RELERR_H
# define ASC_RELERR_H

enum find_errors {
  unmade_instance,      /**< Found an unmade instance (NULL child). */
  undefined_instance,   /**< Instance in an expression is unknown child. */
  impossible_instance,  /**< Name cannot possibily exist(real error),often sets. */
  correct_instance      /**< Return value when everything went okay. */
};

enum relation_errorsx {
	find_error,                   /**< indicates an error finding an instance */
	incorrect_structure,
	incorrect_inst_type,          /**< contains a nonscalar instance type */
	incorrect_real_inst_type,     /**< contains a real_inst type */
	incorrect_boolean_inst_type,  /**< contains a boolean instance type */
	incorrect_integer_inst_type,  /**< contains an integer variable instance type*/
	incorrect_symbol_inst_type,   /**< contains a symbol instance type */
	integer_value_undefined,      /**< integer constant doesn't have a value yet */
	real_value_undefined,         /**< real constant doesn't have a value yet */
	real_value_wild,              /**< real constant doesn't have a dim yet */
	incorrect_num_args,           /**< wrong number of arguements */
	okay
};

/** Logical relation errors. */
enum logrelation_errorsx {
  find_logerror,                /**< error finding an instance */
  incorrect_logstructure,
  incorrect_linst_type,         /**< contains a nonboolean instance type */
  incorrect_real_linst_type,    /**< contains a real_inst type */
  incorrect_boolean_linst_type, /**< contains a boolean instance type */
  incorrect_integer_linst_type, /**< contains an integer variable instance type*/
  incorrect_symbol_linst_type,  /**< contains a symbol instance type */
  boolean_value_undefined,      /**< boolean constant doesn't have a value yet */
  incorrect_num_largs,          /**< wrong number of arguments */
  lokay                         /**< no error */
};

struct Instance;
struct Statement;

struct rel_errorlist_struct;

/* details of the struct need to be here if we want to be able to allocate
this object on the stack. the design principle will be that everything is
on the stack unless an error occurs; in that case there will be stuff on the
heap requiring some attention for cleanup. */
struct rel_errorlist_struct{
	enum relation_errorsx errcode;
	enum logrelation_errorsx lrcode;
	enum find_errors ferr;
	unsigned long ferrpos;
/*	// removed items -- more sophisticated error traces
	struct gl_list *errs;
	enum relation_errorsx lastcode;
	enum find_errors ferr;
	unsigned long ferrpos;
*/
};
typedef struct rel_errorlist_struct rel_errorlist;

/* we will try using the 'cleanup' attribute (GCC, clang) for automating the
freeing of memory for rel_errorlist. In cases where compilers don't support
something like this, we can simply not provide extended error reporting, 
which was the previous behaviour. however, using this mechanism, we can now
store extended error information which can be used to improve WSEM 
(WriteStatementErrorMessage) considerably, we hope, without any risk of
memory leaks. */
#define REL_ERRORLIST rel_errorlist __attribute__((cleanup(rel_errorlist_destroy_contents)))

#define REL_ERRORLIST_EMPTY {.errcode=okay, .lrcode=lokay, .ferr=correct_instance, .ferrpos=0}

//rel_errorlist *rel_errorlist_new();
void rel_errorlist_destroy_contents(rel_errorlist *err);
void rel_errorlist_destroy(rel_errorlist *err);

//int rel_errorlist_add(rel_errorlist *err,enum relation_errorsx errcode, struct Instance *errinst, struct Statement *errstmt);
//int rel_errorlist_set_ok(rel_errorlist *err);

int rel_errorlist_set_find_error(rel_errorlist *err, enum find_errors ferr);

int rel_errorlist_get_find_error(rel_errorlist *err);
int rel_errorlist_set_find_errpos(rel_errorlist *err,unsigned long errpos);
//int rel_errorlist_get_lastcode(rel_errorlist *err);

int rel_errorlist_set_lrcode(rel_errorlist *err, enum logrelation_errorsx lrcode);
int rel_errorlist_get_lrcode(rel_errorlist *err);

int rel_errorlist_set_code(rel_errorlist *err, enum relation_errorsx errcode);
int rel_errorlist_get_code(rel_errorlist *err);

#endif /* ASC_RELERR_H */


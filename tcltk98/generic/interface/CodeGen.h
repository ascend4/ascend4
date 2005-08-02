/*
 *  CodeGen.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: CodeGen.h,v $
 *  Date last modified: $Date: 1997/07/18 12:22:30 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Code Generation Routines
 *  <pre>
 *  Requires:     #include "tcl.h"
 *                #include "utilities/ascConfig.h"
 *                #include "solver/slv_client.h"
 *                #include "solver/var.h"
 *                #include "compiler/exprs.h"
 *                #include "solver/rel.h"
 *                #include "compiler/interface.h"
 *                #include "compiler/instance_enum.h"
 *  </pre>
 *  @todo Complete documentaion of CodeGen.h.
 */

#ifndef CodeGen_module_loaded
#define CodeGen_module_loaded

#define CG_INCLUDED   0x1
#define CG_LESS       0x2
#define CG_GREATER    0x4
#define CG_EQUAL	CG_LESS | CG_GREATER

#define CG_FIXED      0x1
#define CG_INPUT      0x2
#define CG_OUTPUT     0x4
#define CG_SLV_CONST  0x8
#define CG_SLV_REAL   0x10

#define CG_SLV_OPEN   0x0

enum CodeGen_enum {
  CG_ascend, CG_linear, CG_gams, CG_math, CG_c, /* main classes */
  CG_minos, CG_blackbox, CG_glassbox,           /* subclasses of CG_c */
  CG_squarebracket, CG_curlybracket, CG_round,  /* array subsrcripts*/
  CG_hat_power, CG_dstar_power, CG_func_power,  /* exponentiation */
  CG_csr, CG_ll, CG_jds                         /* matrix formats */
};

struct CGFormat {
  enum CodeGen_enum main_format;
  enum CodeGen_enum parens;
  enum CodeGen_enum power;
  enum CodeGen_enum funcs;
  enum CodeGen_enum names;
};

struct CGVar {
  struct Instance *instance;
  VOIDPTR prev_instanceinfo;
  int index;
  int cmplr_index;
  unsigned int flags;
};

struct CGData {
  struct gl_list_t *input_list;
  struct gl_list_t  *output_list;
  enum CodeGen_enum matrix_type;
  struct {
    int num_vars;
    struct CGVar *var_list;         /* an array of structs */
  } vars;
  struct {
    int num_pars;
    struct var_variable **par_list; /* null terminated array of ptrs */
  } pars;
  struct {
    int num_rels;
    struct rel_relation **rel_list; /* null terminated array of ptrs */
  } rels;
  struct {
    int num_objs;
    struct rel_relation **obj_list; /* null terminated array of ptrs */
  } objs;
  struct {
    int num_vars;
    int num_pars;
    int num_rels;
    int num_objs;
    int num_incidences;
  } filtered;
};

extern struct CGData g_cgdata;	/**< The main working data structure */

/*
 * Some data access routines, which external clients should
 * use.
 */
extern struct Instance *Asc_CGVarInstance(struct CGVar *cgvar);
extern struct CGVar *Asc_CGInstanceVar(struct Instance *instance);
extern struct var_variable *Asc_CGInstancePar(struct Instance *instance);
extern int Asc_CGVarFixed(struct CGVar *);
extern int Asc_CGRelIncluded(struct rel_relation *);

extern struct CGVar *Asc_CodeGenSetUpVariables(struct var_variable **vp,
                                               int num_vars);
extern struct CGVar *Asc_CodeGenSetUpVariables3(struct gl_list_t *list);

extern struct rel_relation **
Asc_CodeGenSetUpRelations(struct rel_relation **rp, int num_rels);

extern struct rel_relation **
Asc_CodeGenSetUpObjectives(struct rel_relation **op, int num_objs);

extern int Asc_CodeGenSetupCodeGen(slv_system_t sys,
                                   struct CGVar *cgvarlist, int nvars,
                                   struct rel_relation **rp, int nrels,
                                   struct rel_relation **op, int nobjs,
                                   struct var_variable **pp, int npars,
                                   struct gl_list_t *inputs,
                                   struct gl_list_t *outputs);
/**<
 *  This function sets up the main working data structure g_cgdata.
 *  It is being exported so as to make accessible across all the CodeGen
 *  files.
 */

extern void Asc_CodeGenShutDown(void);
/**<
 *  This function shuts down the main working data structure g_cgdata.
 *  It is being exported so as to make accessible across all the CodeGen
 *  files.
 */

extern int Asc_CodeGenParseDataCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, char *argv[]);
/**<
 *  This function simply attempts to find all the instance 
 *  corresponding to the names in the list. This is mainly
 *  used for debugging.  Where list is a proper tcl list. <br><br>
 *
 *  registered as __codegen_parsedata list.
 */

extern int Asc_CodeGenCCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, char *argv[]);
/**<
 *  Works on g_solvsys_cur, though this may change. The inputlist,
 *  outputlist and parameterlist may be *empty*, but *must* be provided.
 *  These lists are lists of variable that need to be marked specially
 *  for the codegeneration routines.
 *  grad?nograd tells whether gradients should be generated or not.
 *  filename is a prefix that will be used for all generated files, and
 *  exported functions so as to protect the namespace.
 *  Eg. __codegen_c flash grad {T,P,Feed} {Liq,Vap,phi} {tanksize}<br><br>
 *
 *  registered as:
 *  __codegen_c filename ?grad?nograd? inputlist outputlist parameterlist
 */

/*
 * This functionality is implemented in file CodeGen2.c
 */
extern int Asc_CodeGenWriteCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, char *argv[]);
/**<
 *  @todo ken needs to put some header here...
 */

extern int Asc_CodeGenReadCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, char *argv[]);
/**<
 *  @todo ken needs to put some header here...
 */

extern int Asc_CodeGenGamsCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, char *argv[]);
/**<
 *  @todo ken needs to put some header here...
 */

extern int Asc_CodeGenGeneralCmd(ClientData cdata, Tcl_Interp *interp,
                                 int argc, char *argv[]);
/**<
 *  Generates code in one of the major formats supported i.e.,
 *  ascend (linearized), gams, mathematica.
 *  Works on g_solvsys_cur.
 *  grad?nograd tells whether gradients should be generated or not.
 *  filename is a prefix that will be used for all generated files.<br><br>
 *
 *  Registered as __codegen_general filename ?grad?nograd? format.
 */

extern void Asc_CodeGenWriteAscendFile(slv_system_t sys,
                                       FILE *fp, 
                                       char *file_prefix,
                                       int gradients,
                                       char *typelist);
/**<
 *  Start of some codegeneration routine to support the glass-box mode
 *  of pulling in external relations. typelist is a list of types that
 *  may be used to filter the amount of information that is written out.
 */

/** some experimental stuff */
extern int  Asc_CodeGenTypesCmd(ClientData cdata, Tcl_Interp *interp,
                                int argc, char *argv[]);

#endif /* CodeGen_module_loaded */


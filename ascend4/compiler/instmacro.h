/*
 *  Ascend Instance Tree Type Implementation Macros
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: instmacro.h,v $
 *  Date last modified: $Date: 1997/07/18 12:30:46 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright 1996 Bejamin Allan
 *  based on instance.c
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#ifndef __INSTMACRO_H_SEEN__
#define __INSTMACRO_H_SEEN__

/*
 *---------------------------------------------------------------------------
 *      This header for instance and instantiator consumption only.
 *                      Clients keep your hands off!
 *---------------------------------------------------------------------------
 */

/* temporary global variables used for debugging only */
#ifndef NDEBUG
/* fundies */
extern struct RealInstance *g_r_inst;
extern struct IntegerInstance *g_i_inst;
extern struct BooleanInstance *g_b_inst;
extern struct SetInstance *g_s_inst;
extern struct SymbolInstance *g_sym_inst;
/* constants */
extern struct RealConstantInstance *g_rc_inst;
extern struct IntegerConstantInstance *g_ic_inst;
extern struct BooleanConstantInstance *g_bc_inst;
extern struct SetConstantInstance *g_sc_inst;
extern struct SymbolConstantInstance *g_symc_inst;
/* atoms */
extern struct RealAtomInstance *g_ra_inst;
extern struct IntegerAtomInstance *g_ia_inst;
extern struct BooleanAtomInstance *g_ba_inst;
extern struct SetAtomInstance *g_sa_inst;
extern struct SymbolAtomInstance *g_syma_inst;
/* other */
extern struct ModelInstance *g_mod_inst;
extern struct RelationInstance *g_rel_inst;
extern struct LogRelInstance *g_lrel_inst;
extern struct WhenInstance *g_when_inst;
#endif

/* init parent list size */
#define AVG_CONSTANT_PARENTS 2L	       /* size to which all parent lists are */
           /* initialized for real constants */
#define AVG_ICONSTANT_PARENTS 20L      /* size to which all parent lists are */
           /* initialized for int constants */
#define AVG_PARENTS 2L		/* size to which all parent lists are */
    /* initialized */
#define AVG_CASES 2L		/* size to which all cases lists are */
    /* initialized (WHEN instance) */
#define AVG_WHEN 2L		/* size to which all when lists are */
    /* initialized (models, relations) */
#define AVG_RELATIONS 7L	/* size to which all relation lists are */
    /* initialized */
#define AVG_LOGRELS 7L  	/* size to which all logical relation   */
                                /* lists are initialized */
#define AVG_ARY_CHILDREN 7L	/* size to which all array children lists */
    /* are initialized */
#define UNDEFAULTEDREAL 0.0

#define MAX_EXTRELATIONS 20     /* maximum number of different ext relations */
                                /* for a given simulation */

/* type coercion definitions */
/* any */
#define INST(i) ((struct Instance *)(i))
/* other */
#define SIM_INST(i) ((struct SimulationInstance *)(i))
#define RELN_INST(i) ((struct RelationInstance *)(i))
#define LRELN_INST(i) ((struct LogRelInstance *)(i))
#define ARY_INST(i) ((struct ArrayInstance *)(i))
#define MOD_INST(i) ((struct ModelInstance *)(i))
#define W_INST(i) ((struct WhenInstance *)(i))
/* fundies */
#define R_INST(i) ((struct RealInstance *)(i))
#define I_INST(i) ((struct IntegerInstance *)(i))
#define B_INST(i) ((struct BooleanInstance *)(i))
#define S_INST(i) ((struct SetInstance *)(i))
#define SYM_INST(i) ((struct SymbolInstance *)(i))
/* constants */
#define CI_INST(i) ((struct CommonConstantInstance *)(i))
#define RC_INST(i) ((struct RealConstantInstance *)(i))
#define IC_INST(i) ((struct IntegerConstantInstance *)(i))
#define BC_INST(i) ((struct BooleanConstantInstance *)(i))
#define SYMC_INST(i) ((struct SymbolConstantInstance *)(i))
/* atoms */
#define CA_INST(i) ((struct CommonAtomInstance *)(i))
#define RA_INST(i) ((struct RealAtomInstance *)(i))
#define BA_INST(i) ((struct BooleanAtomInstance *)(i))
#define IA_INST(i) ((struct IntegerAtomInstance *)(i))
#define SA_INST(i) ((struct SetAtomInstance *)(i))
#define SYMA_INST(i) ((struct SymbolAtomInstance *)(i))
#define SOL_INST(i) ((struct SolverAtomInstance *)(i))

#define D_INST(i) ((struct GlobalDummyInstance *)(i))
/* constant inst assigned? */
#define CIASS(i) (CI_INST(i)->vflag & ci_ASSIGNED)
/* boolean constant value */
#define BCV(i) ((BC_INST(i)->vflag & ci_BVAL) == ci_BVAL)

/* parent macros for fundamental atoms real, integer, boolean, and set */
#define R_PARENT(i) \
  INST((unsigned long)i-(unsigned long)R_INST(i)->parent_offset)
#define I_PARENT(i) \
  INST((unsigned long)i-(unsigned long)I_INST(i)->parent_offset)
#define B_PARENT(i) \
  INST((unsigned long)i-(unsigned long)B_INST(i)->parent_offset)
#define S_PARENT(i) \
  INST((unsigned long)i-(unsigned long)S_INST(i)->parent_offset)
#define SYM_PARENT(i) \
  INST((unsigned long)i-(unsigned long)SYM_INST(i)->parent_offset)

/* this should probably be conditionally defined by LONGCHILDREN
 * so that if LONGCHILDREN, just returns ivalue.
 */
#define NextHighestEven(ivalue) (((ivalue) & 1) ? ((ivalue)+1) : (ivalue))

/* child array macros */
#define CHILD_ADR(iptr,type,c)\
  ((struct Instance **)((unsigned long)iptr+(unsigned long)sizeof(type))+c)
#define SIM_CHILD(i,c) CHILD_ADR(i,struct SimulationInstance,c)
#define MOD_CHILD(i,c) CHILD_ADR(i,struct ModelInstance,c)
#define RA_CHILD(i,c) CHILD_ADR(i,struct RealAtomInstance,c)
#define BA_CHILD(i,c) CHILD_ADR(i,struct BooleanAtomInstance,c)
#define IA_CHILD(i,c) CHILD_ADR(i,struct IntegerAtomInstance,c)
#define SA_CHILD(i,c) CHILD_ADR(i,struct SetAtomInstance,c)
#define SYMA_CHILD(i,c) CHILD_ADR(i,struct SymbolAtomInstance,c)
#define REL_CHILD(i,c) CHILD_ADR(i,struct RelationInstance,c)
#define LREL_CHILD(i,c) CHILD_ADR(i,struct LogRelInstance,c)

#define CLIST(in,type) (struct Instance **)((unsigned long)(in)+sizeof(type))
#define BASE_ADDR(in,num,type) INST((CLIST(in,type)) + NextHighestEven(num))

/* the whole use of BASE_ADDR needs to be reinvestigated. in particular
 * we need RealInstance children to be aligned to 8 bytes, not 2 bytes
 * which is all NextHighestEven gives us.
 */

#ifdef NDEBUG
#define NotAtom(i) IsCompoundInstance(i)
#else
#define NotAtom(i) NotAtomF(i)
#endif
extern int NotAtomF(struct Instance *);
/*
 *  macro NotAtom(i)
 *  NotAtomF(i)
 *  Returns 1 if instance is compound (i.e. not relation, ATOM, constant,
 *  when. that means array or MODEL.) else returns 0.
 */

#endif
/* __INSTMACRO_H_SEEN__ */

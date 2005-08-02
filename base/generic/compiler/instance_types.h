/*
 *  Real Ascend Instance Types
 *  by Tom Epperly
 *  Version: $Revision: 1.29 $
 *  Version control file: $RCSfile: instance_types.h,v $
 *  Date last modified: $Date: 1998/02/05 16:36:22 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1996 Benjamin Andrew Allan
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

#ifndef __INSTANCE_TYPES_H_SEEN__
#define __INSTANCE_TYPES_H_SEEN__

/** @file
 *  Real Ascend Instance Types.
 *  <pre>
 *  When #including instance_types.h, make sure these files are #included first:
 *         #include "instance_enum.h"
 *
 *  Fields not to be accessed by any client program
 *  except by the read-only functions in headers.
 *  The write functions in atomvalue.h are also reasonable for
 *  clients to use.
 *  Clients should never have to include much beyond instance_enum.h.
 *
 *  All of the following structures should be a multiple of *8*;
 *  being a multiple of 4 is not sufficient. Pad if necessary.
 *  RealInstance
 *	IntegerInstance
 *	BooleanInstance
 *	SymbolInstance
 *	SetInstance
 *  and in fact any new instances that will be atom children.
 *  Remember that these get *packed* in together and the order of
 *  packing is not predictable in any way. If each is a multiple
 *  of 8, then everybody will be properly aligned.
 *  *AtomInstance and  *Rel*Instance also need to be 8 byte
 *  aligned so that we
 *  start children in an 8 byte.
 *
 *  This has to hold for both 4 and 8 byte ptr/long machines.
 *  </pre>
 */

#if SIZEOF_VOID_P == 8
#define LONGCHILDREN 1
#else
#define LONGCHILDREN 0
#endif
/**< if LONGCHILDREN, then pointers are 8 bytes */

/* at present, I don't believe the following define is needed */
/* # if defined(_HPUX_SOURCE) || defined(_SGI_SOURCE) */
/* # define ALIGNSTUPID 1 */
/* # endif */
/* if ALIGNSTUPID, then 4 byte ptrs must fall on 8 byte boundaries */
/* any architecture with such a restrict should be summarily torched */


/* FUNDAMENTAL INSTANCES */

/** this aligns to 8 bytes with 4 or 8 byte pointers. size 32/40 */
struct RealInstance {
  enum inst_t t;          /**< must always be first */
  unsigned assigned;      /**< the number of times it has been assigned */
  struct Instance *parent_offset;
  CONST dim_type *dimen;
  double value;
  unsigned depth;         /**< the depth of the assignment */
  int padding;            /**< required for either type of pointer */
};

/** this aligns to 8 bytes with 4 or 8 byte pointers. size 24/32 */
struct IntegerInstance {
  enum inst_t t;         /**< must always be first */
  unsigned assigned;     /**< the number of times it has been assigned */
  struct Instance *parent_offset;
  long value;
  unsigned depth;        /**< the depth of the last assignment */
  int padding;           /**< required for either type of pointer */
};

/** size 24/24 */
struct BooleanInstance {
  enum inst_t t;
  unsigned assigned;     /**< the number of times it has been assigned */
  struct Instance *parent_offset;
  unsigned value;        /**< 0 false, 1 true */
  unsigned depth;        /**< the depth of the last assignment */
#if (LONGCHILDREN == 0)
  int padding;
#endif
};

/** this aligns to 8 bytes with 4 or 8 byte pointers. size 16/24 */
struct SetInstance {
  enum inst_t t;
  unsigned int_set;		 /**< 0 indicates string set,
                            1 indicates an integer set */
  struct Instance *parent_offset;
  struct set_t *list;  /**< list of enumerated values, NULL
                            indicates that it has not been assigned
                            if HP_sUX 9/10 or SGI is stupid enough to misalign
                            this structure, we will have to pad it with an int
                            on both sides of *list */
};

/** this aligns to 8 bytes with 4 or 8 byte pointers. size 16/24 */
struct SymbolInstance {
  enum inst_t t;
  unsigned assigned;		/**< the number of times it has been assigned */
  struct Instance *parent_offset;
  symchar *value;	      /**< NULL indicates unassigned */
};


/* TRUE ATOM INSTANCES */
/** the first one is a dummy of some importance. All the atomic
 *  variable types should be conformable to this structure. It
 *  is not by itself a structure that should ever exist.
 */
struct CommonAtomInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parents */
  struct Instance *alike_ptr;   /**< circular linked list of clique members */
  struct TypeDescription *desc; /**< list of children pointers */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
};

/**
 * The RealAtomInstance should be reimplemented with a number of
 * supported attributes (bounds, nominal, several boolean flags)
 * and then an explicit pointer to the child space since children
 * are expected to be rare. Making this pointer explicit will allow
 * the real atoms (of which there are a lot) to be pooled and save
 * a fair amount of memory on large problems.<br><br>
 *
 * aligns to 8 byte boundaries on 4 or 8 byte machines. size 56/96
 */
struct RealAtomInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parents */
  struct Instance *alike_ptr;   /**< circular linked list of clique members */
  struct TypeDescription *desc; /**< list of children pointers */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonAtomInstance */
  /* atom value part */
  double value;                 /**< value of real variable */
  CONST dim_type *dimen;        /**< dimensions */
  struct gl_list_t *relations;  /**< relations where this real appears */
  unsigned int assigned;        /**< the number of times it has been assigned */
  unsigned int depth;           /**< the depth of the last assignment */
  /* An even number of child pointers are packed here, the last of which
   * may not be valid because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/** future work.
 * needs parser and interpretter support. Not yet used.
 * @todo Implement SolverAtomInstance.
 */
struct SolverAtomInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parents */
  struct Instance *alike_ptr;   /**< circular linked list of clique members */
  struct TypeDescription *desc; /**< list of slow children pointers, by name */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied */
  /* above should match with CommonAtomInstance */
  /* atom value part */
  struct gl_list_t *relations;  /**< relations where this real appears */
  double value;                 /**< value of variable
                                     fast children, all sharing dimens */
  double nominal;               /**< value of variable scaling */
  double lower;                 /**< value of variable lower bound */
  double upper;                 /**< value of variable upper bound */
  CONST dim_type *dimen;        /**< dimensions */
  struct Instance **carray;     /**< array of pointers to slow children.
                                     Keep slow children in pools by type.
                                     This should be NULL most of the time. */
  unsigned int flags;           /**< boolean flags of the variable, both
                                     internal and external. must be at least
                                     32 bit int */
  unsigned int depth;           /**< the depth of the last assignment */
  /*
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/*
 * Bit field definition for boolean attributes. room to grow.
 * Define all these fields so that a default value of (0,OFF,FALSE)
 * is the appropriate setting for 'typical' uses of ASCEND.
 */
/* internal flags (compiler use) for solveratominstances: */
#define SOL_ASSIGNED    0x1       /**< atom value ever assigned */
/* external flags (user interface/solver use) for solveratominstances: */
#define SOL_FIXED       0x2       /**< variable fixed for solvers */
#define SOL_INACTIVELB  0x4       /**< lower bound to be ignored in solving */
#define SOL_INACTIVEUB  0x8       /**< upper bound to be ignored in solving */
#define SOL_ODESTATE    0x10      /**< variable state for ivp solver */
#define SOL_ODEOBSERVE  0x20      /**< variable recording for ivp */
#define SOL_ISZERO      0x40      /**< semicontinuous set 0 for sciconic */
#define SOL_INTEGER     0x80      /**< integer variable */
#define SOL_RELAXEDINT  0x100     /**< integer variable treat as relaxed */
#define SOL_INTERVAL    0x200     /**< interval real variable */
#define SOL_COMPLEX     0x400     /**< complex variable. lower -> imaginary part
                                       upper -> magnitude bound. ASCEND -> pot */
#define SOL_DOMAIN      0x800     /**< continuous variable defines domain */
#define SOL_ODEINDEP    0x1000    /**< independent variable in ODE/BVP */
#define SOL_NODOF       0x2000    /**< user doesn't want to see var in DOF */
#define SOL_PARAMETRIC  0x4000    /**< whatever that means to a solver */
#define SOL_INTENSIVE   0x8000    /**< physics/che intensive property variable */
#define SOL_NONBASIC    0x10000   /**< in DOF of optimization problem
                                       2,4,8-0000 reserved for future */
/* external flags (solver/interface tool use) for solveratominstances: */
#define SOL_TIGHTLB     0x100000  /**< value at or near lower */
#define SOL_TIGHTUB     0x200000  /**< value at or near upper */
#define SOL_NOELIG      0x400000  /**< variable not eligible to be fixed in DOF */
/* note that the above bit positions need to be settled on still. */

/** aligns to 8 on 4 and 8 byte compilers with compiler pad after t, depth */
struct IntegerAtomInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parents */
  struct Instance *alike_ptr;
  struct TypeDescription *desc; /**< usefull type description information */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonAtomInstance */
  /* atom value part */
  unsigned int assigned;        /**< the number of times it has been assigned */
  struct gl_list_t *whens;      /**< link to whens on which it appears */
  unsigned int depth;           /**< the depth of the last assignment */
  long value;                   /**< integer value */
  /*
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/** this structure aligns (with compiler generated pad after t and flags
 * on 4 or 8 byte ptr machines. */
struct BooleanAtomInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parent instances */
  struct Instance *alike_ptr;
  struct TypeDescription *desc; /**< description of name,children, and size */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonAtomInstance */
  /* atom value part */
  struct gl_list_t *logrelations; /**< logrelations where this boolean appears */
  struct gl_list_t *whens;      /**< whens where this boolean appears*/
  unsigned assigned;            /**< the number of times it has been assigned */
  unsigned depth;               /**< the depth of the assignment */
  unsigned value;               /**< 0 false, 1 true */
  int padding;                  /**< so that children have a square address */
  /*
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/** this aligns to 8 on 4 or 8 byte compilers with compiler pad after t */
struct SetAtomInstance {
  enum inst_t t;                /**< type */
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parents instances */
  struct Instance *alike_ptr;
  struct TypeDescription *desc; /**< description of name,children, and size */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied */
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonAtomInstance */
  /* atom value part */
  unsigned int_set;             /**< 0 indicates string set,
                                     1 indicates integer set */
  struct set_t *list;           /**< list of enumerated values NULL indicates
                                     that is has not been assigned */
  /*
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/** this aligns to 8 on 4 or 8 byte compilers with compiler pad after t, flags */
struct SymbolAtomInstance {
  enum inst_t t;                /**< type */
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;    /**< link to parent instances */
  struct Instance *alike_ptr;   /**< ALIKE clique link */
  struct TypeDescription *desc; /**< description of name, children, and size */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance  is being copied */
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonAtomInstance  */
  struct gl_list_t *whens;      /**< link to whens on which it appears */
  /* atom value part */
  symchar *value;               /**< NULL indicates unassigned */
  /*
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */
  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/* CONSTANT INSTANCES */
/** The first one is a dummy of some importance. All the Constant
 * types should be conformable to this structure. It
 * is not by itself a structure that should ever exist.
 */
struct CommonConstantInstance {
  enum inst_t t;
  unsigned int vflag;
  unsigned long visited;    /**< visited counter */
  unsigned long tmp_num;    /**< used when an instance tree is being copied*/
  VOIDPTR interface_ptr;
  unsigned int anon_flags;  /**< anonymous field to be manipulated
                                 vflag is 32 bits with particular meanings: */
#define ci_ASSIGNED 0x1     /**< instance assigned yet? */
#define ci_BVAL 0x2         /**< boolean const instance value, reserved */
/* other binary flags constants need to be added here. */
};

/** @todo RealConstantInstance should probably be pooled, but isn't yet */
struct RealConstantInstance {
  enum inst_t t;
  unsigned vflag;               /**< assigned? */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  VOIDPTR interface_ptr;
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonConstantInstance */
  double value;
  struct gl_list_t *parents;    /**< link to parents */
  struct Instance *alike_ptr;   /**< circular linked list of clique members?*/
  struct TypeDescription *desc; /**< description of name, size */
  CONST dim_type *dimen;
};

struct IntegerConstantInstance {
  enum inst_t t;
  unsigned vflag;               /**< assigned? */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  VOIDPTR interface_ptr;
  unsigned int anon_flags;	    /**< anonymous field to be manipulated */
  /* above should match with CommonConstantInstance */
  long value;
  struct gl_list_t *parents;    /**< link to parents */
  struct gl_list_t *whens;      /**< whens where this integer appears*/
  struct Instance *alike_ptr;   /**< circular linked list of clique members?*/
  struct TypeDescription *desc; /**< description of name, size */
};

struct BooleanConstantInstance {
  enum inst_t t;
  unsigned vflag;               /**< assigned? */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  VOIDPTR interface_ptr;
  /* above should match with CommonConstantInstance */
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  struct gl_list_t *parents;    /**< link to parents */
  struct gl_list_t *whens;      /**< whens where this boolean appears*/
  struct Instance *alike_ptr;   /**< circular linked list of clique members?*/
  struct TypeDescription *desc; /**< description of name, size */
};

struct SymbolConstantInstance {
  enum inst_t t;
  unsigned vflag;               /**< assigned */
  unsigned long visited;        /**< visited counter */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  VOIDPTR interface_ptr;
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* above should match with CommonConstantInstance */
  struct gl_list_t *parents;    /**< link to parents */
  struct gl_list_t *whens;      /**< whens where this symbol appears*/
  struct Instance *alike_ptr;   /**< circular linked list of clique members?*/
  struct TypeDescription *desc; /**< description of name, size */
  symchar *value;
};

/** aligns on 4 or 8 byte ptr machines. size 48/88
 *
 * @todo RelationInstance should probably be pooled, as RealAtomInstance,
 *       but isn't yet for the same reasons.
 */
struct RelationInstance {
  enum inst_t t;
  /* on long pointer machines, we expect C compiler to pad 4 bytes here */
  VOIDPTR interface_ptr;
  struct Instance *parent[2];   /**< relations can have only two parents and
                                     normally they only have one.  They have
                                     two only during an ARE_THE_SAME */
  struct TypeDescription *desc; /**< holds the child list stuff */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  enum Expr_enum type;          /**< what kind of the 5 possible types of reln
                                     type should be down in union RelationUnion*/
  struct relation *ptr;         /**< pointer to an instance relation */
  struct gl_list_t *whens;      /**< link to whens on which the rel appears */
  struct gl_list_t *logrels;    /**< link to satified's on which rel appears */
  /* So child insts start packing on 8byte address after child ptrs, of
   * which there are (currently, 2/97) always an even number.
   */
  /* Not required anymore:
   *
   * #if (LONGCHILDREN == 0)
   * int padding;
   * #endif
   *
   * Not valid anymore:
   *
   * An even number of child pointers are packed here, the last of which
   * may not be real because the number of children may be odd.
   * This extra should be eliminated for LONG pointer machines.
   */

  /* After the child pointers comes the data space for the actual
   * children.
   */
};

/** this aligns to 8 bytes with 4 or 8 byte pointers. size 48/88 */
struct LogRelInstance {
  enum inst_t t;
  /* on long pointer machines, we expect C compiler to pad 4 bytes here */
  VOIDPTR interface_ptr;
  struct Instance *parent[2];   /**< log relations can have only two parents
                                     and normally they only have one.They have
                                     two only during an ARE_THE_SAME */
  struct TypeDescription *desc; /**< holds the child list stuff */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  struct logrelation *ptr;      /**< pointer to an instance logical relation */
  struct gl_list_t *whens;      /**< link to whens on which the logrel appears */
  struct gl_list_t *logrels;    /**< link to satified's on which lrel appears */
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  int padding;                  /**< so child insts start packing on 8byte
                                     address after child ptrs */
  /*
   * All logrelations are the same type/size, because we require
   * exactly 1 refinement of the base definition.
   */
  /* After the child pointers comes the data space for the actual
   * children. This space must start on an 8 byte address.
   */
};

/** Never, ever, allocate either one of these types. it's a dummy for the
 * instantiator.
 */
struct PendInstance {
  enum inst_t t;
  VOIDPTR p;
};


/* COMPOUND INSTANCES */
/*   Note: All the compound instance structs, except ArrayChild,
 *   and Automatic should have
  enum inst_t t;
  VOIDPTR pending_entry;
 *   as the first two lines to match PendInstance above.
 * Relations of any type are NOT compound, other errors in
 * this file not withstanding.
 */
/** this structure doesn't have to align on 4 byte ptr machines. size 48/88 */
struct ModelInstance {
  enum inst_t t;
  VOIDPTR pending_entry;      /**< hold a word only pending.c can touch */
  VOIDPTR interface_ptr;
  struct gl_list_t *parents;  /**< link to parent instances */
  struct gl_list_t *whens;    /**< link to whens on which the model appears */
  struct TypeDescription *desc;
  struct Instance *alike_ptr;
  struct BitList *executed;   /**< bit list to keep track of which
                                   statements have been executed */
  unsigned long visited;
  unsigned long tmp_num;      /**< used when an instance tree is being copied*/
  unsigned int anon_flags;    /**< anonymous field to be manipulated */
#if (LONGCHILDREN == 1)
  int padding;                /**< so ptrs start packing on 8byte address */
#endif
  /* The child pointers of model instances are packed here, but each is
   * different and there may not even be children. They are not
   * accessible through the struct, but only through InstanceChild.
   */
};

/** This struct not in use anywhere yet, pending more semantics */
struct AutomaticInstance {
  enum inst_t t;
  /* this object never pending since it has no statements */
  VOIDPTR interface_ptr;
  struct Instance *sim_parent;  /**< exactly one of these */
  struct TypeDescription *desc; /**< UNIVERSAL and made at startup */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */

  struct gl_list_t *children;   /**< unique unsorted list of child insts */

  /* struct hashtable *childdata;
   * hash lookup of children.
   */
  /* note that in table there may be multiple names for a child that
   * occurs once in childdata.
   */
  /* HANDS OFF!  --Ben */
};

/** First try of a WhenInstance needs to migrate toward the relations
 *
 * Would be nice to have a commoneqninstance type (CE_INST)
 * for rel,log,when.
 *
 * doesn't align as never has children. 
 */
struct WhenInstance {
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct Instance *parent[2];   /**< relations can have only two parents and
                                     normally they only have one.  They have
                                     two only during an ARE_THE_SAME */
  struct gl_list_t *whens;      /**< used in case of nested whens */
  struct gl_list_t *cases;      /**< list of cases */
  struct gl_list_t *bvar;       /**< list of references to boolean variables  */
  struct TypeDescription *desc; /**< holds the child list stuff */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
};

/** are pooled, and therefore must align, but does so on its own */
struct ArrayChild {
  /* this object never pending, but member inst may be */
  struct Instance *inst;  /**< The instance. */
  union {
    symchar *str;
    long index;           /**< The name, may be string or index. */
  } name;
};

/** never has packed children. see gllist instead. */
struct ArrayInstance {
  enum inst_t t;
  VOIDPTR pending_entry;        /**< hold a word only pending.c can touch */
  VOIDPTR interface_ptr;
  struct TypeDescription *desc; /**< holds the child list stuff */
  struct gl_list_t *parents;    /**< link to parent instances */
  struct gl_list_t *children;   /**< link to children */
  unsigned long indirected;     /**< number of times of indirected */
  unsigned long visited;
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
};

/** CONTAINER INSTANCE for interfaces. never pending.
 * no packed children, so never need align
 */
struct SimulationInstance { 	  
  /* these have *no* parents, yet */
  enum inst_t t;
  VOIDPTR interface_ptr;
  struct TypeDescription *desc; /**< copy of the typedesc of its lone child */
  symchar *name;                /**< name of its lone child */
  struct Instance **extvars;    /**< external variables handles hack */
  unsigned long tmp_num;        /**< used when an instance tree is being copied*/
  unsigned int anon_flags;      /**< anonymous field to be manipulated */
  /* add other interesting stuff here */
};

/** dummy instance for unselected children of models
 * no alignment issue
 */
struct GlobalDummyInstance {
  enum inst_t t;
  /* never pending */
  VOIDPTR interface_ptr;
  /**no link to parent instances */
  /* when instances should ignore dummy completely */
  struct TypeDescription *desc;
  /* all dummies are alike -- and the same */
  /* dummy has not statements */
  unsigned long visited;
  unsigned long ref_count;    /**< mainly for debugging purposes */
  unsigned long tmp_num;      /**< used when an instance tree is being copied
                                   but CopyDummy==dummy always */
  unsigned int anon_flags;    /**< anonymous field to be manipulated */
  /* why make life hell for the unsuspecting client? */
  /*
   * In particular, the DUMMY_INST doesn't look like any category
   * (EQN,Compound,Atom,Constant)  because a single statically allocated
   * instance of it stands in for all UNSELECTED_INSTANCEs that are
   * not compiled.
   * (This has nothing to do with the DummyIndexType.)
   */
};

#endif /* __INSTANCE_TYPES_H_SEEN__ */


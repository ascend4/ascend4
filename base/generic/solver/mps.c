/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Craig Schmidt
	Copyright (C) 2007 Carnegie Mellon University

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
*//** @file
	Description:  This module will create an MPS file representation
	of the current system, and a file mapping
	variable names to MPS names.

	This file is part of the SLV solver.
	The write_MPS routine is passed a mps_data_t
	data structure, the solver subparameters, and the
	name of the file.

	The write_name_map routine creates a file linking the
	ascend name of a variable and CXXXXXXX

 ***       MPS matrix strucutre
 ***                                    v
 ***       min/max cx:  Ax<=b           u
 ***                                    s
 ***                                    e
 ***                       1            d
 ***
 ***                       |            |
 ***                       |            |
 ***                      \ /          \ /
 ***
 ***                      +-            -+
 ***       1          ->  |              |
 ***                      |              |
 ***                      |      A       |
 ***                      |              |
 ***       rused      ->  |              |
 ***                      +-            -+
 ***
 ***       crow       ->  [      c       ]
*//*
	by Craig Schmidt, 2/19/95
	Last in CVS: $Revision: 1.13 $ $Date: 2000/01/25 02:27:03 $ $Author: ballan $
*/

#include "mps.h"

#include <time.h>
#include <errno.h>

#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <utilities/mem.h>

#include "slv_client.h"
#include "slv6.h"

#ifdef STATIC_MPS

/* _________________________________________________________________________ */

/**
 ***  Utility routines
 ***  ----------------------------
 ***  stamp        - stamp file header with a unique timestamp
 ***  process_name - do tilde expansion, check for .mps or .map, etc.
 ***  open_write   - open the designated file for write access
 ***  close_file   - close the file
 ***  print_col_element - print either the first or second column entry
 ***  print_col - prints out a column of data from the Ac_mtx
 **/

static void stamp(FILE *outfile, boolean newstamp, boolean dostamp, boolean dostr)
/**
 ***  Write a unique stamp to file, create a new one
 ***  if newstamp is true
 **/
{
   static char stampstr[26];
   static unsigned long stamptime;
   time_t now;

   now = time(NULL);

   if (newstamp) {  /* generate new stamp */
      stamptime  = (unsigned long) clock();
      sprintf(&stampstr[0], "%-26s", ctime(&now));
   }

   if (dostamp) FPRINTF(outfile,"%-8x", stamptime);  /* only 8 chars for stamp in MPS, so show hex */
   if (dostr)   FPRINTF(outfile," %s", &stampstr);  /* print friendlier form */
}

static FILE *open_write(const char *filename)
/**
 ***  Open file for write access, return NULL if error
 **/
{
  FILE *f;
  errno = 0;
  if (filename == NULL) filename = "\0";  /* shouldn't pass null to fopen */
  f = fopen(filename, "w");
  if( f == NULL ) {
	 FPRINTF(stderr,"ERROR:  (MPS) open_write\n");
	 FPRINTF(stderr,"        Unable to open %s. Error:%s\n",
	         filename, strerror(errno));
  }

  return f;
}


static boolean close_file(FILE *f)
/**
 ***  Close file and handle errors
 **/
{
  int s = 0;
  if(f == NULL) return TRUE;   /* ignore this case */

  errno = 0;
  s = fclose(f);
  if (s == EOF)
  {
  	 FPRINTF(stderr,"ERROR:  (MPS) open_write\n");
             perror("        Unable to close file");
     return FALSE;
  }
  else
     return TRUE;
}


/* define constants for whether this is the first or second value on the line */
#define ONE 0
#define TWO 1

static void print_col_element(FILE *out,
                              int32 var,
                              int32 rel,
                              real64 value)     /* value of mtx element */
/**
 ***  Prints the appropriate variable and relation labels, and
 ***  the value of the element.  If the the variable is different
 ***  from last call of this routine, the first column is used.
 ***  Otherwise it alternates, putting 2 values per line.
 ***
 ***  A value of -1 for var is a special case, meaning that you just
 ***  want a newline if the last call was in the middle of a line
 **/
{
   static int32 oldvar;
   static int         onetwo;  /* is it the first or second value on the line (ONE or TWO) */

   /* set up state */

   if (var == -1) {  /* just write newline if last time were in middle of line */
      if (onetwo == ONE)
         FPRINTF(out,"\n");
      return;
   }

   if (oldvar != var) {  /* are at a new variable */
      onetwo = ONE;
      oldvar = var;
   }
   else
     if  (onetwo == ONE)
          onetwo = TWO;
     else onetwo = ONE;

   /* print full names if in first col, else just the rel label */

   /* Format:  Field 2   Field 3   Field 4    Field 5    Field 6
               (5-12)    (15-22)   (25-36)    (40-47)    (50-61)
               Column     Row 1    Value 1     Row 2     Value 2
               name      name                  name      name
    */

   if (onetwo == ONE)

       /*                   1111   2222    3   */
       /*           1234    2345   2345    6   */
       FPRINTF(out,"    C%07d  R%07d  %12.6G", var, rel, value);
   else
       /*           3334   4445    */
       /*           7890   7890    */
       FPRINTF(out,"   R%07d  %12.6G\n", rel, value);

}


void print_col(FILE *out,               /* file */
          mtx_matrix_t Ac_mtx,     /* Matrix representation of problem */
          char typerow[],          /* array of variable type data */
          int32 curcol,      /* current column number */
          int32 rused,       /* number of relations */
          int32 vused,       /* number of vars */
          int dointeger,           /* supports integer vars */
          int dobinary)            /* supports binary vars */
/**
 ***  Uses print_col_element to print an entire column of the Ac_mtx
 ***  It uses the values of dointeger and dobinary to determine
 ***  if any INTORG markers are needed
 **/
{
   static boolean inBinInt = FALSE;  /* are we currently in a binary or integer var region ? */
   static int     marknum = 0;       /* number for marker label */
   real64 value;               /* value of mtx element */
   mtx_coord_t  nz;                  /* coordinate of row/column in A matrix */
   mtx_range_t  range;               /* define range */
   int32  orgcol;              /* original column number */
   char         coltype;             /* type of current column */
   char         nexttype;            /* type of next column */

   orgcol = mtx_col_to_org(Ac_mtx, curcol);
   coltype = typerow[orgcol];      /* get cur col type */
   if (curcol != vused-1)  /* don't go beyond array limits */
      nexttype = typerow[mtx_col_to_org(Ac_mtx, curcol+1)];
   else
      nexttype = 0;

   if (coltype != SOLVER_FIXED) {  /* only bother with incident, nonfixed variables */

       /* do an INTORG marker if start of binary/integer var */
       if ((! inBinInt) && (((coltype == SOLVER_BINARY) && (dobinary == 0)) ||
	  ((coltype == SOLVER_INT) && (dointeger == 0))))  {
	     inBinInt = TRUE;  /* turn on flag */

	     /*                   1       2         3         4        *
              *           1234    234567890123456789012345678901234567 */
	     FPRINTF(out,"    M%07d  'MARKER'                 'INTORG'\n", marknum);
       }

       nz.row = mtx_FIRST;    /* first nonzero row */
       nz.col = curcol;       /* current col */

       /* note: since mtx_FIRST = mtx_LAST, can't use a while loop */
       value = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,rused));
       do  {
             print_col_element(out, orgcol, mtx_row_to_org(Ac_mtx, nz.row), value);   /* print out a nonzero element */
             value = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,rused));
	   } while (nz.row != mtx_LAST);

       print_col_element(out, -1 , 0, 0.0);   /* clean up newline */

       /* close marker if in last column or next type is not an int or bin */
       if (((inBinInt) && (curcol == (vused-1))) ||
	    (inBinInt && ((nexttype != SOLVER_BINARY) && (nexttype != SOLVER_INT )))) {

             inBinInt = FALSE;  /* turn on flag */
             /*                   1       2         3         4        *
              *           1234    234567890123456789012345678901234567 */
             FPRINTF(out,"    E%07d  'MARKER'                 'INTEND'\n", marknum++);     /* advance number */
       }

   }

}

/* _________________________________________________________________________ */

/**   write_name_map
 ***
 ***  writes out a file mapping the VXXXXXXX variable names
 ***  with the actual ASCEND names
 **/

extern boolean write_name_map(const char *name,        /* filename for output */
                              struct var_variable  **vlist)  /* Variable list (NULL terminated) */
{
  FILE *out;
  int i;

  if ((vlist == NULL) || (name == NULL)) {  /* got a bad pointer */
          FPRINTF(stderr,"ERROR:  (MPS) write_name_map\n");
          FPRINTF(stderr,"        Routine was passed a NULL pointer!\n");
          return FALSE;
  }

  out = open_write(name);
  if (out == NULL)
     return FALSE;

  FPRINTF(out,"ASCEND/MPS Variable Name Mapping\n\n");
  FPRINTF(out,"Timestamp: ");
    stamp(out,FALSE,TRUE,TRUE);   /*  use same stamp as in write_MPS, which was already called */
  FPRINTF(out,"\n");
  FPRINTF(out,"MPS Name   ASCEND Name\n");
  FPRINTF(out,"--------   -----------\n");

  for(; *vlist != NULL ; ++vlist )
     if( free_inc_var_filter(*vlist) )
     {
         FPRINTF(out,"C%07d   ",var_sindex(*vlist));

         /* old way: just the unqualified names */
         /*    slv_print_var_name(out,*vlist);  */

         /* now, from instance_io.h, the full qualified name */
         WriteInstanceName(out, var_instance(*vlist), NULL);
         FPRINTF(out,"\n");
     }

  return close_file(out);

}

/* _________________________________________________________________________ */

/**   routines used by write_MPS
 ***
 ***  do_name - create header NAME section
 ***  do_rows - name constraints: just needs matrix of info
 ***  scan_SOS - identify special ordered sets
 ***  do_columns - create A matrix
 ***  do_rhs - create rhs
 ***  do_bounds - create BOUNDS section
 **/


/* create header */
static void do_name(FILE *out,             /* file */
                    int obj,               /* how does it know to max/min */
                    int bo,                /* QOMILP style cutoff */
                    int eps,               /* QOMILP style termination criteria */
                    double boval,          /* value of cutoff */
                    double epsval)         /* value of termination criteria */
{

/**   Relevant options:
 ***
 ***   sp.ia[SP6_OBJ]      0->solver assumes minimization, do nothing special
 ***                       1->solver assumes maximization, swap obj coeff for min problems
 ***                       2->solver support SCICONIC style MINIMIZE
 ***                       3->solver supports QOMILP style MAX/MIN in names section
 ***
 ***   sp.ia[SP6_BO]       0->no support
 ***                       1->solver supports QOMILP style BO cutoff bound in names section
 ***                       Note: value of bound is in sp.ra[SP6_BNDVAL]
 ***   sp.ia[SP6_EPS]      0->no support
 ***                       1->solver supports QOMILP style EPS termination criterion
 ***                       Note: value of bound is in sp.ra[SP6_EPSVAL]
 ***
 ***   sp.ra[SP6_BOVAL]    value of QOMILP style BO cutoff bound in names section
 ***                       Note: Ignored if sp.ia[SP6_BO]=0
 ***   sp.ra[SP6_EPSVAL]   value of QOMILP style EPS termination criterion
 ***                       Note: Ignored if sp.ia[SP6_EPS]=0
 **/

/* Note: ASCEND assumes we're _minimizing_ the objective */
/* Name can only be 8 characters, so display number in hex */

  switch (obj) {
      case 0:
      case 1:  /* general case: Ok for CPLEXl, OSL, lpsolve */
               FPRINTF(out,"NAME          ");
               stamp(out,TRUE,TRUE,FALSE);   /* create new timestamp, name map comes later */
               FPRINTF(out,"\n");
               break;

      case 2:  /* with SCIONIC use MINIMISE */
               FPRINTF(out,"NAME          ");
               stamp(out,TRUE,TRUE,FALSE);   /* create new timestamp, name map comes later */
               FPRINTF(out,"MINIMISE\n");     /* British spelling */
               break;

      case 3:  /* with QOMILP use MIN */
               FPRINTF(out,"NAME          ");
               stamp(out,TRUE,TRUE,TRUE);   /* create new timestamp, name map comes later */
               FPRINTF(out,"\n");
               FPRINTF(out," MIN\n");          /* optimization direction */
               break;

      default: FPRINTF(stderr,"ERROR:  (MPS) do_name\n");
               FPRINTF(stderr,"        Unknown option for objective!\n");
  }

  if (bo == 1)
     FPRINTF(out," BO                     %12.8f\n", boval);  /* Numbers in 25-36 for QOMILP */

  if (eps == 1)
     FPRINTF(out," EPS                    %12.2f\n", epsval);

}


static void do_rows(FILE *out,             /* file */
                    char relopcol[],       /* array of data */
                    int32 rused)     /* size of array */
{
   int i;

   FPRINTF(out,"ROWS\n");        /* section header */

   for (i = 0; i < rused; i++)
       switch (relopcol[i]) {
          case rel_TOK_less:        FPRINTF(out," L  R%07d\n", i);
                                    break;
          case rel_TOK_equal:       FPRINTF(out," E  R%07d\n", i);
                                    break;
          case rel_TOK_greater:     FPRINTF(out," G  R%07d\n", i);
                                    break;
          case rel_TOK_nonincident: break;

          default: FPRINTF(stderr,"ERROR:  (MPS) do_rows\n");
                   FPRINTF(stderr,"        Unknown value for relational operators!\n");
      }

   FPRINTF(out," N  R%07d\n", rused);     /* objective row */

}


static void upgrade_vars(FILE *out,             /* file */
        		 char typerow[],        /* array of variable type data */
                         real64 ubrow[],  /* to change ub on int->bin */
        		 int32 vused,     /* number of vars */
                         int relaxed,           /* should the relaxed problem be solved */
        		 int dointeger,         /* supports integer vars */
        		 int dobinary,          /* supports binary vars */
        		 int dosemi)            /* supports semi-continuous vars */

/**   This very simple routine checks to see if a variables type is currently
 ***  supported.  If not, it converts it into a type which is, printing a
 ***  warning about the conversion to stderr.
 ***
 ***  It does bin -> integer
 ***          integer -> bin
 ***          semi -> solver_var
 ***          bin -> solver_var
 ***          integer -> solver_var
 ***  conversions, as appropriate.
 **/

{
   int          orgcol;                        /* original column number */

   for(orgcol = 0; orgcol < vused; orgcol++)   /* loop over all columns, is _original_ column number */
       if (typerow[orgcol] != SOLVER_FIXED) {  /* only bother with incident variables */

          if ((relaxed == 1) && ((typerow[orgcol] == SOLVER_BINARY) ||   /* if relaxed convert it to solver_var */
               (typerow[orgcol] == SOLVER_INT) ||
               (typerow[orgcol] == SOLVER_SEMI)))
                typerow[orgcol] = SOLVER_VAR;
          else

          /* upgrade types as appropriate here */
 	  /* if defined a binary var, and solver only supports integer vars, "upgrade" to an int var, etc. */
	  if ((typerow[orgcol] == SOLVER_BINARY) && (dointeger != 2) && (dobinary == 2))  {
               typerow[orgcol] = SOLVER_INT;
          }
	  else if ((typerow[orgcol] == SOLVER_INT) && (dointeger == 2) && (dobinary != 2))  {
	       FPRINTF(stderr,"WARNING: Variable C%07d was treated as a %s instead of a %s.\n", orgcol, SOLVER_BINARY_STR, SOLVER_INT_STR);
	       FPRINTF(stderr,"         The selected MILP solver does not support %s.\n", SOLVER_INT_STR);
	       FPRINTF(stderr,"         Upper bound was set to 1.0.\n");
               typerow[orgcol] = SOLVER_BINARY;
               ubrow[orgcol] = 1.0;   /* note: changed bound */
          }
	  else if ((typerow[orgcol] == SOLVER_SEMI) && (dosemi == 0))  {   /* semi not supported */
	       FPRINTF(stderr,"WARNING: Variable C%07d was converted from a %s to a %s.\n", orgcol, SOLVER_SEMI_STR, SOLVER_VAR_STR);
	       FPRINTF(stderr,"         The selected MILP solver does not support %s.\n", orgcol, SOLVER_SEMI_STR);
	       FPRINTF(stderr,"         The solution found may not be correct for your model.\n");
               typerow[orgcol] = SOLVER_VAR;
          }
	  else if ((typerow[orgcol] == SOLVER_BINARY) && (dointeger == 2) && (dobinary ==2))  {  /* neither is supported */
	       FPRINTF(stderr,"WARNING: Variable C%07d was treated as a %s instead of a %s.\n", orgcol, SOLVER_VAR_STR, SOLVER_BINARY_STR);
	       FPRINTF(stderr,"         The selected MILP solver only supports %s.\n", SOLVER_VAR_STR);
               typerow[orgcol] = SOLVER_VAR;
          }
	  else if ((typerow[orgcol] == SOLVER_INT) && (dointeger == 2) && (dobinary == 2))  {  /* neither is supported */
	       FPRINTF(stderr,"WARNING: Variable C%07d was treated as a %s instead of a %s.\n", orgcol, SOLVER_VAR_STR, SOLVER_INT_STR);
	       FPRINTF(stderr,"         The selected MILP solver only supports %s.\n", SOLVER_VAR_STR);
               typerow[orgcol] = SOLVER_VAR;
          }
       }
}



void scan_SOS(mtx_matrix_t Ac_mtx,     /* Matrix representation of problem */
              char relopcol[],         /* array of relational operator data */
              real64 bcol[],     /* array of RHS data */
              char typerow[],          /* array of variable type data */
              int32 rused,       /* size of arrays */
              int32 vused,       /* number of vars */
              int32 *sosvarptr,  /* output: number of variables used
                                          in SOS's */
              int32 *sosrelptr)  /* output: number of relations
                                          defining SOS's */

/**   This routine identifies relations which are special ordered sets.
 ***  It used the Ac_mtx matrix, and reorganizes it into the following
 ***  format:
 ***                       1            sosvar      vused
 ***                       |            |           |
 ***                       +            +           +
 ***              1  ->  | xxxxx                     |
 ***                     |      xxx                  |
 ***                     |         xxxx              |
 ***         sosrel  ->  |             xx            |
 ***                     |   x   x         x x    x  |
 ***                     | x       x        x  x     |
 ***                     | x    x    x    x    x     |
 ***                     |   x x         x    x    x |
 ***                     | x       x   x  x          |
 ***          rused  ->  | x    x    x    x    x     |
 ***          crow
 ***
 ***     sosvar = number of vars involved in SOS's
 ***     sosrel = number of SOS equations
 ***
 ***  The nomenclature of SOS's is really confused.  The number used
 ***  depends on the solver/modeling language in use.  In this case we are
 ***  looking for equations of the form sum(i, x[i]) = 1, which I'll call
 ***  a SOS1, and sum(i, x[i]) <= 1, which I'll call a SOS3.
 ***
 ***  First, this routine checks to see if a relation is >=.  If so, it
 ***  can't be SOS, so that row is skipped.  Next, it checks to see
 ***  if the RHS is 1.  If not, that row is skipped.
 ***
 ***  Note that overlapping SOS's are not allowed by the MPS format.
 ***  That is, a variable can only be part of one SOS.
 ***
 ***  This routine checks to be sure that all vars involved are binary
 ***  or integer. If so, the row is a SOS. If not, it's just a regular
 ***  constraint.  (It doesn't explicitly check the upper bounds on
 ***  integer vars, since if their not 1 the model will be infeasible.)
 ***
 ***  Note that this routine uses the current row/col, not the original like most
 ***  other routines.  The columns are reorganized so that all vars of a SOS are
 ***  together, as shown above.
 **/

{
   real64 value;        /* value of mtx element */
   mtx_coord_t  nz;           /* coordinate of row/column for going down RHS column */
   mtx_range_t  range;        /* storage for range of A matrix, run down a column */
   int32  current_row;  /* counter for row being examined */
   int32  current_col;  /* counter for column being examined */
   int32  not_row;      /* counter for row at end of matrix which is not an SOS */
   boolean      isSOS;        /* is the current row a SOS ?  */

   current_row    = 0;        /* initialize stuff */
   not_row        = rused-1;  /* rel 0 to rused-1 are actually used */
   current_col    = 0;

   while (current_row <= not_row)   /* examin each row of the A matrix once */
     {
         if ((bcol[mtx_row_to_org(Ac_mtx, current_row)] == 1.0) && (relopcol[mtx_row_to_org(Ac_mtx, current_row)] != rel_TOK_greater))   /* see if row coefficients ok */
         {
               /* it is a SOS if  all coefficients are 1,
                  and all columns with nonzero coeff are >= current_col
                  i.e. the equation doesn't contain any vars from prev SOS equations */

               isSOS = TRUE;
               nz.col = mtx_FIRST;       /* first nonzero col (1) */
               nz.row = current_row;     /* use current row */

               do  {  /* loop until done or find a value != 1.0 */

                   value = mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,vused));
                   if  ((nz.col != mtx_FIRST) && (nz.col != mtx_LAST)) {
                	 if ( nz.col < current_col)  {
                        	   isSOS = FALSE;  /* overlaps prev SOS */
                        	   FPRINTF(stderr, "nz.col, current_col, mtx_FIRST: %d  %d  %d\n", nz.col, current_col, mtx_FIRST);
                         }
                	 if ((typerow[mtx_col_to_org(Ac_mtx, nz.col)] != SOLVER_BINARY) &&
                	     ( typerow[mtx_col_to_org(Ac_mtx, nz.col)] != SOLVER_INT)) {
                               isSOS = FALSE;  /* var is wrong type */
                               FPRINTF(stderr, "typerow: %d\n", typerow[mtx_col_to_org(Ac_mtx, nz.col)]);
                	 }
                   }

               } while ( (value == 1.0) && (nz.row != mtx_LAST) && isSOS );

               if (nz.col != mtx_LAST) isSOS = FALSE;  /* only true if terminated due to mxt_LAST */
               FPRINTF(stderr, "isSOS,nz.col:%d, %d\n", isSOS,nz.col);

         }
         else
            isSOS = FALSE;

         if (isSOS)  /* reorder columns so all line up in first cols */
         {
             FPRINTF(stderr, "current_row, not_row:%d, %d\n", current_row, not_row);
             /* Is a SOS, so rearrange columns so all the vars in the equation
                are from current_col on.  Also advance current_row by one. */

               nz.col = mtx_FIRST;            /* first nonzero col */
               nz.row = current_row;          /* use current row */

               mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,vused));
               while( nz.col != mtx_LAST) {
                      mtx_swap_cols(Ac_mtx, nz.col, current_col++);
                      mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,vused));
               }
               current_row++;   /* now examine next row */
         }
         else
         {
             /* Isn't a SOS, so swap current_row with not_row,
                to get it out of the way.
                Also decrement not_row by one */

             mtx_swap_rows(Ac_mtx, current_row, not_row--);

         }
     }

     *sosvarptr = current_col;
     *sosrelptr = current_row;
}


void do_columns(FILE *out,               /* file */
                mtx_matrix_t Ac_mtx,     /* Matrix representation of problem */
                char typerow[],          /* array of variable type data */
                int32 rused,       /* number of relations */
                int32 vused,       /* number of vars */
                int32 sosvar,      /* number of variables used in SOS's */
                int32 sosrel,      /* number of relations that
                                            are SOS's */
                int dointeger,           /* supports integer vars */
                int dobinary)            /* supports binary vars */
/***
 ***  Creates the main A matrix of data. It handles SOS's, relying on
 ***  the values of sosvar and sosrel (which should be 0 is there aren't
 ***  any SOS's).  It mostly just calls print_col, the appropriate number
 ***  of times.
 ***/
{
   mtx_range_t  range;    /* storage for range of A matrix, run down a column */
   int32  curcol;   /* counter for current column */
   int32  currow;   /* current row number */
   int          sosnum;   /* number used in marker labels */
   int          upcol;    /* upper counter on column in SOS */

   FPRINTF(out,"COLUMNS\n");     /* section header */

   sosnum = 0;
   curcol = 0;

   /* loop over every SOS */
   for (currow = 0; currow < sosrel ; currow++) {

       /*           0       1       2         3         4              *
        *           1234    234567890123456789012345678901234567       */
       FPRINTF(out,"    M%07d  'MARKER'                 'SETORG'\n", sosnum);

       /* figure out end point of next SOS */
       upcol = mtx_nonzeros_in_row(Ac_mtx,currow,mtx_range(&range,0,vused-1)) + curcol;

       for ( ; curcol < upcol; curcol++)
            print_col(out, Ac_mtx, typerow, curcol, rused, vused, dointeger, dobinary);

       /*           0       1       2         3         4        *
        *           1234    234567890123456789012345678901234567 */
       FPRINTF(out,"    E%07d  'MARKER'                 'SETEND'\n", sosnum++);

   }

   /* loop rest of all columns, not in a SOS */
   for( ; curcol < vused; curcol++)
       print_col(out, Ac_mtx, typerow, curcol, rused, vused, dointeger, dobinary);

}



void do_rhs(FILE *out,                /* file */
            real64 bcol[],      /* array of data */
            char relopcol[],          /* is it incident? */
            int32 rused,        /* size of array */
            int32 vused)        /* number RHS column vused+1 */

/***
 ***  Prints out the right hand side data
 ***/
{
   int i;

   FPRINTF(out,"RHS\n");                                  /* section header */

   for(i = 0; i < rused; i++)                             /* loop over all rows */
       if (relopcol[i] != rel_TOK_nonincident)            /* if it is incident, nonfixed */
          print_col_element(out, vused , i, bcol[i]);     /* then print out stuff */

   print_col_element(out, -1 , 0, 0.0);                   /* clean up newline */

}

void do_bounds(FILE *out,              /* file */
               real64 lbrow[],   /* array of data */
               real64 ubrow[],   /* array of data */
               char typerow[],         /* array of data */
               int32 rused,      /* size of arrays */
               int nonneg,             /* allow nonneg vars (no FR or MI) ? */
               int binary_flag,        /* allow BV vars ? */
               int integer_flag,       /* allow UI vars ? */
               int semi_flag,          /* allow SC vars ? */
               double pinf,            /* any UB>=pinf is set to + infinity */
               double minf)            /* any LB<=minf is set to - infinity */

/***    This routine creates the BOUNDS section of the MPS file.
 ***    It checks the following flags:
 ***
 ***    iarray[SP6_NONNEG]   0->solver handles free vars
 ***                         1->solver requires that all vars have LB=0,
                                UB=infinity, no FR or MI
 ***    iarray[SP6_BINARY]   0->solver supports binary variables using INTORG
 ***                         1->solver supports binary variables with
                                BV option in BOUNDS
 ***                         2->no support
 ***    iarray[SP6_INTEGER]  0->solver defines integer vars using INTORG
 ***                         1->solver defines integer vars using UI in BOUNDS
 ***                         2->no support for integer vars
 ***    iarray[SP6_SEMI]     0->no support
 ***                         1->solver supports SCICONIC style semi-
                               continuous vars
 ***    rarray[SP6_PINF]     any UB >= pinf is set to + infinity
 ***    rarray[SP6_MINF]     any LB <= minf is set to - infinity
 **
 ***    The following formats are defined:
 ***
 ***    LO  Lower bound              bj <= xj ( <= pinfinity)
 ***    UP  Upper bound              (0 <= ) xj < bj
 ***    FX  Fixed variable           xj = bj
 ***    FR  Free variable            minfinity < xj < pinfinity
 ***    MI  Lower bound minfinity    minfinity < xj ( <= 0)
 ***    PL  Upper bound pinfinity    (0 <= ) xj <= pinfinity
 ***
 ***    Where default bounds of 0 or infinity are shown in ()
 ***    A default of PL is assumed for all variables, so it is not
 ***    explicitly writen to the file
 ***/
{
   int i;

   FPRINTF(out,"BOUNDS\n");                             /* section header */

   for(i = 0; i < rused; i++)                          /* loop over all rows */
   {
     if ((typerow[i] == SOLVER_BINARY) && (binary_flag == 1))   /* do BV */
              FPRINTF(out," BV B%07d  C%07d\n",i,i);
     else if ((typerow[i] == SOLVER_INT) && (integer_flag == 1))  /* do UI */
              {
                 FPRINTF(out," UI B%07d  C%07d  %12.6G\n",i,i,ubrow[i]);
                 if (lbrow[i] != 0.0)   /* LB of 0 is assumed, so don't need to add it */
                    FPRINTF(out," LO B%07d  C%07d  %12.6G\n",i,i,lbrow[i]);
              }
     else if ((typerow[i] == SOLVER_SEMI) && (semi_flag == 1))      /* do SC, upper bound is value */

              FPRINTF(out," SC B%07d  C%07d  %12.6G\n",i,i,ubrow[i]);

     else if ((ubrow[i] >= pinf) && (lbrow[i] <= minf) && (nonneg == 0)) /* do FR */

               FPRINTF(out," FR B%07d  C%07d\n",i,i);

     else if ((ubrow[i] <= pinf) && (lbrow[i] <= minf) && (nonneg == 0)) /* do MI */
              {
                 FPRINTF(out," MI B%07d  C%07d\n",i,i,ubrow[i]);
                 if (ubrow[i] != 0.0)   /* UB of 0 is assumed, so don't need to add it */
                    FPRINTF(out," UP B%07d  C%07d  %12.6G\n",i,i,ubrow[i]);
              }
     else if (ubrow[i] == lbrow[i])  /* do FX */

              FPRINTF(out," FX B%07d  C%07d  %12.6G\n",i,i,ubrow[i]);

     else if ((ubrow[i] >= pinf) && (lbrow[i] == 0.0)) /* do PL */

               continue;  /* are default limits, no bound necessary */

     else   /* do normal UB and LB */
     {
         if (lbrow[i] != 0.0)   /* LB of 0 is assumed, so don't need to add it */
            FPRINTF(out," LO B%07d  C%07d  %12.6G\n",i,i,lbrow[i]);

         if (ubrow[i] <= pinf)   /* UB of + infinity is assumed, so don't need to add it */
            FPRINTF(out," UP B%07d  C%07d  %12.6G\n",i,i,ubrow[i]);
     }

   }

}


/* _________________________________________________________________________ */

/* writes out an MPS file */

  extern boolean write_MPS(const char *name,                /* filename for output */
                           mps_data_t mps,                  /* the main chunk of data for the problem */
                           int iarray[slv6_IA_SIZE],        /* Integer subparameters */
                           double rarray[slv6_RA_SIZE])     /* Real subparameters */
{
  FILE *out;
  int32 sosvar;   /* number of variables used in SOS's */
  int32 sosrel;   /* number of relations defining SOS's */
  int i;                /* temporary counter */

  if (name == NULL) {  /* got a bad pointer */
          FPRINTF(stderr,"ERROR:  (MPS) write_MPS\n");
          FPRINTF(stderr,"        Routine was passed a NULL pointer!\n");
          return FALSE;
  }

  out = open_write(name);
  if (out == NULL) return FALSE;

  /* create header */
  do_name(out,                 /* file */
          iarray[SP6_OBJ],     /* how does it know to max/min */
          iarray[SP6_BO],      /* QOMILP style cutoff */
          iarray[SP6_EPS],     /* QOMILP style termination criteria */
          rarray[SP6_BOVAL],   /* value of cutoff */
          rarray[SP6_EPSVAL]); /* value of termination criteria */

  do_rows(out,            /* file */
          mps.relopcol,   /* need type of constraint <=, >=, = */
          mps.rinc);      /* number of incident relations */

  upgrade_vars(out,                  /* file */
               mps.typerow,          /* array of variable type data */
               mps.ubrow,            /* change ub on int -> bin conversion */
               mps.vused,            /* number of vars */
               iarray[SP6_RELAXED],  /* should the relaxed problem be solved */
               iarray[SP6_INTEGER],  /* supports integer vars */
               iarray[SP6_BINARY],   /* supports binary vars */
               iarray[SP6_SEMI]);    /* supports semi-continuous vars */

  if ((iarray[SP6_SOS1] == 1) || (iarray[SP6_SOS3] == 1))  /* look for SOS's, reorder matrix */
     scan_SOS(mps.Ac_mtx,     /* Matrix representation of problem */
              mps.relopcol,   /* array of relational operator data */
              mps.bcol,       /* array of RHS data */
              mps.typerow,    /* array of variable type data */
              mps.rinc,       /* size of incident relations */
              mps.vinc,       /* number of vars */
              &sosvar,        /* output: number of variables used in SOS's */
              &sosrel);       /* output: number of relations defining SOS's */
     else {
              sosvar = 0;     /* set for use in do_columns */
              sosrel = 0;
     }

  if (iarray[SP6_SOS2] == 1)  {    /* don't support SOS2 yet */
       FPRINTF(stderr,"WARNING:  (MPS) write_MPS\n");
       FPRINTF(stderr,"          SOS2 are not currently supported in ASCEND!\n");
  }

  do_columns(out,                    /* file */
             mps.Ac_mtx,        /* Matrix representation of problem */
             mps.typerow,       /* array of variable type data */
             mps.rused,         /* number of relations */
             mps.vused,         /* number of vars */
             sosvar,            /* number of variables used in SOS's */
             sosrel,            /* number of SOS's */
             iarray[SP6_INTEGER],    /* supports integer vars */
             iarray[SP6_BINARY]);    /* supports binary vars */

  do_rhs(out,                 /* file */
         mps.bcol,
         mps.relopcol,
         mps.rinc,
         mps.vinc);

  do_bounds(out,                   /* file */
            mps.lbrow,        /* array of data */
            mps.ubrow,        /* array of data */
            mps.typerow,      /* array of data */
            mps.rused,        /* size of arrays */
            iarray[SP6_NONNEG],    /* allow nonneg vars (no FR or MI) ? */
            iarray[SP6_BINARY],    /* allow BV vars ? */
            iarray[SP6_INTEGER],   /* allow UI vars ? */
            iarray[SP6_SEMI],      /* allow SC vars ? */
            rarray[SP6_PINF],      /* any UB>=pinf is set to + infinity */
            rarray[SP6_MINF]);     /* any LB<=minf is set to - infinity */

  FPRINTF(out, "ENDATA\n");  /* finish up the file */

  return close_file(out);
}

#endif



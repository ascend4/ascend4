/*
 *  numlist.h
 *  by Ben Allan
 *  December 20, 1997
 *  Part of ASCEND
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: numlist.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:44 $
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

/* this header requires general/list.h */
/*
 * building with -DNUMPAIRSELFTEST causes numlist to
 * compile a simple main().
 */
#ifndef __NUMPAIR_H_SEEN__
#define __NUMPAIR_H_SEEN__

/*
 *  NUMLISTUSESPOOL == TRUE allows the list module to use pool.[ch] to
 *  manage list memory overhead. Performance is enhanced this way.
 *
 *  NUMLISTUSESPOOL == FALSE removes the pool dependency completely, at
 *  a performance penalty.
 */
#define NUMLISTUSESPOOL TRUE

/*
 * Numlists are lists of integer scalars and ranges.
 * Valid numbers are 1..INT_MAX.
 * Numbers are stored in increasing order, and condensed to ranges
 * where possible.
 * For storage efficiency, only lists you request to be expandable
 * are expandable. All others are of a fixed length.
 * Expandable numlists are noted enlp, and regular cheap ones are just nlp.  
 */
typedef struct numpair_list *Numlist_p;

/*
 * we have an iterator defined for Numlist_p's. It takes a function
 * of the following kind.
 */
typedef void (*NPLFunc)(int, void *);

/*
 * enlp = NumpairExpandableList(enlp,size);
 * Expands the size of nlp, which must be created
 * with NumpairExpandableList. If nlp is NULL, creates
 * the list with the size specified.
 * If insufficient memory to create or expand to size
 * required, returns NULL.
 * Size is the total number of separate scalars and
 * ranges that might be desired. 
 */
extern Numlist_p NumpairExpandableList(Numlist_p,int);

/* NumpairDestroyList(nlp); NumpairDestroyList(enlp);
 * Destroy a list. list may have come from
 * NumpairCopyList or NumpairExpandableList.
 */
extern void NumpairDestroyList(Numlist_p);

/*
 * nlp = NumpairElementary(index);
 * Returns an efficiently allocated numlist containing the
 * scalar with value index. nlp is not expandable.
 */
extern Numlist_p NumpairElementary(int);

/*
 * nlp2 = NumpairCopyList(nlp);
 * Numlist_p nlp2, nlp;
 * Returns an efficiently allocated numpair_list containing the
 * data of the list given. The data in this list may or may not
 * be in a shared allocation, depending on the list size.
 * In either case, it is not expandable.
 */
extern Numlist_p NumpairCopyList(Numlist_p);

/*
 * NumpairCalcUnion(enlp1,nlp2,scratchenlp);
 * Numlist_p enlp1,nlp2,scratchenlp;
 * Calculates the union of enlp1, nlp2 and leaves the result in
 * enlp1. scratchenlp is used if needed and is left in an indeterminate
 * state.
 */
extern void NumpairCalcUnion( Numlist_p, Numlist_p, Numlist_p);

/*
 * NumpairCalcIntersection(nlp1,nlp2,enlp3);
 * Numlist_p nlp1,nlp2,enlp3;
 * Calculates the intersection of nlp1, nlp2 and leaves the result in enlp3. 
 */
extern void NumpairCalcIntersection(Numlist_p, Numlist_p, Numlist_p);

/*
 * nlp = NumpairCombineLists(nlpgl,s1,s2);
 * Numlist_p s1,s2;
 * struct gl_list_t *nlpgl;
 * Numlist_p nlp;
 * Takes a gl_list of Numlist_p and merges the data
 * from all of them into one list, which is allocated
 * in the efficient fashion of NumpairCopyList.
 * If nlpgl is empty, returns NULL.
 * The arguments s1, s2 must be two numlists created to be
 * expandable with NumpairExpandableList.
 * They are scratch spaces. The data in them on return is
 * unpredictable.
 */
extern Numlist_p NumpairCombineLists(struct gl_list_t *, Numlist_p,Numlist_p);

/*
 * NumpairAppendList(enlp,num);
 * Inserts a num to an expandable numlist.
 * typically O(1), sometimes O(len(enlp)).
 */
extern void NumpairAppendList(Numlist_p, int);

/*
 * int NumpairListLen(nlp); 
 * Returns the number of scalars and ranges currently
 * stored in nlp. List capacity may be larger if nlp is
 * expandable, but you do not need to know that.
 */
extern int NumpairListLen(Numlist_p);

/*
 * NumpairClearList(enlp); 
 * Resets the number of elements stored in enlp to 0.
 * List capacity may obviously be larger.
 * enlp must be expandable.
 */
extern void NumpairClearList(Numlist_p);

/*
 * NumpairNumberInList(nlp,number);
 * Returns 1 if number is in list and 0 if it is not.
 * Uses a binary search.
 */
extern int NumpairNumberInList(Numlist_p,int);

/*
 * NumpairNumberInListHintedDecreasing(nlp,number,hint);
 * int number, *hint;
 * Returns 1 if number is in list at or to the left of
 * hint. hint is ignored for small lists. 
 * To initiate a series of searches, call with *hint == -1.
 * Cost O(len) per call worst case, but O(1) if used * properly.
 * Note that if hint value is incorrect, this may lie about 
 * whether number is in the list.
 */
extern int NumpairNumberInListHintedDecreasing(Numlist_p, int, int *);

/*
 * prev = NumpairPrevNumber(nlp,last,hint);
 * int *hint;
 * int last;
 * Returns the next lower number in the list preceding
 * last. If last is 0, returns highest
 * number in the list. *hint should be the output from the
 * last call to this function on nlp, or -1.  This function lets
 * you write a list iteration.
 * Remember that 0 is never a valid list element.
 * (0 may be a valid *hint, however.)
 * If last is not found in the list, then returns 0.
 */
extern int NumpairPrevNumber(Numlist_p,int,int *);

/*
 * prev = NumpairNextNumber(nlp,last,hint);
 * int *hint;
 * int last;
 * Returns the next higher number in the list following
 * last. If last is >= end of list, wraps around and returns 0.
 * *hint should be the output from the
 * last call to this function on nlp, or 0.  This function lets
 * you write a list iteration.
 * Remember that 0 is never really a valid list element.
 */
extern int NumpairNextNumber(Numlist_p,int,int *);

/* NumpairListIterate(nlp,func,userdata);
 * Calls func(i,userdata) for every integer i listed in nlp.
 */
extern void NumpairListIterate(Numlist_p, NPLFunc, void *);

/*
 * common = NumpairGTIntersection(nlp1,nlp2,lowlimit);
 * int lowlimit;
 * Returns the first number that is both common to nlp1, nlp2
 * and >= lowlimit.
 * If no number > lowlimit is common, returns 0.
 */
extern int NumpairGTIntersection(Numlist_p,Numlist_p,int);

/*
 * last = NumpairIntersectionLTHinted(nlp1,&hint1,nlp2,&hint2,highlimit);
 * Return the highest intersection of nlp1 and nlp2 with value < highlimit
 * and using hint1, hint2 from previous calls on the same list to simplify
 * the search. On the first call of a series in the same list pair with
 * DECREASING highlimit hint1 and hint2 should be -1 and highlimit
 * should be 0 or INT_MAX. If no intersection is found, returns 0.
 */
extern int NumpairIntersectionLTHinted(Numlist_p, int *, Numlist_p, int *, int);

/*
 * Returns the count of integers represented by the list.
 * Tolerates all sorts of crappy input and returns 0 in those cases.
 */
extern int NumpairCardinality(Numlist_p);

/*
 * NumpairClearPuddle();
 * Clears up the internal queue of lists that have room left in
 * them to share with other nonexpandable lists.
 * This should be called before exiting the compiler.
 * Clearing the puddle will not disturb lists still in use,
 * it will merely prevent them from being used as puddle
 * donors.
 */
extern void NumpairClearPuddle(void);

#ifdef NUMLISTEXPORTIO
/* temporarily exported function for debugging
 * NLPWrite(fp,nlp);
 * fp may be NULL, --> stderr output.
 */
extern void NLPWrite(FILE *,Numlist_p);
#define NLPWRITE 1
#endif

#endif /* __NUMPAIR_H_SEEN__ */

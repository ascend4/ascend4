/*
 *  General Plot Functions
 *  Interface pointer functions for Ascend
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: plot.c,v $
 *  Date last modified: $Date: 1998/02/18 21:11:53 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Programming System.
 *
 *  Copyright (C) 1990 Thomas Guthrie Epperly, Karl Michael Westerberg
 *  Copyright (C) 1994 Kirk Andre Abbott, Benjamin Andrew Allan
 *
 *  The Ascend Programming System is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 *  This module defines the plot generation auxillaries for Ascend.
 *
 */

#include "utilities/ascConfig.h"
#include "general/list.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/dimen.h"
#include "compiler/atomvalue.h"
#include "compiler/instance_name.h"
#include "compiler/parentchild.h"
#include "compiler/instquery.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/module.h"
#include "compiler/library.h"
#include "interface/plot.h"

/*
 *  ASSUMPTION ASSUMPTION ASSUMPTION:
 *  The points are to be displayed in the order that ASCEND
 *  internally sorts arrays, i.e. increasing integer subscript value.
 *
 *  By making these assumptions, we remove the strict requirement
 *  that the curves have points indexed 1..n. We might, after all,
 *  want to index from 0.
 */
enum PlotTypes g_plot_type;

static symchar *g_strings[16];
#define PLOT_POINT_SYM  g_strings[0]
#define PLOT_POINT_INT  g_strings[1]
#define PLOT_TITLE      g_strings[2]
#define PLOT_XLABEL     g_strings[3]
#define PLOT_YLABEL     g_strings[4]
#define PLOT_XLOG       g_strings[5]
#define PLOT_YLOG       g_strings[6]
#define PLOT_XLO        g_strings[7]
#define PLOT_YLO        g_strings[8]
#define PLOT_XHI        g_strings[9]
#define PLOT_YHI        g_strings[10]
#define PLOT_CURVE      g_strings[11]
#define PLOT_LEGEND     g_strings[12]
#define PLOT_POINT      g_strings[13]
#define PLOT_XPOINT     g_strings[14]
#define PLOT_YPOINT     g_strings[15]

static void init_gstrings(void)
{
  PLOT_POINT_SYM = AddSymbol("plt_plot_symbol");
  PLOT_POINT_INT = AddSymbol("plt_plot_integer");
  PLOT_TITLE     = AddSymbolL("title",5);
  PLOT_XLABEL    = AddSymbolL("XLabel",6);
  PLOT_YLABEL    = AddSymbolL("YLabel",6);
  PLOT_XLOG      = AddSymbolL("Xlog",4);
  PLOT_YLOG      = AddSymbolL("Ylog",4);
  PLOT_XLO       = AddSymbolL("Xlow",4);
  PLOT_YLO       = AddSymbolL("Ylow",4);
  PLOT_XHI       = AddSymbolL("Xhigh",5);
  PLOT_YHI       = AddSymbolL("Yhigh",5);
  PLOT_CURVE     = AddSymbolL("curve",5);
  PLOT_LEGEND    = AddSymbolL("legend",6);
  PLOT_POINT     = AddSymbolL("pnt",3);
  PLOT_XPOINT    = AddSymbolL("x",1);
  PLOT_YPOINT    = AddSymbolL("y",1);
}

/*
 * returns TRUE if the type of inst is more refined than the type of
 * plt_point_int or plt_point_sym
 */
boolean plot_allowed(struct Instance *inst) {
  struct TypeDescription *type, *plt_type_s, *plt_type_i;
  if (inst==NULL) {
    return 0;
  }
  init_gstrings(); /* set up symchars for the child queries */
  plt_type_s = FindType(PLOT_POINT_SYM);   /* look for plt_point_sym */
  plt_type_i = FindType(PLOT_POINT_INT);   /* look for plt_point_int */
  if (plt_type_i == NULL && plt_type_s == NULL) {
    return 0;     /* no plots => fail */
  }
  type = InstanceTypeDesc(inst);
  if (type==NULL) {
     return 0; /* atom children have not type */
  }
  /* type is more refined than symbol plot */
  if ((plt_type_s && type == MoreRefined(type,plt_type_s))||
      (plt_type_i && type == MoreRefined(type,plt_type_i))) {
    return 1;
  }
  return 0;
}


/*
 * A very similar function such as the one below is defined in BrowserProc.c.
 * We need to do some reorganization. kaa.
 */
static unsigned long ChildNumberbyChar(struct Instance *i, symchar *name)
{
  struct InstanceName rec;
  unsigned long c = 0;
  unsigned long nch = 0;
  long index;

  if(i==NULL||name==NULL) {
    FPRINTF(stderr,"Null Instance or name in ChildbyNameChar\n");
    return 0;
  }
  assert(AscFindSymbol(name)!=NULL);
  nch = NumberChildren(i);
  if(!nch) return 0;
  do {
    c++;
    rec = ChildName(i,c);
    switch (InstanceNameType(rec)){
    case StrName:
      if (InstanceNameStr(rec) == name) {
        return c;
      }
      break;
    case IntArrayIndex:
      index = atol(SCP(name));
      if (index==InstanceIntIndex(rec)) {
        return c;
      }
      break;
    case StrArrayIndex:
      if (InstanceStrIndex(rec) == name) {
        return c;
      }
      break;
    }
  } while(c < nch);
  return 0; /*NOTREACHED*/
}

static
void do_plot_legends(FILE *fp, struct Instance *i, symchar *label)
{
  struct Instance *str_inst;
  unsigned long ndx;
  ndx = ChildNumberbyChar(i,label);
  if (ndx) {
    str_inst = InstanceChild(i,ndx);
    if (AtomAssigned(str_inst)) {
      switch(g_plot_type) {
      case PLAIN_PLOT:
        FPRINTF(fp,"\n\n");
        break;
      case GNU_PLOT:
        FPRINTF(fp,"\n#\"%s\"\n",SCP(GetSymbolAtomValue(str_inst)));
        break;
      case XGRAPH_PLOT:
        FPRINTF(fp,"\n\"%s\n",SCP(GetSymbolAtomValue(str_inst)));
        break;
      default:
        break;
      }
    } else {
      FPRINTF(fp,"\n\n");
    }
  }
}

static
void do_plot_labels(FILE *fp, struct Instance *i, symchar *label,
                    char *labelstring)
{
  struct Instance *inst;
  unsigned long ndx;
  assert(AscFindSymbol(label)!=NULL);
  ndx = ChildNumberbyChar(i,label);
  if (ndx) {
    inst = InstanceChild(i,ndx);
    if (AtomAssigned(inst)) {
      switch (InstanceKind(inst)) {
      case REAL_INST:
      case REAL_ATOM_INST:
      case REAL_CONSTANT_INST:
        FPRINTF(fp,"%s  %.18g\n",labelstring, RealAtomValue(inst));
        break;
      case INTEGER_INST:
      case INTEGER_ATOM_INST:
      case INTEGER_CONSTANT_INST:
        FPRINTF(fp,"%s  %ld\n",labelstring, GetIntegerAtomValue(inst));
        break;
      case SYMBOL_INST:
      case SYMBOL_ATOM_INST:
      case SYMBOL_CONSTANT_INST:
        FPRINTF(fp,"%s  %s\n",labelstring, SCP(GetSymbolAtomValue(inst)));
        break;
      case BOOLEAN_INST:
      case BOOLEAN_ATOM_INST:
      case BOOLEAN_CONSTANT_INST:
        FPRINTF(fp,"%s  %d\n",labelstring,
          (GetBooleanAtomValue(inst)!=0)?1:0);
        break;
      default:
        break;
      }
    }
  }
}

static void write_point(FILE *fp, struct Instance *point, boolean drawline)
{
/* ? draw line from previous point
 * Writes a given point (instance of plt_point) to fp
 * if values of point are well defined.
 */
  unsigned long ndx;
  struct Instance *ix, *iy;
  double x,y;

  ndx = ChildNumberbyChar(point,PLOT_XPOINT);    /* do x */
  ix = InstanceChild(point,ndx);
  ndx = ChildNumberbyChar(point,PLOT_YPOINT);    /* do y */
  iy = InstanceChild(point,ndx);

  if (ix != NULL && iy != NULL &&  AtomAssigned(ix) && AtomAssigned(iy)) {
    x = RealAtomValue(ix);
    y = RealAtomValue(iy);
    FPRINTF(fp,drawline ? "%g %g\n" : "move %g %g\n",x,y);
  }
}

static void write_curve(FILE *fp,
                        struct Instance *curve,
                        unsigned long curve_number)
{
/*
 *  Writes a given curve to the given file with descriptor fp.
 */

  struct Instance *point_array_inst, *a_point;
  unsigned long ndx;
  unsigned long npoints;
  unsigned long c;

  (void)curve_number;  /*  stopping gcc whine about unused parameter  */

  /* do plot legend */
  do_plot_legends(fp,curve,PLOT_LEGEND);

  ndx = ChildNumberbyChar(curve,PLOT_POINT);
  if (ndx) {
    point_array_inst = InstanceChild(curve,ndx);
    npoints = NumberChildren(point_array_inst);
    for (c=1;c<=npoints;c++) {
      a_point = InstanceChild(point_array_inst,c);
      write_point(fp,a_point,1);
    }
  }
}

void plot_prepare_file(struct Instance *plot_inst, char *plotfilename)
{
  struct Instance *curve_array_inst, *a_curve;
  unsigned long ndx;
  unsigned long ncurves;
  unsigned long c;
  enum inst_t kind;
  FILE *fp;

  if (plot_inst==NULL) {
    return;
  }
  fp=fopen(plotfilename,"w");
  if( fp == NULL ) {
    FPRINTF(stderr,"ERROR: plot_prepare_file: Cannot open %s for writing.\n",
            plotfilename);
    return;
  }
  init_gstrings(); /* set up symchars for the child queries */
  /* do the plot labels */
  switch(g_plot_type) {
  case PLAIN_PLOT:
    break;
  case GNU_PLOT:
    FPRINTF(fp,"#Plot data for gnuplot generated by ASCEND\n");
    FPRINTF(fp,"#Remove # marks for legends etc.\n\n");
    do_plot_labels(fp,plot_inst,PLOT_TITLE,  "#set title  ");
    do_plot_labels(fp,plot_inst,PLOT_XLABEL, "#set xlabel ");
    do_plot_labels(fp,plot_inst,PLOT_YLABEL, "#set ylabel ");
    break;
  case XGRAPH_PLOT:
    FPRINTF(fp,"#Plot data for Xgraph generated by ASCEND\n\n");
    do_plot_labels(fp,plot_inst,PLOT_TITLE,  "TitleText: ");
    do_plot_labels(fp,plot_inst,PLOT_XLABEL, "XUnitText: ");
    do_plot_labels(fp,plot_inst,PLOT_YLABEL, "YUnitText: ");
    do_plot_labels(fp,plot_inst,PLOT_YLOG, "LogY: ");
    do_plot_labels(fp,plot_inst,PLOT_XLOG, "LogX: ");
    do_plot_labels(fp,plot_inst,PLOT_YLO, "YLowLimit: ");
    do_plot_labels(fp,plot_inst,PLOT_XLO, "XLowLimit: ");
    do_plot_labels(fp,plot_inst,PLOT_YHI, "YHighLimit: ");
    do_plot_labels(fp,plot_inst,PLOT_XHI, "XHighLimit: ");
    break;
  default:
    break;
  }

  ndx =  ChildNumberbyChar(plot_inst,PLOT_CURVE); /*search for curve_array*/
  if (ndx) {
    curve_array_inst = InstanceChild(plot_inst,ndx);
    kind = InstanceKind(curve_array_inst);
    ncurves = NumberChildren(curve_array_inst);   /*get no. of curves*/
    for (c=1;c<=ncurves;c++) {                    /*get true curve number*/
      if (kind==ARRAY_INT_INST || kind==ARRAY_ENUM_INST) {
        a_curve = InstanceChild(curve_array_inst,c);
        write_curve(fp,a_curve,c);
      }
    }
  }
  fclose(fp);
}

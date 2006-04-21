/*	ASCEND modelling environment
	Copyright (C) 1990 Thomas Guthrie Epperly, Karl Michael Westerberg
	Copyright (C) 1994 Kirk Andre Abbott, Benjamin Andrew Allan
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
	This module defines the plot generation auxillaries for Ascend.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler/instance_enum.h"
*//*
	Version: $Revision: 1.1 $
	Version control file: $RCSfile: plot.h,v $
	Date last modified: $Date: 1997/07/18 11:47:38 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_PLOT_H
#define ASC_PLOT_H

#include <utilities/ascConfig.h>

/** Plot types. */
enum PlotTypes {
  PLAIN_PLOT,
  GNU_PLOT,
  XGRAPH_PLOT
};

ASC_DLLSPEC(enum PlotTypes ) g_plot_type;
/**<
 *  These are the plot types recognized by the code in plot.c
 *  Parameterized equivalents are also recognized.
 *  <pre>
 *  MODEL plt_point;
 *      x, y IS_A real;
 *  END plt_point;
 *
 *  MODEL plt_curve;
 *      npnts             IS_A set of integer_constant;
 *      pnt[npnts]        IS_A plt_point;
 *      legend            IS_A symbol; (* or symbol_constant *)
 *  END plt_curve;
 *
 *  MODEL plt_plot_symbol;
 *     curve_set            IS_A set OF symbol_constant;
 *     curve[curve_set]     IS_A plt_curve;
 *
 *     title, XLabel, YLabel IS_A symbol; (* or symbol_constant *)
 *     Xlow,Xhigh,Ylow,Yhigh IS_A real;
 *     Xlog, Ylog IS_A boolean;
 *  END plt_plot_symbol;
 *
 *  MODEL plt_plot_integer;
 *      curve_set                IS_A set of  integer_constant;
 *      curve[curve_set]      IS_A plt_curve;
 *
 *      title, XLabel, YLabel IS_A symbol; (* or symbol_constant *)
 *      Xlow,Xhigh,Ylow,Yhigh IS_A real;
 *      Xlog, Ylog IS_A boolean;
 *  END plt_plot_integer;
 *  </pre>
 *  @todo Support for all the attributes of a plt_plot_*
 *        is good only for xgraph. gnuplot has been neglected.
 */

ASC_DLLSPEC(void ) plot_prepare_file(struct Instance *inst, char *plotfilename);
/**<
 *  Writes data points for the given plot instance to the given plot file.
 */

ASC_DLLSPEC(boolean) plot_allowed(struct Instance *inst);
/**<
 *  Determines whether or not the given instance is allowed to be plotted
 *  (i.e. whether it is a refinement of plt_plot).
 */

#endif  /* ASCTK_PLOT_H */


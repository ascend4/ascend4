/*
 *  Ascend Instance Tree Type Implementation
 *  by Tom Epperly
 *  9/3/89
 *  Version: $Revision: 1.21 $
 *  Version control file: $RCSfile: instance.c,v $
 *  Date last modified: $Date: 1997/07/18 12:30:05 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
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

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "bit.h"
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "instance_enum.h"
#include "child.h"
#include "type_desc.h"
#include "instance_types.h"
#include "instance_io.h"
#include "instance.h"
#include "instmacro.h"
#include "visitinst.h"
#include "extinst.h"
#include "instquery.h"
#include "linkinst.h"
#include "destroyinst.h"
#include "createinst.h"
#include "arrayinst.h"
#include "refineinst.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "check.h"
#include "dump.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "extfunc.h"
#include "relation_type.h"
#include "relation.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "relation_util.h"
#include "logrel_util.h"
#include "rel_common.h"
#include "case.h"
#include "when_util.h"
#include "universal.h"
#include "tmpnum.h"
#include "cmpfunc.h"
#include "instance_name.h"

#ifndef lint
static CONST char InstanceModuleID[] = "$Id: instance.c,v 1.21 1997/07/18 12:30:05 mthomas Exp $";
#endif

/* lights out for instance.c */
/* file should be removed */

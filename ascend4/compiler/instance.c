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

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/bit.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_enum.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/instance_types.h"
#include "compiler/instance_io.h"
#include "compiler/instance.h"
#include "compiler/instmacro.h"
#include "compiler/visitinst.h"
#include "compiler/extinst.h"
#include "compiler/instquery.h"
#include "compiler/linkinst.h"
#include "compiler/destroyinst.h"
#include "compiler/createinst.h"
#include "compiler/arrayinst.h"
#include "compiler/refineinst.h"
#include "compiler/atomvalue.h"
#include "compiler/atomsize.h"
#include "compiler/check.h"
#include "compiler/dump.h"
#include "compiler/prototype.h"
#include "compiler/pending.h"
#include "compiler/find.h"
#include "compiler/extfunc.h"
#include "compiler/relation_type.h"
#include "compiler/relation.h"
#include "compiler/logical_relation.h"
#include "compiler/logrelation.h"
#include "compiler/relation_util.h"
#include "compiler/logrel_util.h"
#include "compiler/rel_common.h"
#include "compiler/case.h"
#include "compiler/when_util.h"
#include "compiler/universal.h"
#include "compiler/tmpnum.h"
#include "compiler/cmpfunc.h"
#include "compiler/instance_name.h"

#ifndef lint
static CONST char InstanceModuleID[] = "$Id: instance.c,v 1.21 1997/07/18 12:30:05 mthomas Exp $";
#endif

/* lights out for instance.c */

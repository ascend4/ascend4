#                        coll_lib.s
#                        by Robert S. Huss
#                        Part of the Ascend Library
#
#This file is part of the Ascend modeling library.
#
#Copyright (C) 1994
#
#The Ascend modeling library is free software; you can redistribute
#it and/or modify it under the terms of the GNU General Public License as
#published by the Free Software Foundation; either version 2 of the
#License, or (at your option) any later version.
#
#The Ascend Language Interpreter is distributed in hope that it will be
#useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#General Public License for more details.
#
#You should have received a copy of the GNU General Public License along with
#the program; if not, write to the Free Software Foundation, Inc., 675
#Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.

# $Id: coll_lib.s,v 1.5 1996/09/04 19:10:35 ballan Exp $

DELETE TYPES
set libraries $env(ASCENDDIST)/models/libraries
set examples $env(ASCENDDIST)/models/examples
#
READ FILE "$libraries/system.lib";
READ FILE "$libraries/atoms.lib";
READ FILE "$libraries/components.lib";
READ FILE "$libraries/H_G_thermodynamics.lib";
READ FILE "$libraries/plot.lib";
READ FILE "$libraries/stream.lib";
READ FILE "$libraries/flash.lib";
READ FILE "$libraries/collocation.lib";
READ FILE flexible_design/cost_column.asc;

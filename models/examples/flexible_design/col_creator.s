#                        col_creator.s
#                        by Ben Allan
#                        Part of the Ascend Library
#
#This file is part of the Ascend modeling library.
#
#Copyright (C) 1995 Benjamin Andrew Allan
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

# $Id: col_creator.s,v 1.1.1.1 1996/05/20 22:05:44 mthomas Exp $

# Load Bob's Column Creator Interface
set cclib $env(ASCENDDIST)/models/examples/flexible_design;
DELETE TYPES;
source $cclib/main.tcl;

# the following line deletes the pesky
# message center Bob has so that the
# window doesn't flash a lot. There is
# also a button for doing this on his
# Options menu.
killmessagecenter;

# All the action is in the created window
# now. The usual sequence is to work the 
# bottom set of buttons from left to right
# and top to bottom.

# Many of the button names are dimmed/
# invisible until a component is picked.

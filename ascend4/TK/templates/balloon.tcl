##############################################################################
# $Id: balloon.tcl,v 1.1 1999/06/22 00:44:22 ballan Exp $
#
# balloon.tcl - procedures used by balloon help
#
# Copyright (C) 1996-1998 Stewart Allen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

##############################################################################
#

# the 7 following lines allow balloon to be used stand-alone with other apps
global vTcl
set vTcl(balloon,first)  0
set vTcl(balloon,set)    0
set vTcl(balloon,soon)   0
set vTcl(pr,balloon)     1
toplevel .vTcl
wm withdraw .vTcl


bind vTcl(balloon) <Enter> {
    set vTcl(balloon,set) 0
    set vTcl(balloon,first) 1
    set vTcl(balloon,id) [after 300 {vTcl:balloon %W $vTcl(balloon,%W)}]
}

bind vTcl(balloon) <Button> {
    set vTcl(balloon,first) 0
    vTcl:kill_balloon
}

bind vTcl(balloon) <Leave> {
    set vTcl(balloon,first) 0
    vTcl:kill_balloon
}

bind vTcl(balloon) <Motion> {
    if {$vTcl(balloon,set) == 0} {
        after cancel $vTcl(balloon,id)
        set vTcl(balloon,id) [after 300 {vTcl:balloon %W $vTcl(balloon,%W)}]
    }
}

proc vTcl:set_balloon {target message} {
    global vTcl
    set vTcl(balloon,$target) $message
    bindtags $target "[bindtags $target] vTcl(balloon)"
}

proc vTcl:kill_balloon {} {
    global vTcl
    after cancel $vTcl(balloon,id)
    if {[winfo exists .vTcl.balloon] == 1} {
        destroy .vTcl.balloon
    }
    set vTcl(balloon,set) 0
}
# The following function is patched to use the same logic as
# unix menu posting to keep the entire balloon on the screen.
# It has not been tested under windows/mac.
proc vTcl:balloon {target message} {
    global vTcl
    if {$vTcl(balloon,first) == 1 && $vTcl(pr,balloon) == 1} {
        set vTcl(balloon,first) 2

        # get centered, below widget balloon upper left corner
        set x [expr {[winfo rootx $target] + ([winfo width $target]/2)}]
        set y [expr {[winfo rooty $target] + [winfo height $target] + 4}]

        # make balloon
        toplevel .vTcl.balloon -bg black
        wm overrideredirect .vTcl.balloon 1
        label .vTcl.balloon.l \
            -text $message -relief flat \
            -bg #ffffaa -fg black -padx 2 -pady 0 -anchor w
        pack .vTcl.balloon.l -side left -padx 1 -pady 1

        set vw .vTcl.balloon.l
        # adjust y to be on screen, like the menu post code
        incr y [winfo vrooty $vw]
        set tmp [expr {[winfo screenheight $vw] -[winfo reqheight $vw]}]
        if {$y > $tmp} {
            set y $tmp
        }
        if {$y < 0} {
          set y 0
        }

        # adjust x to be on screen, like the menu post code
        incr x [winfo vrootx $vw]
        set tmp [expr {[winfo screenwidth $vw] - [winfo reqwidth $vw]}]
        if {$x > $tmp} {
            set x $tmp
        }
        if {$x < 0} {
          set x 0
        }

        wm geometry .vTcl.balloon +${x}+${y}
        set vTcl(balloon,set) 1
    }
}



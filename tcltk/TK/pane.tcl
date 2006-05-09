#  pane.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.2 $
#  Last modified on: $Date: 1998/06/18 15:55:35 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: pane.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1994-1998 Carnegie Mellon University
#
#  The ASCEND Tcl/Tk Interface is free software; you can redistribute
#  it and/or modify it under the terms of the GNU General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  The ASCEND Tcl/Tk Interface is distributed in hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the
#  file named COPYING.  COPYING is found in ../compiler.

#  Tcl Version:  8.0
#  Tk Version:  8.0
#----------------------------------------------------------------------------

#  This file contains the functions needed to support Panes

#----------------------------------------------------------------------------

#
# proc VPane-Bind  {root left right {ks 10} {kf 0.85}}
#----------------------------------------------------------------------------
# Bind an existing window hierarchy to have a moveable division.
# The hierarchy should be a root frame with two child frames
# splitting the horizontal dimension and a relative height of 1 for both.
# can handle more than one hierarchy simultaneously.
# Forces divider knob to stay in window.
# Brings child frames under control of the placer. leaving any grandchild
# frames in whatever geometry manager they originated.
#    args root - the name of a window under control of the packer or placer
#    left - the leaf name of the left child window of root
#    right - the leaf name of the left child window of root
#    ks - the knob size (points) of the grip square
#    kf - the fraction of the width of the window.
#       - the knob will be placed almost at the bottom of the frame (0.90)
#    $root.f is the knob
# eg. VPane-Bind .browser.main_frm parents_box child_box 15 0.85
# suggested by Kirk Abbott(inspired by the net) and generalized by Ben Allan
# associated procs are named starting with VPane. HPane equivalents are easy.
# configurable VPaned(knobcolor)
#
# kf is the fraction of the total horizontal width where the window should
# be split. For example .30 means a smaller left window.
# ks is the size of the knob.
#----------------------------------------------------------------------------
global VPaned
set VPaned(knobcolor) white
proc VPane-Bind {root left right {ks 10} {kf 0.85} } {
  global VPaned
 # make sure anything to be packed has been
  update idletasks
  set VPaned($root,left) $root.$left
  set VPaned($root,right) $root.$right
  set VPaned($root,knobsize) $ks
  set VPaned(xdrag_start) [winfo width $root.$left]
  set VPaned(ydrag_start) [expr $kf * [winfo height $root.$left]]
  set VPaned(xfraction) $kf
  if {$VPaned(xfraction) >0.9} {set VPaned(xfraction) 0.33}
  set VPaned(newxfrac) $VPaned(xfraction)
  set VPaned(yfraction) 0.9
  set VPaned(newyfrac) 0.9
 # for the square i.e. the little button.
  frame $root.f \
    -width $VPaned($root,knobsize) \
    -height $VPaned($root,knobsize) \
    -borderwidth 2 \
    -relief raised \
    -cursor crosshair \
    -background $VPaned(knobcolor)
  place $root.f \
    -in $root \
    -relx $VPaned(xfraction) \
    -rely $VPaned(yfraction) \
    -anchor n

 # Now for the vertical separator.
  frame $root.sep \
    -width 4 \
    -height 1 \
    -borderwidth 2 \
    -relief ridge
  place $root.sep \
    -in $root \
    -relx $VPaned(xfraction) \
    -relheight 1 \
    -rely 0 \
    -anchor n
  raise $root.sep
  raise $root.f

 # set up the bindings for the little square
  bind $root.f <Button-1> {VPane-start-grip %x %y %W}
  bind $root.f <B1-Motion> {VPane-handle-grip %x %y %W}
  bind $root.f <B1-ButtonRelease-1> {VPane-end-grip %W}

 # tell the packer to quit managing left and right after establishing placer
 # this won't hurt even if packer never knew about them
  pack forget $root.$left
  pack forget $root.$right
 #remap after packer unmaps
  VPane-end-grip $root.f
  update idletasks
}

#
# proc HPane-Bind  {root top bot {ks 10} {kf 0.85} {uminsize 0}
#----------------------------------------------------------------------------
# Bind an existing window hierarchy to have a moveable division.
# The hierarchy should be a root frame with two child frames
# splitting the horizontal dimension and a relative height of 1 for both.
# can handle more than one hierarchy simultaneously.
# Forces divider knob to stay in window.
# Brings child frames under control of the placer. leaving any grandchild
# frames in whatever geometry manager they originated.
# args root - the name of a window under control of the packer or placer
# top - the leaf name of the top child window of root
# bot - the leaf name of the bottom child window of root
# ks - the knob size (points) of the grip square
# kf - the fraction of vertical distance to start the knob at, 0.0=left
# uminsize - the minimum pixel height of one of the sub windows,
#            if 0, no min enforced,
#            if >0, min is min height of top window
#            if <0, min is -min height of bottom window
#   minimum can only be enforce on 1 pane. Enforced in HPane-replace
# The reason for the minsize feature is the textbox bug in tk3.6 which
# dumps core in tkTextDisp.c.
# $root.f is the knob
# eg. HPane-Bind .browser.main_frm parents_box child_box 15 0.85
# suggested by Kirk Abbott(inspired by the net) and generalized by Ben Allan
# associated procs are named starting with HPane. VPane equivalents are easy.
# configurable HPaned(knobcolor).
#
# kf is the fraction of the total vertical height where the window should
# be split. For example .85 means a big top window.
# ks is the size of the knob.
#----------------------------------------------------------------------------
global HPaned
set HPaned(knobcolor) white
proc HPane-Bind {root top bot {ks 10} {kf 0.85} {uminsize 0}} {
  global HPaned
 # make sure anything to be packed has been
  update idletasks
  set HPaned($root,top) $root.$top
  set HPaned($root,bot) $root.$bot
  set HPaned($root,knobsize) $ks
  set HPaned($root,uminsize) $uminsize
  set HPaned(ydrag_start) [winfo height $root.$top]
  set HPaned(xdrag_start) [expr $kf * [winfo width $root.$top]]
  set HPaned(yfraction) $kf
  if {$HPaned(yfraction) >0.9} {set HPaned(yfraction) 0.33}
  set HPaned(newyfrac) $HPaned(yfraction)
  set HPaned(xfraction) 0.9
  set HPaned(newxfrac) 0.9
 # for the square i.e. the little button.
  frame $root.f \
    -width $HPaned($root,knobsize) \
    -height $HPaned($root,knobsize) \
    -borderwidth 2 \
    -relief raised \
    -cursor crosshair \
    -background $HPaned(knobcolor)
  place $root.f \
    -in $root \
    -rely $HPaned(yfraction) \
    -relx $HPaned(xfraction) \
    -anchor w

 # Now for the horizontal separator.
  frame $root.sep \
    -width 1 \
    -height 4 \
    -borderwidth 2 \
    -relief ridge
  place $root.sep \
    -in $root \
    -rely $HPaned(yfraction) \
    -relwidth 1 \
    -relx 0 \
    -anchor w
  raise $root.sep
  raise $root.f

 # set up the bindings for the little square
  bind $root.f <Button-1> {HPane-start-grip %x %y %W}
  bind $root.f <B1-Motion> {HPane-handle-grip %x %y %W}
  bind $root.f <B1-ButtonRelease-1> {HPane-end-grip %W}

 # tell the packer to quit managing top and bot after establishing placer
 # this won't hurt even if packer never knew about them
  pack forget $root.$top
  pack forget $root.$bot
 #remap after packer unmaps
  HPane-end-grip $root.f
  update idletasks
}

#
# proc VPane-start-grip {wherex wherey w}
#----------------------------------------------------------------------------
# binding to set starting point for relative repositioning calcs when moving.
# start observing x events on knob
#----------------------------------------------------------------------------
proc VPane-start-grip {wherex wherey w} {
  update idletasks
  global VPaned
  set VPaned(active) 1
  set w [winfo parent $w]
  set VPaned(xdrag_start) [winfo width $VPaned($w,left)]
  set VPaned(ydrag_start) [winfo y $w.f]
  grab $w.f
  raise $w.sep
  raise $w.f
  $w.f configure -relief sunken
  set VPaned(xfraction) [expr $VPaned(xdrag_start) / [winfo width $w].0]
  set VPaned(newxfrac) $VPaned(xfraction)
  set VPaned(yfraction) [expr $VPaned(ydrag_start) / [winfo height $w].0]
  set VPaned(newyfrac) $VPaned(yfraction)
  update idletasks
}
#
# proc HPane-start-grip {wherex wherey w}
#----------------------------------------------------------------------------
# binding to set starting point for relative repositioning calcs when moving.
# start observing x events on knob
#----------------------------------------------------------------------------
proc HPane-start-grip {wherex wherey w} {
  update idletasks
  global HPaned
  set HPaned(active) 1
  set w [winfo parent $w]
  set HPaned(ydrag_start) [winfo height $HPaned($w,top)]
  set HPaned(xdrag_start) [winfo x $w.f]
  grab $w.f
  raise $w.sep
  raise $w.f
  $w.f configure -relief sunken
  set HPaned(yfraction) [expr $HPaned(ydrag_start) / [winfo height $w].0]
  set HPaned(newyfrac) $HPaned(yfraction)
  set HPaned(xfraction) [expr $HPaned(xdrag_start) / [winfo width $w].0]
  set HPaned(newxfrac) $HPaned(xfraction)
  update idletasks
}


#
# proc VPane-handle-grip {wherex wherey w}
#----------------------------------------------------------------------------
# binding for relative repositioning when moving
#----------------------------------------------------------------------------
proc VPane-handle-grip {wherex wherey w} {
  global VPaned

  set x [expr $wherex +  [winfo x $w]]
  set y [expr $wherey +  [winfo y $w]]
  set w [winfo parent $w]
  if $VPaned(active) { #  tk event management not so robust
    set vpxf \
      [expr "(($x.0 - $VPaned(xdrag_start)) / [winfo width $w].0) \
       + $VPaned(xfraction)"]
    if {$vpxf < 0.01} {set vpxf 0.01}
    if {$vpxf > 1.0} {set vpxf 0.99}
    set vpyf \
      [expr "(($y.0 - $VPaned(ydrag_start)) / [winfo height $w].0) \
       + $VPaned(yfraction)"]
    if {$vpyf < 0.02} {set vpyf 0.02}
    if {$vpyf > 0.95} {set vpyf 0.95}
  } else {
    set vpxf $VPaned(xfraction)
    set vpyf $VPaned(yfraction)
  }
  place $w.sep \
    -in $w \
    -relx $vpxf \
    -relheight 1 \
    -rely 0 \
    -anchor n
  place $w.f \
    -in $w \
    -relx $vpxf \
    -rely $vpyf \
    -anchor n
  set VPaned(newxfrac) $vpxf
  set VPaned(newyfrac) $vpyf
}
#
# proc HPane-handle-grip {wherex wherey w}
#----------------------------------------------------------------------------
# binding for relative repositioning when moving
#----------------------------------------------------------------------------
proc HPane-handle-grip {wherex wherey w} {
  global HPaned

  set x [expr $wherex +  [winfo x $w]]
  set y [expr $wherey +  [winfo y $w]]
  set w [winfo parent $w]
  if $HPaned(active) { #  tk event management not so robust
    set hpyf \
      [expr "(($y.0 - $HPaned(ydrag_start)) / [winfo height $w].0) \
       + $HPaned(yfraction)"]
    if {$hpyf < 0.01} {set hpyf 0.01}
    if {$hpyf > 1.0} {set hpyf 0.99}
    set hpxf \
      [expr "(($x.0 - $HPaned(xdrag_start)) / [winfo width $w].0) \
       + $HPaned(xfraction)"]
    if {$hpxf < 0.02} {set hpxf 0.02}
    if {$hpxf > 0.99} {set hpxf 0.99}
  } else {
    set hpxf $HPaned(xfraction)
    set hpyf $HPaned(yfraction)
  }
  place $w.sep \
    -in $w \
    -rely $hpyf \
    -relwidth 1 \
    -relx 0 \
    -anchor w
  place $w.f \
    -in $w \
    -rely $hpyf \
    -relx $hpxf \
    -anchor w
  set HPaned(newxfrac) $hpxf
  set HPaned(newyfrac) $hpyf
}


#
# proc VPane-replace {w}
#----------------------------------------------------------------------------
# redraw root window when split is decided on by user. record new start
#----------------------------------------------------------------------------
proc VPane-replace {w} {
  global VPaned

  set vpxf $VPaned(xfraction)
  set vpyf $VPaned(yfraction)
  place $w.f \
    -in $w \
    -rely $vpyf \
    -relx $vpxf \
    -anchor n
  place $VPaned($w,left) \
    -in $w \
    -x 0 \
    -y 0 \
    -relheight 1 \
    -relwidth $vpxf
  place $VPaned($w,right) \
    -in $w \
    -y 0 \
    -relx $vpxf \
    -relheight 1 \
    -relwidth [expr "1.0 - $vpxf"]
  lower $w.sep
  raise $w.f
  update idletasks
}
#
# proc HPane-replace {w}
#----------------------------------------------------------------------------
# redraw root window when split is decided on by user. record new start.
# if minsize enforced, resets yfraction.
#----------------------------------------------------------------------------
proc HPane-replace {w} {
  global HPaned

  set ums $HPaned($w,uminsize)
  set hpxf $HPaned(xfraction)
  set hpyf $HPaned(yfraction)
  set wdim \
    [expr [winfo height $HPaned($w,top)] + [winfo height $HPaned($w,bot)] ]
  if {$ums != 0} {
    set minfrac [expr (abs($ums)*1.0)/($wdim*1.0)]
    if {$ums > 0} {
      if {$minfrac > $hpyf} {
        set hpyf $minfrac
      }
      if {$hpyf > 1.0} {
        set hpyf 0.99
      }
    } else {
      if {(1 - $minfrac) < $hpyf} {
        set hpyf [expr 1.0 - $minfrac]
      }
      if {$hpyf < 0.01} {
        set hpyf 0.01
      }
    }
  }
  set $HPaned(yfraction) $hpyf
  place $w.f \
    -in $w \
    -rely $hpyf \
    -relx $hpxf \
    -anchor w
  place $w.sep \
    -in $w \
    -rely $hpyf \
    -relwidth 1 \
    -relx 0 \
    -anchor w
  place $HPaned($w,top) \
    -in $w \
    -x 0 \
    -y 0 \
    -relheight $hpyf \
    -relwidth 1
  place $HPaned($w,bot) \
    -in $w \
    -x 0 \
    -rely $hpyf \
    -relwidth 1 \
    -relheight [expr "1.0 - $hpyf"]
  raise $w.sep
  raise $w.f
  update idletasks
}


#
# proc VPane-end-grip {w}
#----------------------------------------------------------------------------
# fix split decided on by user and ignore any linger x events on knob.
#----------------------------------------------------------------------------
proc VPane-end-grip {w} {
  global VPaned
  set w [winfo parent $w]
  set VPaned(xfraction) $VPaned(newxfrac)
  set VPaned(yfraction) $VPaned(newyfrac)
  set VPaned(active) 0
  $w.f configure \
    -relief raised \
    -height $VPaned($w,knobsize) \
    -width $VPaned($w,knobsize)
  VPane-replace $w
  grab release $w.f
}
#
# proc HPane-end-grip {w}
#----------------------------------------------------------------------------
# fix split decided on by user and ignore any linger x events on knob.
# here we need to enforce the min pixel size if specified for part of the
# window.
#----------------------------------------------------------------------------
proc HPane-end-grip {w} {
  global HPaned
  set w [winfo parent $w]
  set HPaned(xfraction) $HPaned(newxfrac)
  set HPaned(yfraction) $HPaned(newyfrac)
  set HPaned(active) 0
  $w.f configure \
    -relief raised \
    -height $HPaned($w,knobsize) \
    -width $HPaned($w,knobsize)
  HPane-replace $w
  grab release $w.f
}


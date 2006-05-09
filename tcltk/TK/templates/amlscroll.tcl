#
# Labelled multilist scrollbox creator.
# By Benjamin Andrew Allan
# May 3, 1998
# Copyright 1998 Carnegie Mellon University
# 
# ascLabeledMultiList parent label width
# Adds a listbox to the parent frame with a label at the top of the
# list box. If this is the first added listbox, the parent also gets
# a scrollbar for controlling the box. Width specifies the initial
# width of the added list widget.
# On subsequent calls with the same parent, the listbox is added to
# the right of the already defined listboxes.
# Calls with distinct parents are handled independently.
# Within one parent all listboxes are scrolled identically.
#
# Returns the name of the added listbox.

global ascMLVect
catch {unset ascMLVect}

proc ascMLset {lbox args} {
  global ascMLVect
  set parent $ascMLVect($lbox)
  foreach lb $ascMLVect($parent.frame) { 
    eval $lb.box $args 
  }
} 

proc ascMLscroll {scrollbar lbox args} {
  global ascMLVect
  set parent $ascMLVect($lbox) 
  eval $scrollbar set $args
  ascMLset $lbox yview moveto [lindex $args 0]
}  

proc ascLabeledMultiList {parent label width} {
  global ascMLVect
  if {![info exists ascMLVect($parent)]} {
    set ascMLVect($parent) 0
    set ascMLVect($parent.sy) $parent
    scrollbar $parent.sy -orient v -command [list ascMLset $parent.sy yview]
    pack $parent.sy -fill y -side right 
  }
  set n $parent.list$ascMLVect($parent)
  incr ascMLVect($parent)
  lappend ascMLVect($parent.frame) $n 
  
  frame $n

  # build widget $n.box
  listbox $n.box \
    -exportsel no \
    -width $width \
    -selectmode browse \
    -yscrollcommand "ascMLscroll $parent.sy $n.box" \
    -height 3
 
  set ascMLVect($n.box) $parent

  foreach i {1 2 3 4 5 6 7 8} {
    $n.box insert end $i
  }

  bind $n.box <ButtonPress-1> {
    ascMLset %W select clear 0 end
    ascMLset %W select set [%W nearest %y]
  }     

  bind $n.box <B1-Motion> {
    ascMLset %W select clear 0 end
    ascMLset %W select set [%W nearest %y]
    ascMLset %W see [%W nearest %y]
  }

  bind $n.box <ButtonRelease-1> {
    ascMLset %W select clear 0 end
    ascMLset %W select set [%W nearest %y]
  }     

  # build widget $n.label
  label $n.label \
    -borderwidth {1} \
    -text $label

  pack configure $n.label \
    -fill x
  pack configure $n.box \
    -expand 1 \
    -fill both
  pack configure $n \
    -expand 1 \
    -side left \
    -fill both

  return $n.box
}
set test 0
if {$test} {
catch {destroy .t}
catch {destroy .u}
toplevel .t
ascLabeledMultiList .t c1
ascLabeledMultiList .t c2
ascLabeledMultiList .t c3
ascLabeledMultiList .t c4
ascLabeledMultiList .t c5
toplevel .u
ascLabeledMultiList .u c1
ascLabeledMultiList .u c2
ascLabeledMultiList .u c3
ascLabeledMultiList .u c4
ascLabeledMultiList .u c5
}

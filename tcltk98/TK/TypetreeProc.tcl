#  TypetreeProc.tcl: a type hierarchy browser
#  by Benjamin A. Allan
#  October 12, 1994
#  Part of ASCEND
#  Revision: $Revision: 1.14 $
#  Last modified on: $Date: 1998/06/18 15:55:04 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: TypetreeProc.tcl,v $
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

# global arrays _ATTV since noone should touch it and it is unset often
#               _attv since noone should touch it
#               ascTypetreeVect which may be of some interest
# i goes from 0
# _ATTV($l) is the number of types at level l
# _ATTV($l,name.$i) is the name of the ith type at level l
# _ATTV($l,nump.$i) is the number of the parent type at level l-1
#                   of type i on level l
# _ATTV($name) is the coordinates of the box for type $name
# _ATTV lmax is the maximum depth
# _ATTV wmax is the maximum width of all levels.
# _ATTV($l.maxname) is the length of the longest name on a given level l
# _ATTV(horzwidth) is the character with of the region plotted horizontally
# _attv(back.$i) the backtrack info for trees
# _attv(backlength) the depth of backtrack history.

# uses drefinement_tree
# predicated on a recursive list of refinements from a given type
# assumes no multiple inheritance is allowed.
#
# proc Type_Handle_Sourceread {args}
#------------------------------------------------------------------------
# eliminate the backtrace of hierarchies and wipe the canvas
#------------------------------------------------------------------------
proc Type_Handle_Sourceread {args} {
  Type_do_OK
}
#
# proc Type_getroot {typename}
#------------------------------------------------------------------------
# get the name of the root type of a given type.
#------------------------------------------------------------------------
proc Type_getroot {typename} {
  if {$typename==""} {return ""}
  if {![libr_query -exists -type $typename] } {return ""}
  set list "[hier $typename]"
  if {$list ==""} {return $typename}
  set c [llength $list]
  set root [lindex $list [expr $c - 1]]
  return $root
}
#
# proc Type_enable_backtrack {past}
#------------------------------------------------------------------------
#bind the backtrack button to the most recent previous call. if none,
#disable the button.
#------------------------------------------------------------------------
proc Type_enable_backtrack {past} {
  global _attv ascTypeTreeVect
  set ascTypeTreeVect(previoustree) $past
  if {$past < 0} {
    $ascTypeTreeVect(backbutton) configure -state disabled
  } else {
    $ascTypeTreeVect(backbutton) configure -state normal \
  }
}
#
# proc Type_do_Ancestry {typename xleft ytop}
#------------------------------------------------------------------------
# plot a type hierarchy for the type given where upleft is x y corner to
# start at.
#------------------------------------------------------------------------
proc Type_do_Ancestry {typename xleft ytop} {
  global _ATTV _attv
  set roottype "[Type_getroot "$typename"]"
  if {$roottype == ""} {return}
  Type_clearcan
  if {[info exists _ATTV]} {unset _ATTV}
  set _ATTV(lmax) 0
  set _ATTV(wmax) 0
  set _ATTV(maxchar) 0
  set _ATTV(horzwidth) 0
  set newback "[drefinement_tree $roottype]"
  Type_ModelRefs "$newback" 0 -1
  if {[info exists _attv(backlength)]} {
    Type_enable_backtrack [expr $_attv(backlength) -1]
    set _attv(back.$_attv(backlength)) $newback
    incr _attv(backlength)
  } else {
    set _attv(backlength) 1
    set _attv(back.0) $newback
  }
  for {set i 0} {$i <= $_ATTV(lmax)} {incr i} {
    incr _ATTV(horzwidth) $_ATTV($i.maxname)
    if {$_ATTV(wmax) < $_ATTV($i)} {
      set _ATTV(wmax) $_ATTV($i)
    }
    if {$_ATTV(maxchar) < $_ATTV($i.width)} {
      set _ATTV(maxchar) $_ATTV($i.width)
    }
  }
  if {$_ATTV(wmax) <6} {
    Type_PlotdagVert "$xleft $ytop"
  } else {
    Type_PlotdagHorz "$xleft $ytop"
  }
  $_attv(canvas) config -scrollregion [$_attv(canvas) bbox all]
  Type_boxtext $typename
}

#
# proc Type_Redo_Ancestry {number}
#------------------------------------------------------------------------
# plot a type hierarchy for the type given where upleft is x y corner to
# start at.
#------------------------------------------------------------------------
proc Type_Redo_Ancestry {number} {
  global _ATTV _attv ascTypeTreeVect
  set newback "$_attv(back.$number)"
  set _attv(backlength) [expr $number+1]
  Type_enable_backtrack [expr $number -1]
  Type_clearcan
  if {[info exists _ATTV]} {unset _ATTV}
  set _ATTV(lmax) 0
  set _ATTV(wmax) 0
  set _ATTV(maxchar) 0
  set _ATTV(horzwidth) 0
  Type_ModelRefs "$newback" 0 -1
  for {set i 0} {$i <= $_ATTV(lmax)} {incr i} {
    incr _ATTV(horzwidth) $_ATTV($i.maxname)
    if {$_ATTV(wmax) < $_ATTV($i)} {
      set _ATTV(wmax) $_ATTV($i)
    }
    if {$_ATTV(maxchar) < $_ATTV($i.width)} {
      set _ATTV(maxchar) $_ATTV($i.width)
    }
  }
  if {$_ATTV(wmax) <4} {
    Type_PlotdagVert "0 0"
  } else {
    Type_PlotdagHorz "0 0"
  }
  $_attv(canvas) config -scrollregion [$_attv(canvas) bbox all]
  Type_boxtext ""
}

#
# proc Type_PlotdagVert {corner}
#------------------------------------------------------------------------
# plat the dag contained in _ATTV starting at corner coordinate x y
# plot ancestry top to bottom
#------------------------------------------------------------------------
proc Type_PlotdagVert {corner} {
  global _ATTV _attv
  if {[llength $corner]!=2} { error "Type_Plotdag called with bad corner"}
  set xoff [lindex $corner 0]
  set yoff [lindex $corner 1]
  if {$_ATTV(maxchar)==0} {return; # nothing to plot}
  # get fontsize estimate
  Type_setlettersize
  # calc region width
  set rw [expr $_ATTV(maxchar) * $_attv(fontx)]
  # calc row height
  set yh [expr 4* $_attv(fonty)]
  # calc region height
  set rh [expr ( $_ATTV(lmax) +1 ) * $yh]
  set ystart [expr $yoff + $_attv(fonty)]
  for {set yi 0} {$yi <= $_ATTV(lmax)} {incr yi} { # for all rows
    set yloc [expr $ystart + $yi*$yh]
    if {$_ATTV($yi) < $_ATTV(wmax) && $_ATTV($yi.width) < $_ATTV(maxchar)} {
      # calc x spacing
      set xw [expr $rw/(1+$_ATTV($yi))]
      set xstart [expr $xoff + $xw/2]
      for {set xi 0} {$xi < $_ATTV($yi)} {incr xi} { # for types in row yi
        set xloc [expr $xstart + $xi*$xw]
        set child $_ATTV($yi,name.$xi)
        Type_addtext $child $xloc $yloc
        if {$yi !=0} {
          set parent $_ATTV([expr $yi -1],name.$_ATTV($yi,nump.$xi))
          Type_addarrowvert $child $parent
        }
      }
    } else {
      set x0 [expr 0.5*[string length $_ATTV($yi,name.0)] * $_attv(fontx)]
      set child $_ATTV($yi,name.0)
      Type_addtext $child $x0 $yloc
      if {$yi !=0} {
        set parent $_ATTV([expr $yi -1],name.$_ATTV($yi,nump.0))
        Type_addarrowvert $child $parent
      }
      for {set xi 1} {$xi < $_ATTV($yi)} {incr xi} { # for types in row yi
        set child $_ATTV($yi,name.$xi)
        set leftchild $_ATTV($yi,name.[expr $xi-1])
        set left [lindex $_attv(bb.$leftchild) 2]
        set xloc \
          [expr $_attv(fontx)*(1 + 0.5* [string length $child]) + $left]
        Type_addtext $child $xloc $yloc
        if {$yi !=0} {
          set parent $_ATTV([expr $yi -1],name.$_ATTV($yi,nump.$xi))
          Type_addarrowvert $child $parent
        }
      }
    }
  }
}

#
# Type_getxcenter {left name}
#------------------------------------------------------------------------
# return the xvalue to plot a name at so that the left edge of the
# name is at coordinate left
#------------------------------------------------------------------------
proc Type_getxcenter {left name} {
  global _attv
  set _attv(scratch) [$_attv(canvas) create text 0 0]
  $_attv(canvas) itemconfigure $_attv(scratch) \
    -justify left \
    -text $name
  set _attv(scratchbb) [$_attv(canvas) bbox $_attv(scratch)]
  $_attv(canvas) delete $_attv(scratch)
  return [expr $left - [lindex $_attv(scratchbb) 0]]
}
#
# proc Type_PlotdagHorz {corner}
#------------------------------------------------------------------------
# plat the dag contained in _ATTV starting at corner coordinate x y
# plot ancestry left to right
#------------------------------------------------------------------------
proc Type_PlotdagHorz {corner} {
  global _ATTV _attv
  if {[llength $corner]!=2} { error "Type_Plotdag called with bad corner"}
  set xoff [lindex $corner 0]
  set yoff [lindex $corner 1]
  if {$_ATTV(maxchar)==0} {return; # nothing to plot}
  # get fontsize estimate
  Type_setlettersize
  # calc region width
  set rw [expr ($_ATTV(horzwidth) + 4*$_ATTV(lmax)) * $_attv(fontx)]
  # calc row height
  set yh [expr 1.1* $_attv(fonty)]
  # calc region height
  set rh [expr ( $_ATTV(wmax) +1 ) * $yh]
  set xstart $xoff
  set nextstart $xoff
  #children are by child xi on level yi, regardless of graph orientation
  # coordinates are otherwise xsub is a horz and ysub a vert coord
  for {set yi 0} {$yi <= $_ATTV(lmax)} {incr yi} { # for all levels yi
    if {$yi==0} {
      set xstart $xoff
    } else {
      set xstart [expr $nextstart + 3 *$_attv(fontx)]
    }
    # calc y spacing
    set yw [expr $rh/(1+$_ATTV($yi))]
    set ystart [expr $yoff + $_attv(fonty)]
    for {set xi 0} {$xi < $_ATTV($yi)} {incr xi} { # for types on level yi
      set yloc [expr $ystart + $xi*$yw]
      set child $_ATTV($yi,name.$xi)
      set xloc [Type_getxcenter $xstart $child]
      Type_addtext $child $xloc $yloc
      if {$yi !=0} {
        set parent $_ATTV([expr $yi -1],name.$_ATTV($yi,nump.$xi))
        Type_addarrowhorz $child $parent
      }
      set cright [lindex $_attv(bb.$child) 2]
      if {$cright > $nextstart} {set nextstart $cright}
    }
  }
}


#
# proc Type_ModelRefs {tlist lvl parent}
#------------------------------------------------------------------------
# prepare _ATTV with hierarchy data given by tlist.
#------------------------------------------------------------------------
proc Type_ModelRefs {tlist lvl parent} {
  global _ATTV
  if {[llength $tlist] !=0 } {
    if {$_ATTV(lmax) < $lvl} {set _ATTV(lmax) $lvl}
    foreach child $tlist {
      if {![info exists _ATTV($lvl)]} {
        set _ATTV($lvl) 0
        set _ATTV($lvl.width) 0
        set _ATTV($lvl.maxname) 0
      }
      set name [lindex $child 0]
      set sl [string length $name]
      if {$_ATTV($lvl.maxname) < $sl} {set _ATTV($lvl.maxname) $sl}
      set _ATTV($lvl,name.$_ATTV($lvl)) $name
      incr _ATTV($lvl.width) [expr 2 + [string length $name]]
      set _ATTV($lvl,nump.$_ATTV($lvl)) $parent
      Type_ModelRefs [lindex $child 1] [expr $lvl + 1] $_ATTV($lvl)
      incr _ATTV($lvl)
    }
  }
}
#
# proc Type_dis {children lvl}
#------------------------------------------------------------------------
# this gives a quick type list with row indices
# children is of the drefinement_tree variety.
#------------------------------------------------------------------------
proc Type_dis {children lvl} {
  if {[llength $children] != 0} {
    foreach child $children {
      puts "name: [lindex $child 0] level: $lvl"
      set nextlist [lindex $child 1]
      dis $nextlist [expr $lvl +1]
    }
  }
}

#
# proc Type_do_Atoms {}
#------------------------------------------------------------------------
# display the type definition differential for a type given and
# set Up a list of the types referenced by IRT, ISA IN the
# complete definition. Type list excludes MODEL types.
#------------------------------------------------------------------------
proc Type_do_Atoms {} {
  global ascTypeTreeVect
  if {$ascTypeTreeVect(curtype)==""} {return}
  set err [Type_show_diffcode "$ascTypeTreeVect(curtype)"]
  if {$err} {return}
  set list ""
  set list "[dgetparts ATOMS $ascTypeTreeVect(curtype)]"
  global ascListSelectBox ascLibrVect
  set ascListSelectBox(grab) 0
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 1
  set ascListSelectBox(btn4destroy) 0
  set ascListSelectBox(btn4command) ""
  set ascListSelectBox(title) "Atom parts"
  set ascListSelectBox(toplevelname) ".typemodels"
  set ascListSelectBox(font) $ascLibrVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "ATOM types in $ascTypeTreeVect(curtype):"
  set alist [lsort $list]
  set button [AscListSelectBox $alist \
                250x240[setpos .typetree 50 20]]
  if {$button==2} {return}
  Type_do_Ancestry "$ascListSelectBox(itemselected)" 0 0
}

#
# proc Type_do_Help {} {
#------------------------------------------------------------------------
# display help on this widget
#------------------------------------------------------------------------
proc Type_do_Help {} {
  Help_button library.typetree
}

#
# proc Type_do_Code {} {
#------------------------------------------------------------------------
# display the code for a type a la ddefine
#------------------------------------------------------------------------
proc Type_do_Code {} {
  global ascUtilVect ascTypeTreeVect ascDispVect

  set jnk ""
  set type "$ascTypeTreeVect(curtype)"
  if {$type==""} {return}
  set outputfile [FileUniqueName $ascUtilVect(asctmp)/ascdiscode]
  set result [catch {ddefine $type $outputfile} jnk]
  if {$result == "0"} {
    FastFileInText $ascDispVect(textBox) $outputfile
  }
  if {[file exists $outputfile]} {
    file delete $outputfile
  }
  DispSetEntry "Internal code for $type"
  newraise .display
}

#
# proc Type_do_Back {} {
#------------------------------------------------------------------------
# display all the root types in the system for selection.
#------------------------------------------------------------------------
proc Type_do_Back {} {
  global ascTypeTreeVect
  Type_Redo_Ancestry $ascTypeTreeVect(previoustree)
}

#
# proc Type_do_Print {} {
#------------------------------------------------------------------------
# print the type window
#------------------------------------------------------------------------
proc Type_do_Print {} {
  global ascTypeTreeVect
  Print_configure $ascTypeTreeVect(windowname) Printer
  if {[Print_cancelcheck]} {
    return
  }
  DispPrint [DispWriteSelection $ascTypeTreeVect(canvas)]
  HUB_Message_to_HUB WINDOWPRINTED TYPETREE
}
#
# proc Type_do_OK {} {
#------------------------------------------------------------------------
# close the type window
#------------------------------------------------------------------------
proc Type_do_OK {} {
  global ascTypeTreeVect _attv _ATTV
  if {![winfo exists $ascTypeTreeVect(windowname)]} {return}
  Type_clearcan
  unset _attv
  DestroyWindow.typetree
}

#
# proc Type_do_Roots {} {
#------------------------------------------------------------------------
# display all the root types in the system for selection.
#------------------------------------------------------------------------
proc Type_do_Roots {} {
  global ascListSelectBox ascLibrVect
  set list ""
  catch {set list [libr_query -roottypes]}
  set ascListSelectBox(grab) 0
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 1
  set ascListSelectBox(btn4destroy) 0
  set ascListSelectBox(btn4command) ""
  set ascListSelectBox(title) "Root types"
  set ascListSelectBox(toplevelname) ".typeroots"
  set ascListSelectBox(font) $ascLibrVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Hierarchy roots:"
  set alist [lsort $list]
  set button [AscListSelectBox $alist \
                250x240[setpos .typetree 50 20]]
  if {$button==2} {return}
  Type_do_Ancestry "$ascListSelectBox(itemselected)" 0 0
}

#
# proc Type_show_diffcode {type}
#------------------------------------------------------------------------
# display the code that is new IN this type compared to the type it
# refines. return 1 if bad request, 0 if ok.
#------------------------------------------------------------------------
proc Type_show_diffcode {type} {
  global ascUtilVect ascDispVect
  if {![libr_query -exists -type $type] } {return "1"}
  set list "[hier $type]"
 # if {$list==""} {return 0}
  set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascdiscode"]
  set result [catch {ddiffdefine $type $outputfile} jnk]
  if {$result == "0"} {
    FastFileInText $ascDispVect(textBox) $outputfile
  }
  if {[file exists $outputfile]} {
    file delete $outputfile
  }
  DispSetEntry "Incremental code for $type"
  newraise .display
  return 0
}

#
# proc Type_do_Parts {} {
#------------------------------------------------------------------------
# display the type definition differential for a type given and
# set Up a list of the types referenced by IRT, ATS, ISA IN the
# complete definition. Type list excludes ATOM types.
#------------------------------------------------------------------------
proc Type_do_Parts {} {
  global ascTypeTreeVect
  if {$ascTypeTreeVect(curtype)==""} {return}
  set err [Type_show_diffcode "$ascTypeTreeVect(curtype)"]
  if {$err} {return}
  set list ""
  set list "[dgetparts MODELS $ascTypeTreeVect(curtype)]"
  global ascListSelectBox ascLibrVect
  set ascListSelectBox(grab) 0
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 1
  set ascListSelectBox(btn4destroy) 0
  set ascListSelectBox(btn4command) ""
  set ascListSelectBox(title) "Model parts"
  set ascListSelectBox(toplevelname) ".typemodels"
  set ascListSelectBox(font) $ascLibrVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "MODEL types in $ascTypeTreeVect(curtype):"
  set alist [lsort $list]
  set button [AscListSelectBox $alist \
                250x240[setpos .typetree 50 20]]
  if {$button==2} {return}
  Type_do_Ancestry "$ascListSelectBox(itemselected)" 0 0
}



#
# proc Type_OpenTree {args}
#------------------------------------------------------------------------
# startup the typetree window. if args, first element assumed to be type
#------------------------------------------------------------------------
proc Type_OpenTree {args} {
  global ascTypeTreeVect _attv
  set _attv(canvas) $ascTypeTreeVect(canvas)
  if {![winfo exists $ascTypeTreeVect(windowname)]} {
    set _attv(backlength) 0
    set ascTypeTreeVect(geometry) [osgpos 400x300[setpos .library 40 40]]
    VShowWindow.typetree
    Type_enable_backtrack -1
    set _attv(backlength) 0
  } else {
    newraise $ascTypeTreeVect(windowname)
  }
  if {$args!=""} {
    set type [lindex $args 0]
    Type_do_Ancestry $type 0 0
  }
}
#
# proc set_Typetree_Defaults {} {
#------------------------------------------------------------------------
# display all the model child declarations of a type
#------------------------------------------------------------------------
proc set_Typetree_Defaults {} {
  global _attv ascTypeTreeVect
  set ascTypeTreeVect(backbutton) .typetree.mb_frm.back_btn
  set ascTypeTreeVect(windowname) .typetree
  set ascTypeTreeVect(canvas) .typetree.can.canvas2
  set _attv(canvas) $ascTypeTreeVect(canvas)
  set ascTypeTreeVect(geometry) 400x300+20+20
}

# canvas internals
#------------------------------------------------------------------------
#
#------------------------------------------------------------------------
#erase the canvas and clean out anything that we can afford to from the
# attv array. any persistent working variables should be added here.
# destroys the current map of text ids and bboxes.
#------------------------------------------------------------------------
proc Type_clearcan {} {
  global _attv
  Type_setlettersize
  set fontx $_attv(fontx)
  set fonty $_attv(fonty)
  set canvas $_attv(canvas)
  set bl(num) $_attv(backlength)
  for {set i 0} {$i < $bl(num)} {incr i} {
    set bl($i) $_attv(back.$i)
  }
  $_attv(canvas) delete all
  unset _attv
  for {set i 0} {$i < $bl(num)} {incr i} {
    set _attv(back.$i) $bl($i)
  }
  set _attv(backlength) $bl(num)
  set _attv(fontx) $fontx
  set _attv(fonty) $fonty
  set _attv(canvas) $canvas
}
#
# proc Type_addtext {t x y}
#------------------------------------------------------------------------
# returns the id of the text added at x y. text is bound and has value t
#------------------------------------------------------------------------
proc Type_addtext {t x y} { # add text t to canvas at x y with type bindings
  global _attv
  set _attv(id.$t) [$_attv(canvas) create text $x $y]
  $_attv(canvas) itemconfigure $_attv(id.$t) \
    -justify left \
    -text $t
  set _attv(bb.$t) [$_attv(canvas) bbox $_attv(id.$t)]
  $_attv(canvas) bind $_attv(id.$t) <B1-ButtonRelease> \
    "Type_boxtext \{$t\}"
  return $_attv(id.$t)
}

#
# proc Type_boxtext {t}
#------------------------------------------------------------------------
# add a rectangle around an existing text value t and trash last rectangle
#------------------------------------------------------------------------
proc Type_boxtext {t} {
  global _attv ascTypeTreeVect
  catch {$_attv(canvas) delete $_attv(currect)}
  set ascTypeTreeVect(curtype) $t
  if {$t==""} {return}
  set _attv(currect) [eval $_attv(canvas) create rectangle $_attv(bb.$t)]
}
#
# proc Type_setlettersize {}
#------------------------------------------------------------------------
# get a guess for lettersizes (x,y IN pixels) IN current canvas
#------------------------------------------------------------------------
proc Type_setlettersize {} {
  global _attv
  set uid [$_attv(canvas) create text -10 -10]
  $_attv(canvas) itemconfigure $uid \
    -justify left \
    -text u
  set corners [$_attv(canvas) bbox $uid]
  $_attv(canvas) delete $uid
  set _attv(fontx) [expr [lindex $corners 2] - [lindex $corners 0]]
  set _attv(fonty) [expr [lindex $corners 3] - [lindex $corners 1]]
}

#------------------------------------------------------------------------
# adds an arrow pointing from top of child to bottom of parent, centered
#------------------------------------------------------------------------
proc Type_addarrowvert {c p} { # c p are text names
  global _attv
  # draw an arrow between 2 items c p where c below p
  set pbox $_attv(bb.$p)
  set cbox $_attv(bb.$c)
  set cx [expr ([lindex $cbox 2] - [lindex $cbox 0])/2.0 + [lindex $cbox 0]]
  set px [expr ([lindex $pbox 2] - [lindex $pbox 0])/2.0 + [lindex $pbox 0]]
  set cy [expr [lindex $cbox 1] -1]
  set py [expr [lindex $pbox 3] +1]
  set _attv(id.a.$c.$p) [$_attv(canvas) create line $cx $cy $px $py]
  $_attv(canvas) itemconfigure $_attv(id.a.$c.$p) -arrow last
}
# proc Type_addarrowhorz {child parent}
#------------------------------------------------------------------------
# adds an arrow pointing from left of child to right of parent, centered
#------------------------------------------------------------------------
proc Type_addarrowhorz {c p} { # c p are text names
  global _attv
  # draw an arrow between 2 items c p where c right of p
  set pbox $_attv(bb.$p)
  set cbox $_attv(bb.$c)
  set cy [expr ([lindex $cbox 3] - [lindex $cbox 1])/2.0 + [lindex $cbox 1]]
  set py [expr ([lindex $pbox 3] - [lindex $pbox 1])/2.0 + [lindex $pbox 1]]
  set cx [expr [lindex $cbox 0] -1]
  set px [expr [lindex $pbox 2] +1]
  set _attv(id.a.$c.$p) [$_attv(canvas) create line $cx $cy $px $py]
  $_attv(canvas) itemconfigure $_attv(id.a.$c.$p) -arrow last
}

set_Typetree_Defaults

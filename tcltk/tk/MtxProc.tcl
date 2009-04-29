#  MtxProc.tcl: Mtxview procedures
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.15 $
#  Last modified on: $Date: 1998/06/18 15:54:50 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: MtxProc.tcl,v $
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

 # File structure:
 # default set procedure
 # menu button direct callbacks
 # menu button internals
 # utility routines
 # routines that should be in other files
 #
#
# proc set_Mtx_Defaults {}
#---------------------------------------------------------------------------
# set startup vars
#---------------------------------------------------------------------------
proc set_Mtx_Defaults {} {
  global ascMtxVect
  set ascMtxVect(blknumber) " "
  set ascMtxVect(blkboxid) ""
  set ascMtxVect(blkboxcorners) ""
 # blkboxcorners is valid only when blkboxid != ""
 # both are managed in the bindings
  set ascMtxVect(oldvpart) " "
  set ascMtxVect(oldrpart) " "
  set ascMtxVect(sf) 9
  set ascMtxVect(canvas) .mtx.can_mtx.canvas2
  set ascMtxVect(windowname) .mtx
}

#
# proc Mtx_do_Redraw {}
#----------------------------------------------------------------------------
# Redraw button. Checks scale for map size and updates the plot.            #
#----------------------------------------------------------------------------
proc Mtx_do_Redraw {} {
  global ascSolvStatVect ascMtxVect ascSolvVect
  if {$ascSolvVect(mtxup)} {
    set ascMtxVect(sf) [.mtx.zoom get]
    Mtx_Plot_FIncidence $ascSolvStatVect(vars) \
                       $ascSolvStatVect(rels) $ascMtxVect(sf)
  }
}
 # old version
 #proc Mtx_do_Redraw {} {
 #  global ascSolvStatVect ascMtxVect ascSolvVect
 #  if {$ascSolvVect(mtxup)} {
 #    set ascMtxVect(sf) [.mtx.zoom get]
 #    Mtx_Plot_CIncidence $ascSolvStatVect(vars) \
 #                       $ascSolvStatVect(rels) $ascMtxVect(sf) \
 #                        [dbg_get_order r] \
 #                        [dbg_get_order c] 1
 #  }
 #}

#
# proc Mtx_do_Grill {}
#----------------------------------------------------------------------------
# Mtx attributes set command                                                #
#----------------------------------------------------------------------------
proc Mtx_do_Grill {} {
  puts stderr "There will be options when someone tells me what they want"
  Mtx_do_Help
}

#
# proc Mtx_do_PrintBlock {}
#----------------------------------------------------------------------------
# the print block button internals.                                         #
#----------------------------------------------------------------------------
proc Mtx_do_PrintBlock {} {
  global ascMtxVect
  if {"$ascMtxVect(blkboxid)" != ""} {
    Print_configure $ascMtxVect(windowname) Printer
    if {[Print_cancelcheck]} {
      return
    }
    DispPrint [DispWriteCanvasRegion  \
      $ascMtxVect(canvas) $ascMtxVect(blkboxcorners)]
    HUB_Message_to_HUB WINDOWPRINTED MTX
  } else {
    puts  stderr "Can't print 1x1 or nonexistent blocks"
  }
}

#
# proc Mtx_do_Print {}
#----------------------------------------------------------------------------
# the print button internals.
#
#----------------------------------------------------------------------------
proc Mtx_do_Print {} {
  global ascMtxVect
  Print_configure $ascMtxVect(windowname) Printer
  if {[Print_cancelcheck]} {
    return
  }
  DispPrint [DispWriteSelection $ascMtxVect(canvas)]
  HUB_Message_to_HUB WINDOWPRINTED MTX
}

#
# proc Mtx_do_OK {}
#----------------------------------------------------------------------------
# the ok button internals.                                                  #
#----------------------------------------------------------------------------
proc Mtx_do_OK {} {
  global ascMtxVect
  View_Save_SpecialWindow_Values matrix
  set ascMtxVect(blkboxid) ""
  Solve_CloseMtx
}

#
# proc Mtx_do_Font {}
#---------------------------------------------------------------------------
# set the font for mtx
#---------------------------------------------------------------------------
proc Mtx_do_Font {args} {
  global ascMtxVect
  set font ""
  if {$args !=""} {
    set font $args
  } else {
    set font [ascFontGet]
  }
  if {"$font" == ""} {
    return
  }
  if {![winfo exists .mtx]} {
    set ascMtxVect(font) $args
    return
  }
  foreach i { col eqn row var blk } {
    .mtx.$i.label4 configure -font $font
    .mtx.$i.entry5 configure -font $font
  }
  .mtx.btn_ok configure -font $font
  .mtx.btn_mtx_red configure -font $font
  set ascMtxVect(font) $font
}
  
#
# proc Mtx_do_Help {}
#---------------------------------------------------------------------------
# the partial semantic description of the matrix                           #
#---------------------------------------------------------------------------
proc Mtx_do_Help {} {
  Help_button solver.display incidencematrix
}

#
# proc mp { cols rows colwid rowhit size func}
#----------------------------------------------------------------------------
# cols horiz/vert dim in sqrs , colwid rowhit spacing per square, size XxY
# func is external that returns a square type code for valid col/row args
# returns map to 0:no bitmap 1: solid 2: hollow 3: cross 4: cross hollow
# binds to set var/row indexes
#
# plot a test pattern
# this is good for up to 200 eqns, but gets expensive for large dense
# systems. use atobm from X distribution to build a bitmap in /tmp
# on the fly. Display monolith but set mouse divisions accordingly
#----------------------------------------------------------------------------
proc mp { cols rows colwid rowhit size func} {
   global ascMtxVect ascSolvStatVect
  .mtx.can_mtx.canvas2 delete all
  .mtx.can_mtx.canvas2 config -cursor left_ptr
  .mtx.can_mtx.canvas2 config -scrollregion \
    "0 0 [expr ($cols+1)*$colwid] [expr ($rows +1)*$rowhit]"
  set ascMtxVect(colwid) $colwid
  set ascMtxVect(rowhit) $rowhit
 # calc position from canvas coords. binding
  bind .mtx.can_mtx.canvas2 <Button-1>  {
    set ascMtxVect(colindex)  [expr 1+ [.mtx.can_mtx.canvas2 canvasx \
      [expr %x -2 ]]/$ascMtxVect(colwid)]
    set ascMtxVect(rowindex)  [expr 1+[.mtx.can_mtx.canvas2 canvasy  \
      [expr %y - 2]]/$ascMtxVect(rowhit)]}

  for {set i 1} { $i <= $cols} {set i [expr $i + 1]} {
    for {set j 1} { $j <= $rows} {set j [expr $j + 1]} {
      set x [expr $colwid*$i]
      set y [expr $rowhit*$j]
      switch [$func $i $j] {
        {0} {NoFunction}
        {1} {.mtx.can_mtx.canvas2 create bitmap $x $y \
              -bitmap "asc_sq_$size"}
        {2} {.mtx.can_mtx.canvas2 create bitmap $x $y \
              -bitmap "asc_sq_h$size"}
        {3} {.mtx.can_mtx.canvas2 create bitmap $x $y \
              -bitmap "asc_sq_c$size"}
        {4} {.mtx.can_mtx.canvas2 create bitmap $x $y \
              -bitmap "asc_sq_x$size"}
        default {NoFunction}
      }
    }
  }
}

#
# proc mtx_incident {var eqn}
#---------------------------------------------------------------------------
# test pattern generator
#---------------------------------------------------------------------------
proc mtx_incident {var eqn} {
  return [expr ($var + $eqn) %5]
}
#
# proc dbg_incident {var eqn}
#---------------------------------------------------------------------------
# check for incidence of variable in eqn
#---------------------------------------------------------------------------
proc dbg_incident {var eqn} {
  if {[lsearch [dbg_get_incidence $eqn] $var] != "-1"} {
    return 1
  }
}

#
# proc Mtx_Plot_CIncidence {cols rows sf relpart varpart new}
#----------------------------------------------------------------------------
# plot a matrix from slv                                                    #
# cols = total number of vars in system                                     #
# rows = total number of relations                                          #
# sf = size factor from 1 to 14, applied to bitmaps                         #
# relpart/varpart are dbg_get_order data                                    #
# plots free variables in included eqns as solid                            #
#       fixed vars in included eqns as crosses                              #
#       free vars in unincluded eqns as hollow squares                      #
#       fixed vars in unincluded eqns as hollow crossed squares             #
#
# code in disuse, at least until FIncidence proves buggy. This works
# with the old Redraw code
#----------------------------------------------------------------------------
proc Mtx_Plot_CIncidence {cols rows sf relpart varpart new} {

  global ascMtxVect ascMtxVarVect ascSolvStatVect 

  .mtx.can_mtx.canvas2 config -cursor left_ptr
  .mtx.can_mtx.canvas2 delete all
  .mtx.can_mtx.canvas2 config -scrollregion \
    "0 0 [expr ($cols+1)*$sf] [expr ($rows +1)*$sf]"
  set ascMtxVect(sf) $sf

 # set up the canvas binding for the grid given
  bind .mtx.can_mtx.canvas2 <Button-1>  {
  # grid coords go from 0 to n-1, matching the C
    set globx [.mtx.can_mtx.canvas2 canvasx [expr %x ]]
    set globy [.mtx.can_mtx.canvas2 canvasy [expr %y ]]
    set ascMtxVect(colindex)  [expr $globx / $ascMtxVect(sf)]
    set ascMtxVect(rowindex)  [expr $globy / $ascMtxVect(sf)]
  # sanity on grid input, due to scrollbar silliness potential
    if {[expr $ascMtxVect(colindex) <0]} {
      set ascMtxVect(colindex) 0
    }
    if {[expr $ascMtxVect(colindex) > [expr $ascSolvStatVect(inc_vars) -1]]} {
      set ascMtxVect(colindex) [expr $ascSolvStatVect(inc_vars) -1]
    }
    if {[expr $ascMtxVect(rowindex) <0]} {
      set ascMtxVect(rowindex) 0
    }
    if {[expr $ascMtxVect(rowindex) > [expr $ascSolvStatVect(rels)-1] ]} {
      set ascMtxVect(rowindex) [expr $ascSolvStatVect(rels) -1]
    }
  # get names
    set ascMtxVect(varindex) \
      [lindex $ascMtxVect(collist) $ascMtxVect(colindex)]
    set ascMtxVect(eqnindex) \
      [lindex $ascMtxVect(rowlist) $ascMtxVect(rowindex)]
    set ascMtxVect(eqnname) [dbg_write_rel 2 $ascMtxVect(eqnindex) 0]
    set ascMtxVect(varname) [dbg_write_var 2 $ascMtxVect(varindex) 0 0]
    set ascMtxVect(eqnnum) [expr $ascMtxVect(eqnindex)]
    set ascMtxVect(varnum) [expr $ascMtxVect(varindex)]
    set ascMtxVect(blknumber) [dbg_get_blk_of_var $ascMtxVect(varindex)]
    catch {.mtx.can_mtx.canvas2 delete $ascMtxVect(blkboxid)}
    set ascMtxVect(blkboxid) ""
    if {$ascMtxVect(blknumber)!="none"} {
      set ascMtxVect(blkcorners) [dbg_get_blk_coords $ascMtxVect(blknumber)]
      set ixlo [lindex $ascMtxVect(blkcorners) 0]
      set iylo [lindex $ascMtxVect(blkcorners) 1]
      set ixhi [lindex $ascMtxVect(blkcorners) 2]
      set iyhi [lindex $ascMtxVect(blkcorners) 3]
      if {$ixlo != $ixhi || $iylo != $iyhi} {
        set bxlo [expr 2+ $ixlo * $ascMtxVect(sf)]
        set bylo [expr 2+ $iylo * $ascMtxVect(sf)]
        set bxhi [expr 2+ (1+ $ixhi) * $ascMtxVect(sf)]
        set byhi [expr 2+ (1+ $iyhi) * $ascMtxVect(sf)]
        set ascMtxVect(blkboxid) \
          [.mtx.can_mtx.canvas2 create rectangle $bxlo $bylo $bxhi $byhi]
        set ascMtxVect(blkboxcorners) "$bxlo $bylo $bxhi $byhi"
      }
    } else {
      set ascMtxVect(blkboxid) ""
      set ascMtxVect(blkcorners) "0 0 0 0"
    }
  # prepend instance inplace of & in interpresult and remove braces
    regsub & $ascMtxVect(eqnname) $ascSolvVect(simname) ascMtxVect(eqnname)
    regsub & $ascMtxVect(varname) $ascSolvVect(simname) ascMtxVect(varname)
    regsub \{ $ascMtxVect(eqnname) "" ascMtxVect(eqnname)
    regsub \} $ascMtxVect(eqnname) "" ascMtxVect(eqnname)
    regsub \{ $ascMtxVect(varname) "" ascMtxVect(varname)
    regsub \} $ascMtxVect(varname) "" ascMtxVect(varname)
  }
 # end binding

 # in the Case where we only need a window resize, maybe
 # not a C update, skip all the variable manipulations and just plot
  if {!($ascMtxVect(oldvpart)==$varpart) ||
      !($ascMtxVect(oldrpart)==$relpart) ||
      ($new)} {
 #   redo from scratch
    set ascMtxVect(oldvpart) $varpart
    set ascMtxVect(oldrpart) $relpart

 #collist is just varpart now, but collist is all inclusive too
 # strip out phantom columns due to square mtx in C
    set  ascMtxVect(collist) ""
    foreach i $varpart {
      if {$i < $ascSolvStatVect(vars)} {lappend ascMtxVect(collist) $i}
    }
 # sort mtx ordering out to fixed/nonincident on right
 # move any fixed and incident he's partitioned to the right
 # move to wayfar right non-incident vars
    set fixedlist [dbg_list_vars 2]
    set notinclist [dbg_list_vars 1 not]
    set farright ""
    foreach i $fixedlist {
      if {[lsearch $notinclist $i] == "-1"} {
        lappend farright $i
      }
    }
    foreach i $farright {
      set cpos [lsearch $ascMtxVect(collist) $i]
      set ascMtxVect(collist) [lreplace $ascMtxVect(collist) $cpos $cpos]
      lappend ascMtxVect(collist) $i
    }
    foreach i $notinclist {
      set cpos [lsearch $ascMtxVect(collist) $i]
      set ascMtxVect(collist) [lreplace $ascMtxVect(collist) $cpos $cpos]
      lappend ascMtxVect(collist) $i
    }
    set ascMtxVect(usedcols) [llength $ascMtxVect(collist)]

 #set array which is columninfo subscripted by C var#
 # done for faster plotting purposes
    set ascMtxVect(col) 0
    foreach i $ascMtxVect(collist) {
      set ascMtxVarVect($i) $ascMtxVect(col)
      incr ascMtxVect(col)
    }

 # list created -> lindex list row# =Crelnumber
 # stick unassigned equations in middle since they dont appear on part list
 # mash unincluded to bottom
 # strip out phantom columns due to square mtx in C
    set  ascMtxVect(rowlist) ""
    foreach i $relpart {
      if {$i < $ascSolvStatVect(rels)} {lappend ascMtxVect(rowlist) $i}
    }
    set asslist [dbg_list_rels 4]
    if {$asslist !=""} {
      set notasslist [dbg_list_rels 4 not]
    } else {
      set notasslist ""
    }
    foreach i $notasslist {
      set rpos [lsearch $ascMtxVect(rowlist) $i]
      set ascMtxVect(rowlist) [lreplace $ascMtxVect(rowlist) $rpos $rpos]
      lappend ascMtxVect(rowlist) $i
    }
    set notinclist [dbg_list_rels 1 not]
    foreach i $notinclist {
      set rpos [lsearch $ascMtxVect(rowlist) $i]
      set ascMtxVect(rowlist) [lreplace $ascMtxVect(rowlist) $rpos $rpos]
      lappend ascMtxVect(rowlist) $i
    }
  }

 # start here if this is a replot
 #plot the matrix
  set ascMtxVect(drow) 0
  foreach rel $ascMtxVect(rowlist) {
    if {[catch {set varsinrow [dbg_get_incidence $rel]} ]} {
    } else {
      foreach var $varsinrow {
        set gx $ascMtxVarVect($var)
        set gy $ascMtxVect(drow)
        set x [expr $sf*$gx +2]
        set y [expr $sf*$gy+2]
 #puts stderr "picking var bitmap"
# if the code dies here, you have mismatch between libtcl.a and interface
# objects. use the same CC for both.
# command line symptom [dbg_write_rel 2 0 3] will die in Tcl_Parse
        if {[dbg_rel_included $rel]} {
          if {[dbg_var_fixed $var]} {
            set type "asc_sq_c"
          } else {
            set type "asc_sq_"
          }
        } else {
          if {[dbg_var_fixed $var]} {
            set type "asc_sq_x"
          } else {
            set type "asc_sq_h"
          }
        }
        .mtx.can_mtx.canvas2 create bitmap $x $y \
           -bitmap "$type$sf" -anchor nw
      }
    incr ascMtxVect(drow)
    }
  }
}

#
# proc Mtx_Plot_FIncidence {sf}
#----------------------------------------------------------------------------
# plot a matrix from slv                                                    #
# cols = total number of vars in system                                     #
# rows = total number of relations                                          #
# sf = size factor from 1 to 14, applied to bitmaps                         #
#                                                                           #
# cols and rows still needed at tcl level to bind canvas                    #
# plots free variables in included eqns as solid                            #
#       fixed vars in included eqns as crosses                              #
#       free vars in unincluded eqns as hollow squares                      #
#       fixed vars in unincluded eqns as hollow crossed squares             #
#----------------------------------------------------------------------------
proc Mtx_Plot_FIncidence {cols rows sf} {

  global ascMtxVect ascMtxVarVect ascSolvStatVect
  global ascMtxVect_ra
  global ascMtxVect_ca
  global ascMtxVect_va
  global ascMtxVect_ea
  .mtx.can_mtx.canvas2 config -cursor left_ptr
  .mtx.can_mtx.canvas2 delete all
  .mtx.can_mtx.canvas2 config -scrollregion \
    "0 0 [expr ($cols+1)*$sf] [expr ($rows +1)*$sf]"
  set ascMtxVect(sf) $sf

 # set up the canvas binding for the grid given
  bind .mtx.can_mtx.canvas2 <Button-1>  {
    global ascMtxVect
  # grid coords go from 0 to n-1, matching the C
    set globx [expr round([.mtx.can_mtx.canvas2 canvasx [expr %x ]])]
    set globy [expr round([.mtx.can_mtx.canvas2 canvasy [expr %y ]])]
    set ascMtxVect(colindex)  [expr $globx / $ascMtxVect(sf)]
    set ascMtxVect(rowindex)  [expr $globy / $ascMtxVect(sf)]
  # sanity on grid input, due to scrollbar silliness potential
    if {[expr $ascMtxVect(colindex) <0]} {
      set ascMtxVect(colindex) 0
    }
    if {[expr $ascMtxVect(colindex) > [expr $ascSolvStatVect(inc_vars) -1]]} {
      set ascMtxVect(colindex) [expr $ascSolvStatVect(inc_vars) -1]
    }
    if {[expr $ascMtxVect(rowindex) <0]} {
      set ascMtxVect(rowindex) 0
    }
    if {[expr $ascMtxVect(rowindex) > [expr $ascSolvStatVect(rels)-1] ]} {
      set ascMtxVect(rowindex) [expr $ascSolvStatVect(rels) -1]
    }
  # get names
    set ascMtxVect(varindex) $ascMtxVect_ca($ascMtxVect(colindex))
    set ascMtxVect(eqnindex) $ascMtxVect_ra($ascMtxVect(rowindex))
    set ascMtxVect(eqnname) [dbg_write_rel 2 $ascMtxVect(eqnindex) 0]
    set ascMtxVect(varname) [dbg_write_var 2 $ascMtxVect(varindex) 0 0]
    set ascMtxVect(eqnnum) [expr $ascMtxVect(eqnindex)]
    set ascMtxVect(varnum) [expr $ascMtxVect(varindex)]
    set ascMtxVect(blknumber) [dbg_get_blk_of_var $ascMtxVect(varindex)]
    catch {.mtx.can_mtx.canvas2 delete $ascMtxVect(blkboxid)}
    set ascMtxVect(blkboxid) ""
    if {$ascMtxVect(blknumber)!="none"} {
      set ascMtxVect(blkcorners) [dbg_get_blk_coords $ascMtxVect(blknumber)]
      set ixlo [lindex $ascMtxVect(blkcorners) 0]
      set iylo [lindex $ascMtxVect(blkcorners) 1]
      set ixhi [lindex $ascMtxVect(blkcorners) 2]
      set iyhi [lindex $ascMtxVect(blkcorners) 3]
      if {$ixlo != $ixhi || $iylo != $iyhi} {
        set bxlo [expr 2+ $ixlo * $ascMtxVect(sf)]
        set bylo [expr 2+ $iylo * $ascMtxVect(sf)]
        set bxhi [expr 2+ (1+ $ixhi) * $ascMtxVect(sf)]
        set byhi [expr 2+ (1+ $iyhi) * $ascMtxVect(sf)]
        set ascMtxVect(blkboxid) \
          [.mtx.can_mtx.canvas2 create rectangle $bxlo $bylo $bxhi $byhi]
        set ascMtxVect(blkboxcorners) "$bxlo $bylo $bxhi $byhi"
      }
    } else {
      set ascMtxVect(blkboxid) ""
      set ascMtxVect(blkcorners) "0 0 0 0"
    }
  # prepend instance inplace of & in interpresult and remove braces
    regsub & $ascMtxVect(eqnname) $ascSolvVect(simname) ascMtxVect(eqnname)
    regsub & $ascMtxVect(varname) $ascSolvVect(simname) ascMtxVect(varname)
    regsub \{ $ascMtxVect(eqnname) "" ascMtxVect(eqnname)
    regsub \} $ascMtxVect(eqnname) "" ascMtxVect(eqnname)
    regsub \{ $ascMtxVect(varname) "" ascMtxVect(varname)
    regsub \} $ascMtxVect(varname) "" ascMtxVect(varname)
  }
 # end binding
 # plot the beast
  mtx_gui_plot_incidence $sf 2 2 .mtx.can_mtx.canvas2 black white \
    ascMtxVect_ra ascMtxVect_ca \
    ascMtxVect_va ascMtxVect_ea
}


#  ascplotproc.tcl: a columnar data manipulator
#  by Benjamin Allan
#  August 1995
#  Part of ASCEND
#  Revision: $Revision: 1.38 $
#  Last modified on: $Date: 2003/02/06 13:49:43 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: ascplotproc.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1995-1998 Carnegie Mellon University
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

#
# This file requires the tkTable extension 1.4 or later.
#
# The idea here is that we can take ASCII numeric data files
# in a columnar format and feed various bizarre x/unix plot
# programs. If there is direct access to a decent spreadsheet
# or real plot package, this isn't needed. Limited spreadsheetlike
# functionality is supported.
#
# Depends:
# tkTable widget.
# generalk.tcl
# templates/Procedures/ascparm.tcl
# templates/Procedures/ascoktext.tcl
#
# global variables:
# public:
# ascplotvect (an array with many many elements)
# private:
# plotan$setuid (data arrays)
# ascplot_xgraphvect xgraph plot window configuration ascparmpage
#
# public elements of ascplot_xgraphvect:
# command  system command for invoking xgraph
#
# public elements of ascplotvect:
# titlefont  font for label at the top of the window
# textfont   font for lists, texts, tables, entries
# btnfont    font for buttons and misc widgets
# textname   widget pathname of the message text.
# dataname   widget pathname of the data table
# setsname   widget pathname of the set list
# varsname   widget pathname of the vars list
# depname    widget pathname of the dependent vars list
# indvname   widget pathname of the independent var entry
# bitmap     bitmap name
#
# private elements:
# nfiles          no. of files known
# file.$i         name of ith file
# nsets           no. of data sets known
# set.$j          {{filenum} {setnum}} of jth data set
# topline.$j      initial line of set j
# array.$j        array name of the global array for the table of set j
# cols.$j         number of columns presently in data set j
# rows.$j         number of rows presently in data set j
# curset          number of the current selected data set
# curfile         number of the current file, redundant
# sel.var         current selection indices in the vars listbox
# sel.dep         current selection indices in the dependent vars listbox
# sel.dat         current selection indices in the data sets listbox
# note: sel.*may be unset if there is no selection
# filename        latest file attempted name
# setctr          a number we will use in making up array names
# plotter         one of {xgraph }, the currently selected plotter.
# uplegend        boolean value indicating window is up or not
# showdata        boolean value indicating what data view should do next
#
# The following info is stored with the data array. aname is
# the name in $ascplotvect(array.$set), the name of the data array.
# ${aname}(-7,$c)       type of column c in set {data, calcexpr}
# ${aname}(-6,$c)       legend of column c in set
# ${aname}(-5,$c)       full variable name of column c in set
# ${aname}(-4,$vid)     column of vid in set
# ${aname}(-3,$c)       column number, redundant but video
# ${aname}(-2,$c)       variable id of column c in set
# ${aname}(-1,$c)       variable units of column c in set
# ${aname}($r,-1)       row # of row r in set
# ${aname}($r,-2)       type of row r {data, calcexpr}
# Don't forget tcls indirect address! the value of ${aname}(-1,$c)
# can only be retrieved as [set ${aname}(-1,$c)] or by otherwise
# invoking a second pass on ${aname}(-1,$c) with eval.
# A calcexpr is the most recent mathematical expression evaluated
# to fill up the row or column. data may have been altered since.
#
# plot info
# This portion of the ascplotvect is compatible with ascParPage.
# The dependent vars are plotted as ordered in the dependent box.
#   ncurves is the number of dependent variables
#   legend.$c is the legend for curve c
#   plottitle is the title of the plot
#   xtitle is the horizontal label x axis
#   ytitle is the vertical label y axis
# Everything else is controlled by the specific plotter parameters.
# We try to cook up sane defaults for the titles and legends from the data.
#

proc set_ascplot_defaults {} {
  global ascplotvect
  if {0} {
   # do nothing
  } else {
    global ascplotvect ascGlobalVect
    if {[info exists ascGlobalVect(viewoptions)]} {
      # steal ascend defaults
      set ascplotvect(titlefont) $ascGlobalVect(labelfont)
      set ascplotvect(textfont) $ascGlobalVect(font)
      set ascplotvect(btnfont) $ascGlobalVect(font)
      set ascplotvect(bitmap) "$ascGlobalVect(toolbitmap)"
    } else {
      set ascplotvect(titlefont) {helvetica 12 bold}
      set ascplotvect(textfont) {helvetica 12 bold}
      set ascplotvect(btnfont) {helvetica 12 bold}
      set ascplotvect(bitmap) "info"
    }
    set ascplotvect(dataname) {.ascplot.bot_frm.table_frm.datatable.table1}
    set ascplotvect(textname) {.ascplot.bot_frm.message.text2}
    set ascplotvect(setsname) {.ascplot.top_frm.datasets.listbox1}
    set ascplotvect(varsname) {.ascplot.top_frm.varfrm.varsel.varlist.listbox1}
    set ascplotvect(depvname) {.ascplot.top_frm.varfrm.varsel.depvar.listbox1}
    set ascplotvect(indvname) {.ascplot.top_frm.varfrm.indepvar.entry5}
    set ascplotvect(filename) {obs.dat}
    set ascplotvect(xgraph_filename) {plot.xgraph}
    set ascplotvect(entrywidth) 20
    set ascplotvect(uplegend) 0
    set ascplotvect(showdata) 1
    set ascplotvect(array.-1) apv.dummy

    set ascplotvect(plotter) xgraph
    set ascplotvect(plotterchoices) {xgraph}
    set ascplotvect(nfiles) 0
    set ascplotvect(nsets) 0
    set ascplotvect(curset) -1
    set ascplotvect(windowname) .ascplot
    set ascplotvect(winopen) 0

   # parpage plot stuff defaults. can have more legends
    set ascplotvect(ncurves) 0
    for {set i 0} {$i < 64} {incr i} {
      set ascplotvect(legend$i) "legend$i"
      set ascplotvect(legend$i.type) string
      set ascplotvect(legend$i.label) "Legend for $i"
    }
    set ascplotvect(npages) 1
    set ascplotvect(grab) 0
    set ascplotvect(helpcommand) {
      global ascplotvect
      OKText .ascplot_help "Which word didn't you understand??" "Doh!!" \
      0 $ascplotvect(textfont) [getpos .ascplot] 12 40
    }
    set ascplotvect(whenokcommand) ascplot_genericok
    set ascplotvect(title) "Graph Generics"
    set ascplotvect(toplevel) .ascplot_titles
    set ascplotvect(namelist) \
      [list plottitle xtitle ytitle \
       legend0 legend1 legend2 legend3 \
       legend4 legend5 legend6 legend7]
    set ascplotvect(baselist) "plottitle xtitle ytitle"
    set ascplotvect(xtitle) X
    set ascplotvect(xtitle.label) "X Axis Title"
    set ascplotvect(ytitle) Y
    set ascplotvect(ytitle.label) "Y Axis Title"
    set ascplotvect(plottitle) AscPlot
    set ascplotvect(plottitle.label) "Plot Title"
  }
}
set_ascplot_defaults

proc ascplot_setssdef {} {
  global ascplot_ssvect
  set ascplot_ssvect(newcolnum) 0
  set ascplot_ssvect(npages) 1
  set ascplot_ssvect(grab) 1
  set ascplot_ssvect(entrywidth) 40
  set ascplot_ssvect(helpcommand) {ascplot_sshelp}
  set ascplot_ssvect(whenokcommand) "ascplot_insert col ok"
  set ascplot_ssvect(title) "Create Data"
  set ascplot_ssvect(toplevel) .ascplot_sscreate
  set ascplot_ssvect(namelist) \
    [list after last dtype formula cancel]
  set ascplot_ssvect(after) 0
  set ascplot_ssvect(after.lo) 0
  set ascplot_ssvect(after.hi) 0
  set ascplot_ssvect(after.type) int
  set ascplot_ssvect(after.label) {Insert after Column}
  set ascplot_ssvect(dtype) data
  set ascplot_ssvect(dtype.type) string
  set ascplot_ssvect(dtype.choices) "data formula"
  set ascplot_ssvect(dtype.label) "Column type"
  set ascplot_ssvect(formula) {A($r,$c) + 1}
  set ascplot_ssvect(formula.type) string
  set ascplot_ssvect(formula.label) Formula
  set ascplot_ssvect(colformula) {A($r,$c-1) + 1}
  set ascplot_ssvect(colformula.type) string
  set ascplot_ssvect(colformula.label) Formula
  set ascplot_ssvect(rowformula) {A($r-1,$c) + 1}
  set ascplot_ssvect(rowformula.type) string
  set ascplot_ssvect(rowformula.label) Formula
  set ascplot_ssvect(last) 0
  set ascplot_ssvect(last.type) bool
  set ascplot_ssvect(last.label) {Insert at end (overrides col)}
  set ascplot_ssvect(cancellable) 0
  set ascplot_ssvect(cancel) 0
  set ascplot_ssvect(cancel.type) bool
  set ascplot_ssvect(cancel.label) {Forget this insertion}
}


# dummy widget info
proc ascplot_setdummy {} {
  global ascplotvect apv.dummy
  set ascplotvect(curset) -1
  set ascplotvect(curfile) -1
  set ascplotvect(nfiles) 1
  set ascplotvect(nsets) 1
  set ascplotvect(file.-1) /tmp/dummy.baa
  set ascplotvect(set.-1) {-1 -1}
  set ascplotvect(array.-1) apv.dummy
  set ascplotvect(dummyvar) apv.dummy
  set ascplotvect(cols.-1) 4
  set ascplotvect(rows.-1) 4
 # put data here
}

#
# ascplot_seldataset {}
#---------------------------------------------------------------------
# makes the first mouse 
# selected set in the dataset list the current working set.
#---------------------------------------------------------------------
proc ascplot_seldataset {} {
  global ascplotvect
  if {![info exists ascplotvect(sel.dat)] || $ascplotvect(sel.dat)==""} {
    return
  }
 # get list index
  set new [lindex $ascplotvect(sel.dat) 0]
 # get data set index
  set new [lindex $ascplotvect(list2set) $new]
 # make new the current set
  ascplot_select_set_by_number $new
}

#
# ascplot_select_set_by_number {new}
#---------------------------------------------------------------------
# makes the numbered set the current working set.
#---------------------------------------------------------------------
proc ascplot_select_set_by_number {new} {
  global ascplotvect
  set ascplotvect(curset) $new
  set ascplotvect(sel.dat) ""
  set ascplotvect(curfile) \
    $ascplotvect(file.[lindex $ascplotvect(set.$new) 0])
  ascplot_showdata 0
  $ascplotvect(varsname) delete 0 end
  $ascplotvect(depvname) delete 0 end
  set ncol "$ascplotvect(cols.$new)"
  set aname "$ascplotvect(array.$new)"
  global $aname
  # make list of col#) shortid longid units
  for {set i 0} {$i < $ncol} {incr i} {
    $ascplotvect(varsname) insert end \
      "$i) [set ${aname}(-2,$i)] [set ${aname}(-5,$i)] [set ${aname}(-1,$i)]"
  }

  $ascplotvect(varsname) selection clear 0 end
  catch {
    $ascplotvect(varsname) selection set 0 0
  }
  set ascplotvect(sel.var) 0
  ascplot_selindependentleft discard
  ascplot_setlegends
  ascplot_showdata 1
}

#
# proc ascplot_unseldependent {}
#---------------------------------------------------------------------
# moves lines from right box to end of left box
#---------------------------------------------------------------------
proc ascplot_unseldependent {} {
  global ascplotvect
  set dlist -1
  if {![info exists ascplotvect(sel.dep)] || $ascplotvect(sel.dep)==""} {
    return
  }
  foreach i $ascplotvect(sel.dep) {
    $ascplotvect(varsname) insert end [$ascplotvect(depvname) get $i]
    set dlist [linsert $dlist 0 $i]
  }
  foreach i $dlist {
    if {[expr $i > -1]} {
      $ascplotvect(depvname) delete $i $i
    }
  }
  set ascplotvect(sel.dep) ""
  ascplot_setlegends
}
#
# proc ascplot_seldependent {}
#---------------------------------------------------------------------
# moves lines from left box to end of right box based on mouse
#---------------------------------------------------------------------
proc ascplot_seldependent {} {
  global ascplotvect
  if {![info exists ascplotvect(sel.var)] || $ascplotvect(sel.var)==""} {
    return
  }
  ascplot_seldependent_list $ascplotvect(sel.var)
  set ascplotvect(sel.var) ""
  ascplot_setlegends
}

#
# proc ascplot_seldependent_list {sellist}
#---------------------------------------------------------------------
# moves lines from left box to end of right box based on list input.
# list must be integers, not {1 end}
#---------------------------------------------------------------------
proc ascplot_seldependent_list {sellist} {
  global ascplotvect
  set dlist -1
  foreach i $sellist {
    $ascplotvect(depvname) insert end [$ascplotvect(varsname) get $i]
    set dlist [linsert $dlist 0 $i]
  }
  foreach i $dlist {
    if {[expr $i > -1]} {
      $ascplotvect(varsname) delete $i $i
    }
  }
}
#
# proc ascplot_showtitles {openclose}
#---------------------------------------------------------------------
# opens or closes generic titles window when called with <open,close>
# if called when already open, redraws in current position.
# binds the window so that titles and legends in data arrays stay synced
#---------------------------------------------------------------------
proc ascplot_showtitles {oc} {
  global ascplotvect
  if {$oc== "close" && $ascplotvect(uplegend)} {
    catch "destroy $ascplotvect(toplevel)"
    set ascplotvect(uplegend) 0
    return
  }
  if {$oc=="open"} {
    if {$ascplotvect(uplegend)} {
      set geom [getpos $ascplotvect(toplevel)]
      destroy $ascplotvect(toplevel)
      ascParPage ascplotvect $geom 1
    } else {
      ascParPage ascplotvect [setpos .ascplot 20 20] 1
      set ascplotvect(uplegend) 1
    }
    bind $ascplotvect(toplevel) <Any-Leave> {
      set aname $ascplotvect(array.$ascplotvect(curset))
      for {set i 0} {$i < $ascplotvect(ncurves)} {incr i} {
        set col "[lindex $ascplotvect(legend$i.label) 1]"
        set ${aname}(-6,$col) "$ascplotvect(legend$i)"
      }
    }
    return
  }
}
#
# proc ascplot_showdata {show}
#---------------------------------------------------------------------
# makes the array with dataset $ascplotvect(curset) the displayed data
# table. If show is 0, only header rows are shown.
# if show is 1, shows all data.
# Configures menu appropriately
#---------------------------------------------------------------------
proc ascplot_showdata {show} {
  global ascplotvect
  set cs $ascplotvect(curset)
  if {$show} {
    $ascplotvect(dataname) configure \
      -variable $ascplotvect(array.$cs) \
      -rows [expr $ascplotvect(rows.$cs) + 3] \
      -cols [expr $ascplotvect(cols.$cs) + 1]
    .ascplot.menubar.display entryconfigure 0 \
      -label "Hide data"
    set ascplotvect(showdata) 0
  } else {
    $ascplotvect(dataname) configure \
      -variable $ascplotvect(array.$cs) \
      -rows 3 \
      -cols [expr $ascplotvect(cols.$cs) + 1]
    .ascplot.menubar.display entryconfigure 0 \
      -label "Show data"
    set ascplotvect(showdata) 1
  }
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_defineplot {} {
  ascplot_showtitles open
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_loadplot {} {
  error "ascplot_loadplot unimplemented"
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_updateplot {} {
  error "ascplot_updateplot unimplemented"
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_deleteplot {} {
  error "ascplot_deleteplot unimplemented"
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_loaddata {} {
  global ascplotvect
  set defaultname "[pwd]"
  set filename [tk_getOpenFile \
    -defaultextension .dat \
    -filetypes $ascplotvect(filetypes) \
    -initialdir $defaultname \
    -parent .ascplot \
    -title {Load data file}]

  if {$filename == ""} {
    return
  } else {
    ascplot_parse_file $filename
    ascplot_drawsets
  }
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_savedata {} {
  error "ascplot_savedata not implemented."
}
#
# proc ascplot_unloaddata {interactive}
#---------------------------------------------------------------------
# delete data sets. if interactive, prompt for confirmation of
# deletion.
#---------------------------------------------------------------------
proc ascplot_unloaddata {int} {
  global ascplotvect
  if {![info exists ascplotvect(sel.dat)] || $ascplotvect(sel.dat)==""} {
    return
  }
 # names to be nuked
  set killsets ""
 # set numbers to be nuked
  set killlist ""
 # set true if cur to be killed
  set killcur 0
  foreach i $ascplotvect(sel.dat) {
 # get data set index
    set new "[lindex $ascplotvect(list2set) $i]"
    lappend killlist $new
    append killsets "[$ascplotvect(setsname) get $i]\n"
    if {$new == $ascplotvect(curset)} {
      set killcur 1
    }
  }
  set ascplotvect(sel.dat) ""

  if {[tk_dialog .question "Delete these data sets?" \
         $killsets {} 0 OK Cancel] == 0} {
    if {$killcur} {
      ascplot_showdata 0
      set ascplotvect(curset) -1
      set ascplotvect(curfile) ""
      $ascplotvect(dataname) configure -variable apv.dummy
      set ascplotvect(sel.dep) ""
      set ascplotvect(sel.var) ""
      set ew $ascplotvect(indvname)
      $ew configure -state normal
      $ew delete 0 end
      $ew configure -state disabled
      $ascplotvect(depvname) delete 0 end
      $ascplotvect(varsname) delete 0 end
    }
    foreach i $killlist {
      set ascplotvect(rows.$i) 0
      set aname "$ascplotvect(array.$i)"
      global $aname
      unset $aname
      set fn [lindex $ascplotvect(set.$i) 0]
      incr ascplotvect(filecnt.$fn) -1
    }
    ascplot_drawsets
    if {$killcur} {
      ascplot_setlegends
    }
  } else {
    $ascplotvect(setsname) selection clear 0 end
  }
}

proc ascplot_reloaddata {int} {
  # by cf2w. prolly needs checking 
  global ascplotvect
  # names to be nuked
  set killsets ""
  # set numbers to be nuked
  set killlist ""
  # set TRUE if cur to be killed
  set killcur 0
  foreach i $ascplotvect(sel.dat) {
    # get data set index
    set new "[lindex $ascplotvect(list2set) $i]"
    lappend killlist $new
    append killsets "[$ascplotvect(setsname) get $i]\n"
    if {$new == $ascplotvect(curset)} {
      set killcur 1
    }
  }
  set ascplotvect(sel.dat) ""
  if {$killcur} {
    ascplot_showdata 0
    set ascplotvect(curset) -1
    set ascplotvect(curfile) ""
    $ascplotvect(dataname) configure -variable apv.dummy
    set ascplotvect(sel.dep) ""
    set ascplotvect(sel.var) ""
    set ew $ascplotvect(indvname)
    $ew configure -state normal
    $ew delete 0 end
    $ew configure -state disabled
    $ascplotvect(depvname) delete 0 end
    $ascplotvect(varsname) delete 0 end
  }
  foreach i $killlist {
    set ascplotvect(rows.$i) 0
    set aname "$ascplotvect(array.$i)"
    global $aname
    unset $aname
    set fn [lindex $ascplotvect(set.$i) 0]
    incr ascplotvect(filecnt.$fn) -1
  }
  foreach i $killsets {
      ascplot_parse_file $i
      ascplot_drawsets
  }
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_mergedata {} {
  error "ascplot_mergedata unimplemented"
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_selplotter {args} {
  global ascplotvect
  if {[lsearch -exact $ascplotvect(plotterchoices) [lindex $args 0] ] > -1} {
    set ascplotvect(plotter) "[lindex $args 0]"
  } else {
    error "Unsupported plotter."
  }
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_writegraph {} {
  global ascplotvect
  switch $ascplotvect(plotter) {
    {xgraph} -
    {xmgr} - 
    {gnuplot} {
      ascplot_write$ascplotvect(plotter)
    }
    default {
      error "Can't write unsupported graph type."
    }
  }
}
#
# proc ascplot_writexgraph
#---------------------------------------------------------------------
# generates a plot file and saves to a user specified place
#---------------------------------------------------------------------
proc ascplot_writexgraph {} {
  global ascplotvect
  set pattern "*.xgraph"
  set defaultname $ascplotvect(xgraph_filename)
  set filename [tk_getSaveFile \
    -defaultextension .xgraph \
    -filetypes $ascplotvect(filetypes) \
    -initialdir $defaultname \
    -parent .ascplot \
    -title {Write to which data file?}]

  if {$filename == ""} {
    return
  } else {
    set ascplotvect(xgraph_filename) $filename
    set file "[ascplot_genxgraph]"
    if {$file != "" && $file != "err"} {
      file rename -- $file $filename
      puts stdout "Wrote $filename"
    }
  }
}
#
# proc ascplot_writegnuplot
#---------------------------------------------------------------------
# generates a plot file and saves to a user specified place
#---------------------------------------------------------------------
proc ascplot_writegnuplot {} {
  error "ascplot_writegnuplot: options page not written yet."
}
#
# proc ascplot_writexmgr
#---------------------------------------------------------------------
# generates a plot file and saves to a user specified place
#---------------------------------------------------------------------
proc ascplot_writexmgr {} {
  error "ascplot_writexmgr: options page not written yet."
}
#
# proc ascplot_viewgraph {}
#---------------------------------------------------------------------
# generate and invoke x program on a graph file.
#---------------------------------------------------------------------
proc ascplot_viewgraph {} {
  global ascplotvect
  set plotcmd ""
  set file ""
  switch $ascplotvect(plotter) {
    {xgraph} {
      global ascplot_xgraphvect
      set plotcmd $ascplot_xgraphvect(Command)
      set file "[ascplot_genxgraph]"
    }
    default {
      error "$ascplotvect(plotter) unsupported"
    }
  }
  if {$file != "" && $file != "err"} {
    if {[catch {eval "exec" $plotcmd $file &} msg]} {
      error "Invoking $plotcmd $filename failed. $msg"
    }
  } else {
    error "Unable to generate graph file."
  }
}

#
# proc ascplot_genxgraph {}
#---------------------------------------------------------------------
# generate an xgraph file in /tmp or TMPDIR and return the filename
#---------------------------------------------------------------------
proc ascplot_genxgraph {} {
  global ascplotvect ascplot_xgraphvect env
  set sdir ""
  if {[info exists env(TMPDIR)]} {
    set sdir $env(TMPDIR)
  } else {
    set sdir "/tmp"
  }
  set file [FileUniqueName "$sdir/ascdiscode"]
  set fid [open $file w+]
  ascplot_xgraphpreamble $fid
  set cs $ascplotvect(curset)
  set aname $ascplotvect(array.$cs)
  global $aname
  set lmax [$ascplotvect(depvname) size]
  set nr $ascplotvect(rows.$cs)
  set ew $ascplotvect(indvname)
  $ew configure -state normal
  set line "[$ew get]"
  $ew configure -state disabled
  set x "[string trim [lindex $line 0] ()]"

  for {set i 0} {$i < $lmax} {incr i} {
    set line "[$ascplotvect(depvname) get $i]"
    set c "[string trim [lindex $line 0] ()]"
    puts $fid "\n\"$ascplotvect(legend$i)\""
    for {set r 0} {$r < $nr} {incr r} {
      catch {puts $fid "[set ${aname}($r,$x)] [set ${aname}($r,$c)]"}
    }
  }
  close $fid
  return $file
}
proc ascplot_xgraphpreamble {fid} {
  global ascplot_xgraphvect ascplotvect
  puts $fid "BarGraph: $ascplot_xgraphvect(BarGraph)"
  puts $fid "NoLines: $ascplot_xgraphvect(NoLines)"
  puts $fid "LogX: $ascplot_xgraphvect(LogX)"
  puts $fid "LogY: $ascplot_xgraphvect(LogY)"
  puts $fid "BoundBox: $ascplot_xgraphvect(BoundBox)"
  puts $fid "Ticks: $ascplot_xgraphvect(Ticks)"
  puts $fid "Markers: $ascplot_xgraphvect(Markers)"
  puts $fid "PixelMarkers: $ascplot_xgraphvect(PixelMarkers)"
  puts $fid "LargePixels: $ascplot_xgraphvect(LargePixels)"
  puts $fid "StyleMarkers: $ascplot_xgraphvect(StyleMarkers)"
  puts $fid "ReverseVideo: $ascplot_xgraphvect(ReverseVideo)"
  puts $fid "Debug: $ascplot_xgraphvect(Debug)"
  puts $fid "XHighLimit: $ascplot_xgraphvect(XHighLimit)"
  puts $fid "YHighLimit: $ascplot_xgraphvect(YHighLimit)"
  puts $fid "XLowLimit: $ascplot_xgraphvect(XLowLimit)"
  puts $fid "YLowLimit: $ascplot_xgraphvect(YLowLimit)"
  puts $fid "BarWidth: $ascplot_xgraphvect(BarWidth)"
  puts $fid "BarBase: $ascplot_xgraphvect(BarBase)"
  puts $fid "BorderSize: $ascplot_xgraphvect(BorderSize)"
  puts $fid "GridSize: $ascplot_xgraphvect(GridSize)"
  puts $fid "LineWidth: $ascplot_xgraphvect(LineWidth)"
  puts $fid "ZeroWidth: $ascplot_xgraphvect(ZeroWidth)"
  puts $fid "GridStyle: $ascplot_xgraphvect(GridStyle)"
  puts $fid "ZeroStyle: $ascplot_xgraphvect(ZeroStyle)"
  puts $fid "TitleText: $ascplotvect(plottitle)"
  puts $fid "XUnitText: $ascplotvect(xtitle)"
  puts $fid "YUnitText: $ascplotvect(ytitle)"
  puts $fid "LabelFont: $ascplot_xgraphvect(LabelFont)"
  puts $fid "TitleFont: $ascplot_xgraphvect(TitleFont)"
  puts $fid "Device: $ascplot_xgraphvect(Device)"
  puts $fid "FileOrDev: $ascplot_xgraphvect(FileOrDev)"
  puts $fid "Disposition: $ascplot_xgraphvect(Disposition)"
  puts $fid "Border: $ascplot_xgraphvect(Border)"
  puts $fid "ZeroColor: $ascplot_xgraphvect(ZeroColor)"
  puts $fid "0.Style: $ascplot_xgraphvect(Style0)"
  puts $fid "0.Color: $ascplot_xgraphvect(Color0)"
  puts $fid "1.Style: $ascplot_xgraphvect(Style1)"
  puts $fid "1.Color: $ascplot_xgraphvect(Color1)"
  puts $fid "2.Style: $ascplot_xgraphvect(Style2)"
  puts $fid "2.Color: $ascplot_xgraphvect(Color2)"
  puts $fid "3.Style: $ascplot_xgraphvect(Style3)"
  puts $fid "3.Color: $ascplot_xgraphvect(Color3)"
  puts $fid "4.Style: $ascplot_xgraphvect(Style4)"
  puts $fid "4.Color: $ascplot_xgraphvect(Color4)"
  puts $fid "5.Style: $ascplot_xgraphvect(Style5)"
  puts $fid "5.Color: $ascplot_xgraphvect(Color5)"
  puts $fid "6.Style: $ascplot_xgraphvect(Style6)"
  puts $fid "6.Color: $ascplot_xgraphvect(Color6)"
  puts $fid "7.Style: $ascplot_xgraphvect(Style7)"
  puts $fid "7.Color: $ascplot_xgraphvect(Color7)"
}
#
# ascplot_credits {}
#---------------------------------------------------------------------
# put up a window about whodunnit
#---------------------------------------------------------------------
proc ascplot_credits {} {
  global ascplotvect
  OKText .ascplot_credit "
Plot Widget for feeding various graph programs
from a standard tabular input file.
Supports basic data manipulations in a spreadsheet-like fashion.

By Benjamin A Allan, August 1995
Carnegie Mellon University
Engineering Design Research Center
ASCEND project. (ballan@cs.cmu.edu)
ascend+bb@cs.cmu.edu.

Thanks also to the authors of
Tcl/Tk, xgraph, and tkTable." "Plot Credits" 0 $ascplotvect(textfont) \
[getpos .ascplot] 12 40
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_sources {} {
  global ascplotvect
  OKText .ascplot_sources "
This widget and associated tools are part of the ASCEND modeling
environment." "Plot availability" 0 $ascplotvect(textfont) \
[getpos .ascplot] 8 40
}
#
# proc ascplot_selindependentleft {args}
#---------------------------------------------------------------------
# moves the selected variable from var box to independent entry
# if args is "discard" forgets the old variable info
#---------------------------------------------------------------------
proc ascplot_selindependentleft {args} {
  global ascplotvect
  set ew $ascplotvect(indvname)
  if {![info exists ascplotvect(sel.var)] || $ascplotvect(sel.var)==""} {
    return
  }
  set new [lindex $ascplotvect(sel.var) 0]
  set dlist $new
  set new [$ascplotvect(varsname) get $new]
  $ew configure -state normal
  set old "[$ew get]"
  if {$old != ""} {
    if {[info exists args] && $args != "discard"} {
      $ascplotvect(varsname) insert end $old
    }
    $ew delete 0 end
  }
  $ew insert end $new
  $ew configure -state disabled
  $ascplotvect(varsname) delete $dlist $dlist
  set ascplotvect(sel.var) ""
  $ascplotvect(varsname) selection clear 0 end
}
#
# proc ascplot_selindependentright {}
#---------------------------------------------------------------------
# moves the selected variable from dependent box to independent entry
#---------------------------------------------------------------------
proc ascplot_selindependentright {} {
  global ascplotvect
  set ew $ascplotvect(indvname)
  if {![info exists ascplotvect(sel.dep)] || $ascplotvect(sel.dep)==""} {
    return
  }
  set new [lindex $ascplotvect(sel.dep) 0]
  set dlist $new
  set new [$ascplotvect(depvname) get $new]
  $ew configure -state normal
  set old "[$ew get]"
  if {$old != ""} {
    $ascplotvect(depvname) insert end $old
    $ew delete 0 end
  }
  $ew insert end $new
  $ew configure -state disabled
  $ascplotvect(depvname) delete $dlist $dlist
  set ascplotvect(sel.dep) ""
  $ascplotvect(depvname) selection clear 0 end
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_grill {} {
  global ascplotvect
  ascParPage ascplot_$ascplotvect(plotter)vect [getpos .ascplot] 1
}
#
# ascplot_insert {rc args}
#---------------------------------------------------------------------
# inserts a row or column after ascertaining the sort of
# stuff to insert and calculating it if needed.
#---------------------------------------------------------------------
proc ascplot_insert {rc args} {
  global ascplotvect
  global ascplot_ssvect
  # if no args, pop up dialog initial else process dialog
  if {![info exists args] || $args == ""} {
    if {$rc=="row"} {
      set type Row
      set ascplot_ssvect(after.hi) $ascplotvect(rows.$ascplotvect(curset))
    } else {
      set ascplot_ssvect(after.hi) $ascplotvect(cols.$ascplotvect(curset))
      set type Column
    }
    set ascplot_ssvect(after.label) "Insert after $type"
    set ascplot_ssvect(formula) "$ascplot_ssvect(${rc}formula)"
    set ascplot_ssvect(last.label) "Insert at end (overrides $type)"
    set ascplot_ssvect(dtype.label) "$type type"
    set ascplot_ssvect(cancel) 0
    set ascplot_ssvect(whenokcommand) "ascplot_insert $rc ok"
    ascParPage ascplot_ssvect [setpos .ascplot 20 20] 1
    return
  } else {
    set aname $ascplotvect(array.$ascplotvect(curset))
    global $aname
    if {$ascplot_ssvect(cancel)} {
      return
    }
    if {$rc=="row"} {
      set new $ascplotvect(rows.$ascplotvect(curset))
      if {$ascplot_ssvect(last) || $ascplot_ssvect(after) > $new} {
        set ascplot_ssvect(after) [expr $new -1]
      }
      set new [expr $ascplot_ssvect(after)+1]
      ascplot_insertrow $ascplot_ssvect(after)
      set ${aname}($new,-1) "Row $new"
      set ${aname}($new,-2) $ascplot_ssvect(dtype)
      if {$ascplot_ssvect(dtype)=="formula"} {
        set ${aname}($new,-2) $ascplot_ssvect(rowformula)
        ascplot_calcrow $new
      } else {
        set max $ascplotvect(cols.$ascplotvect(curset))
        for {set c 0} {$c <= $max } {incr c} {
          set ${aname}($new,$c) ""
        }
      }
    }
    if {$rc=="col"} {
      set new $ascplotvect(cols.$ascplotvect(curset))
      if {$ascplot_ssvect(last) || $ascplot_ssvect(after) >= $new} {
        set ascplot_ssvect(after) [expr $new -1]
      }
      set new [expr $ascplot_ssvect(after) +1]
      ascplot_insertcol $ascplot_ssvect(after)
      set ${aname}(-7,$new) $ascplot_ssvect(dtype)
      set ${aname}(-6,$new) "NewCol$ascplot_ssvect(newcolnum)"
      set ${aname}(-5,$new) "NewCol$ascplot_ssvect(newcolnum)"
      set ${aname}(-4,NewCol$ascplot_ssvect(newcolnum)) $new
      set ${aname}(-3,$new) "Col $new"
      set ${aname}(-2,$new) "NewCol$ascplot_ssvect(newcolnum)"
      set ${aname}(-1,$new) "NewCol$ascplot_ssvect(newcolnum)"
      incr ascplot_ssvect(newcolnum)
      if {$ascplot_ssvect(dtype)=="formula"} {
        set ${aname}(-7,$new) $ascplot_ssvect(colformula)
        ascplot_calccol $new
      } else {
        set max $ascplotvect(rows.$ascplotvect(curset))
        for {set r 0} {$r <= $max } {incr r} {
          set ${aname}($r,$new) ""
        }
      }
    }
  }
  update
}
#
# proc ascplot_calccol {num}
#---------------------------------------------------------------------
# calculates and sets the col num cells using the formula stored in
# the top edge of the data array.
# works top down
#---------------------------------------------------------------------
proc ascplot_calccol {num} {
  global ascplotvect
  set cs $ascplotvect(curset)
  set an $ascplotvect(array.$cs)
  global $an
  set ann "\$$an"
  set n $ascplotvect(rows.$cs)
  set f ""
  set f1 ""
  if {"[set ${an}(-7,$num)]" == "data"} {
    error "No formula has been defined for this column."
  } else {
    set f "[set ${an}(-7,$num)]"
  }
  if { [catch {
         set c $num
         for {set r 0} {$r < $n} {incr r} {
           regsub -all {(A\()([^,]*)(,)([^)]*)(\))} "$f" \
             {ANAME([expr \2],[expr \4])} f1
           regsub -all ANAME "$f1" $ann f
           set q "ERR"
           catch {set q "[expr $f]"}
           set ${an}($r,$c) $q
         }
               } m]} {
     error "Enormous error in calculating column."
  }
}
#
# proc ascplot_calcrow {num}
#---------------------------------------------------------------------
# calculates and sets the row num cells using the formula stored in
# the left edge of the data array.
# works left 2 right
#---------------------------------------------------------------------
proc ascplot_calcrow {num} {
  global ascplotvect
  set cs $ascplotvect(curset)
  set an $ascplotvect(array.$cs)
  global $an
  set ann "\$$an"
  set n $ascplotvect(cols.$cs)
  set f ""
  set f1 ""
  if {"[set ${an}($num,-2)]" == "data"} {
    error "No formula has been defined for this row."
  } else {
    set f "[set ${an}($num,-2)]"
  }
  if { [catch {\
         set r $num
         for {set c 0} {$c < $n} {incr c} {
           regsub -all {(A\()([^,]*)(,)([^)]*)(\))} "$f" \
             {ANAME([expr \2],[expr \4])} f1
           regsub -all ANAME "$f1" $ann f
           set q "ERR"
           catch {set q "[expr $f]"}
           set ${an}($r,$c) $q
         }
               } m]} {
     error "Enormous error in calculating row."
  }
}

#
# proc ascplot_ssbinds {}
#---------------------------------------------------------------------
# puts the help message on table bindings
#---------------------------------------------------------------------
proc ascplot_ssbinds {} {
  global ascplotvect
  OKText .ascplot_ssbinds \
"   The Table class bindings that give the following default behaviour:

\[1\]  Clicking the mouse button in a cell moves the selection to that cell.

\[2\]  The left, right, up and down arrows move the selected cell.

\[3\]  Control-leftarrow and Control-rightarrow move the insertion cursor within the cell.

\[4\]  Backspace deletes the character before the insertion cursor.

\[5\]  Delete deletes the character after the insertion cursor." \
"Table Widget Bindings" 0 $ascplotvect(textfont) [getpos .ascplot] 15 70
}

#
# proc ascplot_sshelp {}
#---------------------------------------------------------------------
# puts the help message for ss use.
#---------------------------------------------------------------------
proc ascplot_sshelp {} {
  global ascplotvect
  OKText .ascplot_sshelp \
  "With the window you define a new column/row of data to be added to the data
set currently displayed. If you wish to not add anything,
you can turn on the Forget button and the add will be canceled when you hit
OK.\n
The column/row added will be inserted where specified unless you turn on the
Insert at end button.\n
The column/row added can be data entered manually,(edit the table
yourself) or by a Tcl-like expression as specified by the Column/Row type.\n
The formula is spreadsheet-like in that we allow absolute addressing and a
form of relative cell addressing.\n
The calculations are not dynamic, that is they are only evaluated when you
explicitly tell us to. We do not intend to write a real spreadsheet as nobody
wants a chapter in our thesis about spreadsheet implementation.\n
Tcl does double precision, integer, and logical arithmetic
just like C. The Tcl math operators are listed at the end.\n
  WARNING:  All data in Tcl is stored as a string. If there is not
a decimal point followed by a nonzero digit appearing in the cell,
Tcl will assume the number is an integer. This may lead to unexpected
results when division is used. Tcl does follow C-like (FORTRAN-like)
rules when determining the math type of the result of a subexpression.
So, 4/5.1 will return 0.78... while 4/5 returns zero since both are int.\n
Not all the ASCEND math operators are supported by Tcl.\n
Cell addressing examples (A(i,j) is the ijth cell value):\n
A(1,2) --- this is absolute addressing.\n
A(\$r,\$c) --- this is relative addressing. \$r and \$c are the row and column
numbers of the cell in which the calculation is being done.
A(\$r,1) --- this is mixed addressing. (value in cell in col 2 of this row)\n
A(\$r-2,\$c-1) --- this is calculated addressing (value 2 rows up and 1 col left).
Any integer expression is permitted in the subscripts of A.\n
CELL ADDRESSES are those AFTER the row or column is inserted. Take this into
account when writing your formulae.\n
Tcl operators: (all have the same basic semantics as in C)\n
man math in unix should tell you what these mean if you have not C book.\n\n
(,),+,-,*,/,%\n
~,^,!,&,|,x?y:z\n
<<,>>,<,>,<=,>=,==,!=,&&,||\n
asin() acos() atan() atan2()\n
sin() cos() tan()\n
exp() sinh() cosh() tanh()\n
pow() log() log10()\n
sqrt() hypot() \n
ceil() floor() round() fmod() abs()\n
double() int() (these are casting operators)\n\n
Last updated ballan@cs.cmu.edu 8/19/95." "Creating data" 0 \
$ascplotvect(textfont) [getpos .ascplot] 20 70
}
#
# proc ascplot_insertcol {after}
#---------------------------------------------------------------------
# Insert an empty column after the column number given
# and fix up all the crap that shifts when columns move.
# after should not be >= the number of columns in the data.
#---------------------------------------------------------------------
proc ascplot_insertcol {after} {
  global ascplotvect
  global ascplot_ssvect
  set cset $ascplotvect(curset)
  if {$after == $ascplotvect(cols.$cset) -1} {
    incr ascplotvect(cols.$cset)
    $ascplotvect(dataname) configure \
      -cols [expr $ascplotvect(cols.$cset) + 1]
  } else {
    set aname $ascplotvect(array.$cset)
    global $aname
    set right $ascplotvect(cols.$cset)
    set nr $ascplotvect(rows.$cset)
    # $ascplotvect(dataname) batch on // equivalent in 2.x?
 # move data, skipping anything in row -4 which is not col indexed
    for {set c [expr $right -1]} {$c > $after} {incr c -1} {
      for {set r -7} {$r < $nr} {incr r} {
        if {$r != -4} {
          catch {set ${aname}($r,$right) "[set ${aname}($r,$c)]"}
        }
      }
      set ${aname}(-3,$right) "Col $right"
      incr right -1
    }
 # fix depv
    set ll [$ascplotvect(depvname) size]
    for {set li 0} {$li < $ll} {incr li} {
      set line "[$ascplotvect(depvname) get $li]"
      set i "[string trim [lindex $line 0] ()]"
      incr i
      set line \
        "$i) [set ${aname}(-2,$i)] [set ${aname}(-5,$i)] [set ${aname}(-1,$i)]"
      $ascplotvect(depvname) delete $li $li
      $ascplotvect(depvname) insert $li $line
    }
 # fix vars
    set ll [$ascplotvect(varsname) size]
    for {set li 0} {$li < $ll} {incr li} {
      set line "[$ascplotvect(varsname) get $li]"
      set i "[string trim [lindex $line 0] ()]"
      incr i
      set line \
        "$i) [set ${aname}(-2,$i)] [set ${aname}(-5,$i)] [set ${aname}(-1,$i)]"
      $ascplotvect(varsname) delete $li $li
      $ascplotvect(varsname) insert $li $line
    }
    set ew $ascplotvect(indvname)
    $ew configure -state normal
    set line "[$ascplotvect(indvname) get]"
    if {$line != ""} {
      set i "[string trim [lindex $line 0] ()]"
      if {$i > $after} {
        incr i
        set line \
         "$i) [set ${aname}(-2,$i)] [set ${aname}(-5,$i)] [set ${aname}(-1,$i)]"
        $ew delete 0 end
        $ew insert end $line
      }
    }
    $ew configure -state disabled
 # fix vid2c
    set right $ascplotvect(cols.$cset)
    for {set c [expr $after +2]} {$c <= $right} {incr c} {
      set vid "[set ${aname}(-2,$c)]"
      set ${aname}(-4,$vid) "$c"
    }
    incr ascplotvect(cols.$cset)
    $ascplotvect(dataname) configure \
      -cols [expr $ascplotvect(cols.$cset) + 1]
    # $ascplotvect(dataname) batch off // equivalent in 2.x?
 # redraw everything dependent
    ascplot_setlegends
    update idletasks
  }
}
#
# proc ascplot_insertrow {after}
#---------------------------------------------------------------------
# Insert an empty row after the number given
# and fix up all the crap that shifts when rows move.
# after should not be >= the number of rows in the data.
#---------------------------------------------------------------------
proc ascplot_insertrow {after} {
  global ascplotvect
  global ascplot_ssvect
  set cset $ascplotvect(curset)
  if {$after == $ascplotvect(rows.$cset) -1} {
    incr ascplotvect(rows.$cset)
    $ascplotvect(dataname) configure \
      -rows [expr $ascplotvect(rows.$cset) + 3]
  } else {
    set aname $ascplotvect(array.$cset)
    global $aname
    set bot $ascplotvect(rows.$cset)
    set nc $ascplotvect(cols.$cset)
    # $ascplotvect(dataname) batch on // equivalent in 2.x?
    for {set r [expr $bot -1]} {$r > $after} {incr r -1} {
      for {set c -2} {$c < $nc} {incr c} {
        catch {set ${aname}($bot,$c) "[set ${aname}($r,$c)]"}
      }
      set ${aname}($bot,-1) "Row $bot"
      incr bot -1
    }
    incr ascplotvect(rows.$cset)
    $ascplotvect(dataname) configure \
      -rows [expr $ascplotvect(rows.$cset) + 3]
    # $ascplotvect(dataname) batch off // equivalent in 2.x?
    update idletasks
  }
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------

#---------------------------------------------------------------------
# ASCPLOT UTILITY FUNCTIONS (INTERNAL)
#---------------------------------------------------------------------
#
# proc ascplot_open {}
#---------------------------------------------------------------------
# opens the ascplot window if not already up.
#---------------------------------------------------------------------
proc ascplot_open {} {
  global ascplotvect
  if {[catch {package require Tktable 2.5} err]} {
    error \
      "ASC_PLOT cannot locate the tkTable package.\ninternal message: $err"
  }
  if {![winfo exists .ascplot]} {
    ShowWindow.ascplot
    ascplot_bindwindow
    set ascplotvect(winopen) 1
  }
}

#
# proc ascplot_dook {}
#---------------------------------------------------------------------
# the ok button.
#---------------------------------------------------------------------
proc ascplot_dook {} {
  global ascplot_xgraphvect
  View_Save_SpecialWindow_Values ascplot
  ascplot_showtitles close
  catch {ascParPageClose ascplot_xgraphvect}
  ascplot_destroy "none"
}

#
# proc ascplot_destroy {all}
#---------------------------------------------------------------------
# nukes everything to do with the ascplot except the global array
# ascplotvect. if $all =="all" nuke ascplotvect too.
#---------------------------------------------------------------------
proc ascplot_destroy {all} {
  global ascplotvect
  set na $ascplotvect(nsets)
  for {set i 0} {$i < $na} {incr i} {
    global $ascplotvect(array.$i)
    catch {unset $ascplotvect(array.$i)}
    set ascplotvect(rows.$i) 0
    set ascplotvect(cols.$i) 0
  }
  set ascplotvect(curset) -1
  set ascplotvect(nsets) 0
  set ascplotvect(nfiles) 0
  if {"$all"=="all"} {
    unset ascplotvect
  }
  catch {destroy .ascplot}
  set ascplotvect(winopen) 0
}

#
# proc ascplot_get_array_name {}
#---------------------------------------------------------------------
# makes up a unique array name and returns it.
# doesn't check for uniqueness, but what are the odds?
#---------------------------------------------------------------------
proc ascplot_get_array_name {} {
  global ascplotvect
  if {![info exist ascplotvect(setctr)]} {
    set ascplotvect(setctr) 0
  }
  set an "plotan$ascplotvect(setctr)uid"
  global $an
  set ${an}(isarray) 1
  incr ascplotvect(setctr)
  return $an
}
#
#
# proc ascplot_parse_file {filename}
#---------------------------------------------------------------------
# Here we use an idiots machine written in tcl which is linebased.
# This function takes care of the file io checking and figuring out
# where the data goes.
# We use a command called stringcompact which is in C for speed.
# returns < 0 if error. returns nfile (number of new file) OTHERWISE.
#---------------------------------------------------------------------
proc ascplot_parse_file {filename} {
  global ascplotvect
  set ascplotvect(filename) $filename
  if {![file exists $filename]} {
    ascplot_message "ERROR File Not Found: $filename"
    return -1
  }
  if {![file readable $filename]} {
    ascplot_message "ERROR File Not Readable: $filename"
    return -1
  }
  if {![file isfile $filename]} {
    ascplot_message "ERROR File Not Text: $filename"
    return -1
  }
  set nfile $ascplotvect(nfiles)
  for {set i 0} {$i < $nfile} {incr i} {
    if {"$filename"=="$ascplotvect(file.$i)" && $ascplotvect(filecnt.$i) > 0} {
      ascplot_message "ERROR File Already Loaded: $filename\nUse Update instead"
      return -1
    }
  }
  set fid [open $filename r]
  close $fid
  set fid [open $filename r]
  set nset $ascplotvect(nsets)
  set firstline i
  if {[gets $fid firstline]==-1} {
    ascplot_message "ERROR File Empty: $filename"
    return -1
  }
  set firstline "[stringcompact $firstline]"
  set ascplotvect(file.$nfile) $filename
  set ascplotvect(filecnt.$nfile) 0
  ascplot_parse_data $fid $nfile $nset $firstline
  close $fid
  incr ascplotvect(nfiles)
  return $nfile
}

#
# proc ascplot_parse_data {fid nfile nset firstline}
#---------------------------------------------------------------------
# the driver for ascplot_parse_data_stage
# calls itself until eof reached. eats 1 data set per call.
#---------------------------------------------------------------------
proc ascplot_parse_data {fid nfile nset fl} {
  global ascplotvect
  set notdone 1
  set emptyset 0
 # find set beginning
  set ascplotvect(set.$nset) "$nfile $nset"
  set c [ascplot_parse_data_stage $fid $nfile $nset top $fl]
  set ascplotvect(topline.$nset) "[lindex $c 1]"
  set ascplotvect(array.$nset) "[ascplot_get_array_name]"
  set aname "$ascplotvect(array.$nset)"
  global $aname
  if {[lindex $c 0]==-1} {
    set ascplotvect(rows.$nset) 0
    set ascplotvect(cols.$nset) 0
    return
  }
 # get next line
  if {[gets $fid fl]==-1} {
    set ascplotvect(rows.$nset) 0
    set ascplotvect(cols.$nset) 0
    return
  }
  set fl "[stringcompact $fl]"
 # eat titles
  set c [ascplot_parse_data_stage $fid $nfile $nset title $fl]
  if {[lindex $c 0]==1} {
    set ascplotvect(rows.$nset) 0
    set ascplotvect(cols.$nset) 0
    set emptyset 1
  }
  if {[lindex $c 0]==-1} {
    return
  }
  set fl "[lindex $c 1]"
 # fl should now be first line of column info
 # get col names
  if {!$emptyset} {
    set c [ascplot_parse_data_stage $fid $nfile $nset var $fl]
    if {[lindex $c 0]==-1} {
      set ascplotvect(rows.$nset) 0
      set ascplotvect(cols.$nset) 0
      return
    }
    set fl "[lindex $c 1]"
    if {![info exists ${aname}(-2,0)] || \
        "[lindex $fl 0]" != "[set ${aname}(-2,0)]" } {
      puts stderr "Insane dataset ($nset) found. skipping to next."
      set ascplotvect(rows.$nset) 0
      set ascplotvect(cols.$nset) 0
      set emptyset 1
    }
  }
 # eat  to shortid line or whitespace
  if {!$emptyset} {
    set c [ascplot_parse_data_stage $fid $nfile $nset names $fl]
    if {[lindex $c 0]==-1} {
      set ascplotvect(rows.$nset) 0
      return
    }
    set cc 0
    set lim $ascplotvect(cols.$nset)
    foreach i $fl {
      if {$cc >= $lim  || "$i" != "[set ${aname}(-2,$cc)]"} {
        puts stderr "Insane dataset ($nset) found. skipping to next."
        set ascplotvect(rows.$nset) 0
        set emptyset 1
        break
      }
      incr cc
    }
    if {[gets $fid fl]==-1} {
      set ascplotvect(rows.$nset) 0
      return
    }
    set fl "[stringcompact $fl]"
  }
 # eat --- line if present
  if {!$emptyset} {
    set c [ascplot_parse_data_stage $fid $nfile $nset spacer $fl]
    if {[lindex $c 0]==-1} {
      set ascplotvect(rows.$nset) 0
      return
    }
    if {[llength [lindex $c 1]] != $ascplotvect(cols.$nset)} {
      puts stderr "Insane dataset ($nset) found. skipping to next."
      set ascplotvect(rows.$nset) 0
      set emptyset 1
    } else {
      if {"[string range "[lindex $c 1]" 0 1]"=="--"} {
        if {[gets $fid fl]==-1} {
          set ascplotvect(rows.$nset) 0
          return
        }
        set fl "[stringcompact $fl]"
      }
    }
  }
 # eat data
  if {!$emptyset} {
    set c [ascplot_parse_data_stage $fid $nfile $nset data $fl]
    incr ascplotvect(filecnt.$nfile)
  }
  incr nset
  incr ascplotvect(nsets)
  if {[lindex $c 0]==-1} {
    set notdone 0
  } else {
    set fl "[lindex $c 1]"
  }
  if {$notdone} {
    ascplot_parse_data $fid $nfile $nset $fl
  }
}
#
# proc ascplot_parse_data_stage {fid nfile nset mode firstline}
#---------------------------------------------------------------------
# Heres the stupid little linebased parser in TCL
# fid must be an open file identifier
# nfile must be the number of the file we are working on in ascplotvect
# nset must be the number of the set we are attempting to create
# mode is one of several. they include:
#    top: we expect we are about to find a new set
#    title: we expect to be eating a title line (anything with a : in it)
#          that is not a top line
#    var: we expect to be eating a variable definition line which is tcl
#          list of shortname longname units
#    names: we expect to be eating a list of names matching the vars
#    spacer: we expect to be eating a list of --- matching the vars
#    data: we expect to be eating a list of numbers matching the vars
# firstline is the current first line of the file
#
# The return of ascplot_parse_data_stage is a list pair: code {firstline}
# code 0 means everything ok, code != 0 implies problem.
# Firstline is the line the parser advanced to which caused it to
# return.
# side effects: updates lots of stuff in ascplotvect
# with a successful set read
# codes:
# -1 : EOF reached unexpectedly
# algorithm states (1pass) {the error exits not included}
# top: get DATASET head
#       exit condition is line starting with DATASET found
# title: skip titles but backup to top if DATASET seen again
#       exit condition is nonblank line w/o a : in it
# var: get list of column header triples {shortid} {longid} {units}
#       exit condition is nonblank line without triplet or with shortid
#       repeating a previous shortid or apparent shortid starting with --
# names: expect a line of shortid matching var list
# spacer: optional expect a line with nvars elements looking like ---
# data: get a data line nvars long
#       exit condition is nonblank line w/o nvars items,
#       or a nonnumeric item or EOF
# All lines are hit with stringcompact before analysis, so extra
# whitespace/tabs make no difference. input firstline should be
# compacted.
#---------------------------------------------------------------------
proc ascplot_parse_data_stage {fid nf ns mo fl} {
  global ascplotvect
 # puts "$mo : $fl"
  switch $mo {
    {data} {
 #    puts "apd data" ;#comment normally
      set cc $ascplotvect(cols.$ns)
 #    puts "cc= $cc" ;#comment normally
      set rc 0
      set notdone 1
      set aname $ascplotvect(array.$ns)
      global $aname
      while {$notdone > 0} {
 #      puts "$fl" ;#comment normally
        if {"[string range $fl 0 6]"=="DATASET"} {
          set notdone 0
          break
        }
        set w "[llength $fl]"
 # here we want a 3 Case equivalent set of ifs
   # Case cc==w parseline, Case cc=0 eat line, default punt
        if {$w==$cc} {
          set ic 0
          set ${aname}($rc,-2) "data"
          set ${aname}($rc,-1) "Row $rc"
          foreach i $fl {
            set ${aname}($rc,$ic) "$i"
            incr ic
          }
          incr rc
          if {[gets $fid fl]==-1} {
            set notdone -1
          }
          set fl "[stringcompact $fl]"
        } else {
          if {$w != 0} { 
            # default case
            set notdone 0
          } else {
            # empty line case
            if {[gets $fid fl]==-1} {
              set notdone -1
            }
            set fl "[stringcompact $fl]"
          }
        }
      # endwhile
      }
      set ascplotvect(rows.$ns) $rc
      return [list $notdone $fl]
    # end data case
    }
    {var} {
 #    puts "apd var $fl"
      set ascplotvect(cols.$ns) 0
      set cc 0
      set firstid ""
      global $ascplotvect(array.$ns)
      set aname $ascplotvect(array.$ns)
      if {"$fl"!=""} {
        if {[llength $fl]!=3} {
          return [list 1 $fl]
        } else {
          set vid [lindex $fl 0]
          set ${aname}(-7,$cc) "data"
          set ${aname}(-6,$cc) "[lrange $fl 1 2]"
          set ${aname}(-5,$cc) "[lindex $fl 1]"
          set ${aname}(-4,$vid) "$cc"
          set ${aname}(-3,$cc) "Col $cc"
          set ${aname}(-2,$cc) "$vid"
          set ${aname}(-1,$cc) "[lindex $fl 2]"
          set firstid $vid
          incr cc
        }
      }
      while {[gets $fid fl]!=-1} {
        set fl "[stringcompact $fl]"
 #      puts "varwhile $fl"
        if {"$fl"==""} {
          continue
        }
        set vid "[lindex $fl 0]"
        if {[llength $fl]!=3 || "$vid"=="$firstid" || \
          "--"=="[string range $vid 0 1]"} {
          set ascplotvect(cols.$ns) $cc
          return [list 0 $fl]
        } else {
          set ${aname}(-7,$cc) "data"
          set ${aname}(-6,$cc) "[lrange $fl 1 2]"
          set ${aname}(-5,$cc) "[lindex $fl 1]"
          set ${aname}(-4,$vid) "$cc"
          set ${aname}(-3,$cc) "Col $cc"
          set ${aname}(-2,$cc) "$vid"
          set ${aname}(-1,$cc) "[lindex $fl 2]"
          incr cc
        }
      }
      set ascplotvect(cols.$ns) $cc
      return [list 0 $fl]
    # END var case
    }
    {title} {
 #    puts "apd title"
      set skip 0
      set data 0
      while {[string first : $fl] > -1 && \
             "[string range $fl 0 6]"!="DATASET"} {
        if {[gets $fid fl]==-1} {
          return {-1 {}}
        }
        set fl "[stringcompact $fl]"
        incr skip
      }
 #    puts "skipped $skip title lines"
      if {"[string range $fl 0 6]"!="DATASET"} {
        return "[list 0 $fl]"
      } else {
        return "[list 1 $fl]"
      }
    # END title case
    }
    {top} {
 #    puts "apd top"
      set ascplotvect(rows.$ns) 0
      set skip 0
      while {"[string range $fl 0 6]"!="DATASET"} {
        if {[gets $fid fl]==-1} {
          return {-1 {}}
        }
        set fl "[stringcompact $fl]"
        incr skip
      }
 #    puts "skipped $skip lines"
      return [list 0 $fl]
    # END top case
    }
    {names} -
    {spacer} {
 #    puts "apd $mo"
      while {"$fl"==""} {
        if {[gets $fid fl]==-1} {
          return {-1 {}}
        }
        set fl "[stringcompact $fl]"
      }
      return [list 0 $fl]
    # END names/spacer cases
    }
    default {
      puts "apd unknown mode"
      return {1 {}}
    }
  }
}

#
# proc ascplot_drawsets {}
#---------------------------------------------------------------------
# redraws the listbox for data sets. deleted/empty data sets are
# not shown. updates indexing arrays which convert list# to set # and vv.
# works with data sets indexed starting at 0
#---------------------------------------------------------------------
proc ascplot_drawsets {} {
  global ascplotvect
  set ascplotvect(list2set) ""
  set nsets $ascplotvect(nsets)
  set lc 0
  $ascplotvect(setsname) delete 0 end
  for {set i 0} {$i < $nsets} {incr i} {
    if {$ascplotvect(rows.$i) > 0} {
      lappend ascplotvect(list2set) $i
      set j "[lindex $ascplotvect(set.$i) 0]"
      set line \
      "($i) [file tail $ascplotvect(file.$j)] \{$ascplotvect(topline.$i)\}"
      $ascplotvect(setsname) insert end $line
    }
  }
}
#
# proc ascplot_setlegends {}
#---------------------------------------------------------------------
# updates the legends from the array names for them based on the current
# set of dependent variables.
#---------------------------------------------------------------------
proc ascplot_setlegends {} {
  global ascplotvect
  set aname $ascplotvect(array.$ascplotvect(curset))
  global $aname
  global ascplot_xgraphvect
  if {[winfo depth .]==1} {
    set mono 1
  } else {
    set mono 0
  }
  set pp ""
  catch {set pp "$ascplotvect(plotter)"}
  set lmax [$ascplotvect(depvname) size]
  set ascplotvect(ncurves) $lmax
  set ascplotvect(namelist) "$ascplotvect(baselist)"
  set ascplotvect(cancellable) 0
  set cmax 0
  switch $pp {
    {xgraph} {
      if {$mono} {
        set cmax 8
      } else {
        set cmax 64
      }
    }
    default {
      error "No plotter selected"
    }
  }
  for {set i 0} {$i < $lmax} {incr i} {
    set line "[$ascplotvect(depvname) get $i]"
    set col "[string trim [lindex $line 0] ()]"
    set ascplotvect(legend$i) "[set ${aname}(-6,$col)]"
    set ascplotvect(legend$i.label) "Column $col legend"
    lappend ascplotvect(namelist) "legend$i"
  }
  if $ascplotvect(uplegend) {
    ascplot_showtitles open
  }
  update
  if {$lmax > $cmax} {
    error \
      "Number of dependent variables ($lmax) > distinguishable curves ($cmax)"
  }
}
#
# proc ascplot_message {m}
#---------------------------------------------------------------------
# sets the plot message to m, clearing any previous
#---------------------------------------------------------------------
proc ascplot_message {m} {
  global ascplotvect
  $ascplotvect(textname) delete 1.0 end
  $ascplotvect(textname) insert end "HINT:\n"
  $ascplotvect(textname) insert end $m
}

#
# proc ascplot_message_append {m}
#---------------------------------------------------------------------
# appends the plot message with \n$m
#---------------------------------------------------------------------
proc ascplot_message_append {m} {
  global ascplotvect
  $ascplotvect(textname) insert end "\n$m"
}

#
# proc ascplot_bindwindow {}
#---------------------------------------------------------------------
# sets up bindings on .ascplot widgets.
# sets up defaults on xgraphvect
#---------------------------------------------------------------------
proc ascplot_bindwindow {} {
  global ascplotvect

  set ascplotvect(filetypes)  {
    {{ASCII data} {.dat} }
    {{GNUplot} {.gnuplot .gpl} }
    {{xgraph} {.xgraph .xg} }
    {{ASCII data} {.dat} }
    {{Most} {.*} }
    {{All} {*} }
  }

  # kill all the selection exports
  $ascplotvect(varsname) configure -exportselection 0
  $ascplotvect(depvname) configure -exportselection 0
  $ascplotvect(indvname) configure -exportselection 0
  $ascplotvect(setsname) configure -exportselection 0
  $ascplotvect(textname) configure -exportselection 0

  # bind to raise window from label or messagebox
  bind $ascplotvect(textname) <B1-ButtonRelease> {raise .ascplot}

  bind $ascplotvect(depvname) <Any-Leave> {
    global ascplotvect
    set ascplotvect(sel.dep) "[%W curselection]"
  }
  bind $ascplotvect(setsname) <Any-Leave> {
    global ascplotvect
    set ascplotvect(sel.dat) "[%W curselection]"
  }
  bind $ascplotvect(setsname) <Double-1> {
    %W select set [%W nearest %y]
    set ascplotvect(sel.dat) "[%W curselection]"
    ascplot_seldataset
  }
  bind $ascplotvect(varsname) <Any-Leave> {
    global ascplotvect
    set ascplotvect(sel.var) "[%W curselection]"
  }
  bind $ascplotvect(dataname) <Any-Enter> {
    ascplot_widget_message %W
  }
  bind $ascplotvect(setsname) <Any-Enter> {
    ascplot_widget_message %W
  }
  bind $ascplotvect(varsname) <Any-Enter> {
    ascplot_widget_message %W
  }
  bind $ascplotvect(depvname) <Any-Enter> {
    ascplot_widget_message %W
  }
  bind $ascplotvect(indvname) <Any-Enter> {
    ascplot_widget_message %W
  }
  bind .ascplot.top_frm.varfrm.varsel.ctrbtns.btop <Any-Enter> {
    ascplot_widget_message %W
  }
  bind .ascplot.top_frm.varfrm.varsel.ctrbtns.bmid <Any-Enter> {
    ascplot_widget_message %W
  }
  bind .ascplot.top_frm.varfrm.varsel.ctrbtns.bleft <Any-Enter> {
    ascplot_widget_message %W
  }
  bind .ascplot.top_frm.varfrm.varsel.ctrbtns.bright <Any-Enter> {
    ascplot_widget_message %W
  }
  trace variable ascplotvect(curset) w ascplot_ablebtns
  ascplot_disableunimplemented
  # Pane binding the first one seems to mess up the window. don't know why.
  # HPane-Bind .ascplot top_frm bot_frm 12 0.98
  HPane-Bind .ascplot.top_frm datasets varfrm 15 0.98
  # set a min pixel height to avoid tk core dump in message textbox
  # set minpix [.ascplot.bot_frm.message lineheight] dont work
  # HPane-Bind .ascplot.bot_frm message table_frm 15 0.98 $minpix
  # kluge
  HPane-Bind .ascplot.bot_frm message table_frm 15 0.98 17
  ascplot_init_widgets
}
proc ascplot_ablebtns {n1 n2 op} {
  global ascplotvect
  if {[winfo exists .ascplot]} {
    if {$ascplotvect(curset) >= 0} {
      .ascplot.menubar.execute entryconfigure 0 -state normal
      .ascplot.menubar.execute entryconfigure 1 -state normal
      .ascplot.menubar.execute entryconfigure 3 -state normal
      .ascplot.menubar.execute entryconfigure 4 -state normal
      .ascplot.menubar.execute entryconfigure 5 -state normal
      .ascplot.menubar.execute entryconfigure 6 -state normal
      .ascplot.menubar.file entryconfigure 2 -state normal
      .ascplot.menubar.edit entryconfigure 0 -state normal
      .ascplot.menubar.display entryconfigure 0 -state normal
      .ascplot.menubar.display entryconfigure 2 -state normal
      update
    } else {
      .ascplot.menubar.execute entryconfigure 0 -state disabled
      .ascplot.menubar.execute entryconfigure 1 -state disabled
      .ascplot.menubar.execute entryconfigure 3 -state disabled
      .ascplot.menubar.execute entryconfigure 4 -state disabled
      .ascplot.menubar.execute entryconfigure 5 -state disabled
      .ascplot.menubar.execute entryconfigure 6 -state disabled
      .ascplot.menubar.edit entryconfigure 0 -state disabled
      .ascplot.menubar.display entryconfigure 0 -state disabled
      .ascplot.menubar.display entryconfigure 2 -state disabled
    }
  }
}
#
# proc ascplot_widget_message {w}
#---------------------------------------------------------------------
# sets the text message that goes with widget w.
#---------------------------------------------------------------------
proc ascplot_widget_message {w} {
  global ascplotvect
  if {"$w"=="$ascplotvect(textname)"} {
    ascplot_message "Information about the current box or button appears here."
    return
  }
  if {"$w"=="$ascplotvect(dataname)"} {
    ascplot_message \
"Spreadsheet-like list of data in the selected set shown here when requested
from Display menu."
    return
  }
  if {"$w"=="$ascplotvect(setsname)"} {
    ascplot_message \
"List of currently loaded data sets. Select a set to use by double clicking
mouse button one (One is normally the left mouse button.) Large data sets take
a fair amount of memory and should be unloaded once they are no longer needed."
    return
  }
  if {"$w"=="$ascplotvect(varsname)"} {
    ascplot_message \
    "List of unused variables in the data set. Each appears as:
Col# colid varname varunits.
To make a curve of the variable, select it and hit the >> button."
    return
  }
  if {"$w"=="$ascplotvect(depvname)"} {
    ascplot_message "List of variables to be plotted on the Y axis.
To remove a variable from the list, select it and hit the << button."
    return
  }
  if {"$w"=="$ascplotvect(indvname)"} {
    ascplot_message \
"The independent variable in the plot. This can be changed by selecting a new
one from either list and hitting that list's  \"V\"  button in the center to
trade with the independent variable."
    return
  }
  if {"$w"==".ascplot.top_frm.varfrm.varsel.ctrbtns.btop"} {
    ascplot_message \
      "Adds currently selected unused variable to the plotted var list."
    return
  }
  if {"$w"==".ascplot.top_frm.varfrm.varsel.ctrbtns.bmid"} {
    ascplot_message \
      "Moves currently selected plotted vars to the unused list."
    return
  }
  if {"$w"==".ascplot.top_frm.varfrm.varsel.ctrbtns.bleft"} {
    ascplot_message \
      "Makes first currently selected unused variable the x axis variable."
    return
  }
  if {"$w"==".ascplot.top_frm.varfrm.varsel.ctrbtns.bright"} {
    ascplot_message \
      "Makes first selected plotted y variable into the x variable."
    return
  }
  error "ascplot_widget_message called with bad widget $w"
}
#
# proc ascplot_init_widgets {}
#---------------------------------------------------------------------
# call  ascplot_init_widgets after loading widgets and initing plot window
#---------------------------------------------------------------------
proc ascplot_init_widgets {} {
  global ascplotvect AscConfirm ascParPageVect
 # file select
 # confirm button
  set AscConfirm(font) $ascplotvect(btnfont)
 # parms page
  set ascParPageVect(btn_font) $ascplotvect(btnfont)
  set ascParPageVect(lbl_font) $ascplotvect(btnfont)
  ascplot_setxgraphdef
  ascplot_setssdef
}
#
# proc ascplot_disableunimplemented {}
#---------------------------------------------------------------------
# disables all unimplemented buttons and startup unwanted buttons.
#---------------------------------------------------------------------
proc ascplot_disableunimplemented {} {
 # no display.loadoldplot
  .ascplot.menubar.display entryconfigure 3 -state disabled
  .ascplot.menubar.display entryconfigure 4 -state disabled
  .ascplot.menubar.display entryconfigure 5 -state disabled
 # no xmgr/gnuplot
  .ascplot.menubar.options.graph entryconfigure 1 -state disabled
  .ascplot.menubar.options.graph entryconfigure 2 -state disabled
 # no save data set or unload
  .ascplot.menubar.file entryconfigure 1 -state disabled
  .ascplot.menubar.file entryconfigure 2 -state disabled
  .ascplot.menubar.edit entryconfigure 0 -state disabled

 # disable plotting/calculating until data exists
  .ascplot.menubar.execute entryconfigure 0 -state disabled
  .ascplot.menubar.execute entryconfigure 1 -state disabled
  .ascplot.menubar.execute entryconfigure 3 -state disabled
  .ascplot.menubar.execute entryconfigure 4 -state disabled
  .ascplot.menubar.execute entryconfigure 5 -state disabled
  .ascplot.menubar.execute entryconfigure 6 -state disabled
 # disable showdata until data exists
  .ascplot.menubar.display entryconfigure 0 -state disabled
  .ascplot.menubar.display entryconfigure 2 -state disabled
}

#
# proc ascplot_genericok {}
#---------------------------------------------------------------------
# rescues any legend info that has been changed interactively for later
# reuse.
#---------------------------------------------------------------------
proc ascplot_genericok {} {
  global ascplotvect
  set aname $ascplotvect(array.$ascplotvect(curset))
  for {set i 0} {$i < $ascplotvect(ncurves)} {incr i} {
    set col "[lindex $ascplotvect(legend$i.label) 1]"
    set ${aname}(-6,$col) "$ascplotvect(legend$i)"
  }
  # mark window as down, as it is about to be
  set ascplotvect(uplegend) 0
}
#---------------------------------------------------------------------
#---------------------------------------------------------------------
proc ascplot_xgraphok {} {
  global ascplotvect
  set ascplotvect(upxgraph) 0
}
#
# proc ascplot_setxgraphdef {}
#---------------------------------------------------------------------
# here we set the defaults for ascplot_xgraphvect
# most are just as defaulted in by xgraph as shown with xgraph -db
#---------------------------------------------------------------------
proc ascplot_setxgraphdef {} {
  global ascplot_xgraphvect
  set ascplot_xgraphvect(namelist) \
    [list \
     BarGraph NoLines LogX LogY BoundBox Ticks \
     Markers PixelMarkers LargePixels StyleMarkers ReverseVideo Debug \
     XLowLimit XHighLimit  YLowLimit YHighLimit \
     BarWidth BarBase  \
     BorderSize GridSize LineWidth ZeroWidth GridStyle ZeroStyle \
     LabelFont TitleFont \
     Device FileOrDev Disposition \
     Style0 Style1 Style2 Style3 Style4 Style5 Style6 Style7 \
     Color0 Color1 Color2 Color3 Color4 Color5 Color6 Color7 \
     Border ZeroColor Command]
  set ascplot_xgraphvect(title) "XGraph Control"
  set ascplot_xgraphvect(cancellable) 1
  set ascplot_xgraphvect(toplevel) ".xgraph"
  set ascplot_xgraphvect(maxlines) "20"
  set ascplot_xgraphvect(entrywidth) "40"
  set ascplot_xgraphvect(npages) "4"
  set ascplot_xgraphvect(grab) "0"
  set ascplot_xgraphvect(helpcommand) "error {See the XGraph man page}"
  set ascplot_xgraphvect(whenokcommand) "ascplot_xgraphok"
 # page layout
  set ascplot_xgraphvect(BarGraph.page) 1
  set ascplot_xgraphvect(NoLines.page) 1
  set ascplot_xgraphvect(LogX.page) 1
  set ascplot_xgraphvect(LogY.page) 1
  set ascplot_xgraphvect(XHighLimit.page) 1
  set ascplot_xgraphvect(YHighLimit.page) 1
  set ascplot_xgraphvect(XLowLimit.page) 1
  set ascplot_xgraphvect(YLowLimit.page) 1
  set ascplot_xgraphvect(ReverseVideo.page) 1
  set ascplot_xgraphvect(Debug.page) 1
  set ascplot_xgraphvect(BoundBox.page) 2
  set ascplot_xgraphvect(Ticks.page) 2
  set ascplot_xgraphvect(Markers.page) 2
  set ascplot_xgraphvect(PixelMarkers.page) 2
  set ascplot_xgraphvect(LargePixels.page) 2
  set ascplot_xgraphvect(StyleMarkers.page) 2
  set ascplot_xgraphvect(BarBase.page) 2
  set ascplot_xgraphvect(BarWidth.page) 2
  set ascplot_xgraphvect(BorderSize.page) 2
  set ascplot_xgraphvect(GridSize.page) 2
  set ascplot_xgraphvect(ZeroWidth.page) 2
  set ascplot_xgraphvect(LineWidth.page) 2
  set ascplot_xgraphvect(LabelFont.page) 3
  set ascplot_xgraphvect(TitleFont.page) 3
  set ascplot_xgraphvect(Color0.page) 3
  set ascplot_xgraphvect(Color1.page) 3
  set ascplot_xgraphvect(Color2.page) 3
  set ascplot_xgraphvect(Color3.page) 3
  set ascplot_xgraphvect(Color4.page) 3
  set ascplot_xgraphvect(Color5.page) 3
  set ascplot_xgraphvect(Color6.page) 3
  set ascplot_xgraphvect(Color7.page) 3
  set ascplot_xgraphvect(ZeroColor.page) 3
  set ascplot_xgraphvect(Border.page) 3
  set ascplot_xgraphvect(Device.page) 4
  set ascplot_xgraphvect(Disposition.page) 4
  set ascplot_xgraphvect(FileOrDev.page) 4
  set ascplot_xgraphvect(GridStyle.page) 4
  set ascplot_xgraphvect(ZeroStyle.page) 4
  set ascplot_xgraphvect(Style0.page) 4
  set ascplot_xgraphvect(Style1.page) 4
  set ascplot_xgraphvect(Style2.page) 4
  set ascplot_xgraphvect(Style3.page) 4
  set ascplot_xgraphvect(Style4.page) 4
  set ascplot_xgraphvect(Style5.page) 4
  set ascplot_xgraphvect(Style6.page) 4
  set ascplot_xgraphvect(Style7.page) 4


 # booleans
  set ascplot_xgraphvect(BarGraph) 0
  set ascplot_xgraphvect(BarGraph.type) bool
  set ascplot_xgraphvect(NoLines) 0
  set ascplot_xgraphvect(NoLines.type) bool
  set ascplot_xgraphvect(LogX) 0
  set ascplot_xgraphvect(LogX.type) bool
  set ascplot_xgraphvect(LogY) 0
  set ascplot_xgraphvect(LogY.type) bool
  set ascplot_xgraphvect(BoundBox) 0
  set ascplot_xgraphvect(BoundBox.type) bool
  set ascplot_xgraphvect(Ticks) 0
  set ascplot_xgraphvect(Ticks.type) bool
  set ascplot_xgraphvect(Markers) 1
  set ascplot_xgraphvect(Markers.type) bool
  set ascplot_xgraphvect(PixelMarkers) 0
  set ascplot_xgraphvect(PixelMarkers.type) bool
  set ascplot_xgraphvect(LargePixels) 0
  set ascplot_xgraphvect(LargePixels.type) bool
  set ascplot_xgraphvect(StyleMarkers) 0
  set ascplot_xgraphvect(StyleMarkers.type) bool
  set ascplot_xgraphvect(ReverseVideo) 0
  set ascplot_xgraphvect(ReverseVideo.type) bool
  set ascplot_xgraphvect(Debug) 0
  set ascplot_xgraphvect(Debug.type) bool
 # reals
  set ascplot_xgraphvect(XHighLimit) 0
  set ascplot_xgraphvect(XHighLimit.type) real
  set ascplot_xgraphvect(YHighLimit) 0
  set ascplot_xgraphvect(YHighLimit.type) real
  set ascplot_xgraphvect(XLowLimit) 1
  set ascplot_xgraphvect(XLowLimit.type) real
  set ascplot_xgraphvect(YLowLimit) 1
  set ascplot_xgraphvect(YLowLimit.type) real
  set ascplot_xgraphvect(BarBase) 0
  set ascplot_xgraphvect(BarBase.type) real
  set ascplot_xgraphvect(BarWidth) -1.0
  set ascplot_xgraphvect(BarWidth.type) real
 # ints
  set ascplot_xgraphvect(BorderSize) 2
  set ascplot_xgraphvect(BorderSize.lo) 0
  set ascplot_xgraphvect(BorderSize.hi) 10
  set ascplot_xgraphvect(BorderSize.type) int
  set ascplot_xgraphvect(GridSize) 0
  set ascplot_xgraphvect(GridSize.lo) 0
  set ascplot_xgraphvect(GridSize.hi) 10
  set ascplot_xgraphvect(GridSize.type) int
  set ascplot_xgraphvect(ZeroWidth) 3
  set ascplot_xgraphvect(ZeroWidth.lo) 0
  set ascplot_xgraphvect(ZeroWidth.hi) 10
  set ascplot_xgraphvect(ZeroWidth.type) int
  set ascplot_xgraphvect(ZeroWidth.label) ZeroWidth
  set ascplot_xgraphvect(LineWidth) 0
  set ascplot_xgraphvect(LineWidth.lo) 0
  set ascplot_xgraphvect(LineWidth.hi) 10
  set ascplot_xgraphvect(LineWidth.type) int
 # strings
  global tcl_platform env ascUtilVect
  if {[string compare $tcl_platform(platform) windows]==0} {
    if {[info exists ascUtilVect(plot_command)] && \
        [string compare $ascUtilVect(plot_type) "xgraph"]==0} {
      set ascplot_xgraphvect(Command) $ascUtilVect(plot_command)
    } else {
      set ascplot_xgraphvect(Command) "{tkxgraph.exe} -- -f"
    }
  } else {
    set ascplot_xgraphvect(Command) "xgraph"
  }
  set ascplot_xgraphvect(Command.type) string
  global ascUtilVect
  set ascplot_xgraphvect(LabelFont) "helvetica-12"
  set ascplot_xgraphvect(LabelFont.type) string
  set ascplot_xgraphvect(TitleFont) "helvetica-18"
  set ascplot_xgraphvect(TitleFont.type) string
  set ascplot_xgraphvect(Device) Postscript
  set ascplot_xgraphvect(Device.type) string
  set ascplot_xgraphvect(Device.choices) "Postscript HPGL Idraw"
  set ascplot_xgraphvect(Disposition) "To Device"
  set ascplot_xgraphvect(Disposition.type) string
  set ascplot_xgraphvect(Disposition.choices) "{To Device} {To File}"
  set oname default
  catch {set oname $env(PRINTER)}
  set ascplot_xgraphvect(FileOrDev) $oname
  set ascplot_xgraphvect(FileOrDev.type) string
 # bins
  set ascplot_xgraphvect(GridStyle) 10
  set ascplot_xgraphvect(GridStyle.hi) 31
  set ascplot_xgraphvect(GridStyle.type) bin
  set ascplot_xgraphvect(GridStyle.label) "GridStyle bits"
  set ascplot_xgraphvect(ZeroStyle) 1
  set ascplot_xgraphvect(ZeroStyle.hi) 31
  set ascplot_xgraphvect(ZeroStyle.type) bin
  set ascplot_xgraphvect(ZeroStyle.label) "ZeroStyle bits"
  set ascplot_xgraphvect(Style0) 1
  set ascplot_xgraphvect(Style0.hi) 31
  set ascplot_xgraphvect(Style0.type) bin
  set ascplot_xgraphvect(Style0.label) "LineStyle 0 bits"
  set ascplot_xgraphvect(Style1) 10
  set ascplot_xgraphvect(Style1.hi) 31
  set ascplot_xgraphvect(Style1.type) bin
  set ascplot_xgraphvect(Style1.label) "LineStyle 1 bits"
  set ascplot_xgraphvect(Style2) 11110000
  set ascplot_xgraphvect(Style2.hi) 31
  set ascplot_xgraphvect(Style2.type) bin
  set ascplot_xgraphvect(Style2.label) "LineStyle 2 bits"
  set ascplot_xgraphvect(Style3) "010111"
  set ascplot_xgraphvect(Style3.hi) 31
  set ascplot_xgraphvect(Style3.type) bin
  set ascplot_xgraphvect(Style3.label) "LineStyle 3 bits"
  set ascplot_xgraphvect(Style4) 1110
  set ascplot_xgraphvect(Style4.hi) 31
  set ascplot_xgraphvect(Style4.type) bin
  set ascplot_xgraphvect(Style4.label) "LineStyle 4 bits"
  set ascplot_xgraphvect(Style5) 1111111100000000
  set ascplot_xgraphvect(Style5.hi) 31
  set ascplot_xgraphvect(Style5.type) bin
  set ascplot_xgraphvect(Style5.label) "LineStyle 5 bits"
  set ascplot_xgraphvect(Style6) 11001111
  set ascplot_xgraphvect(Style6.hi) 31
  set ascplot_xgraphvect(Style6.type) bin
  set ascplot_xgraphvect(Style6.label) "LineStyle 6 bits"
  set ascplot_xgraphvect(Style7) "0011000111"
  set ascplot_xgraphvect(Style7.hi) 31
  set ascplot_xgraphvect(Style7.type) bin
  set ascplot_xgraphvect(Style7.label) "LineStyle 7 bits"

 # x strings
  if {[winfo depth .]==1} {
    set ascplot_xgraphvect(Color0) "black"
    set ascplot_xgraphvect(Color0.type) string
    set ascplot_xgraphvect(Color0.label) "Line Color 0"
    set ascplot_xgraphvect(Color1) "black"
    set ascplot_xgraphvect(Color1.type) string
    set ascplot_xgraphvect(Color1.label) "Line Color 1"
    set ascplot_xgraphvect(Color2) "black"
    set ascplot_xgraphvect(Color2.type) string
    set ascplot_xgraphvect(Color2.label) "Line Color 2"
    set ascplot_xgraphvect(Color3) "black"
    set ascplot_xgraphvect(Color3.type) string
    set ascplot_xgraphvect(Color3.label) "Line Color 3"
    set ascplot_xgraphvect(Color4) "black"
    set ascplot_xgraphvect(Color4.type) string
    set ascplot_xgraphvect(Color4.label) "Line Color 4"
    set ascplot_xgraphvect(Color5) "black"
    set ascplot_xgraphvect(Color5.type) string
    set ascplot_xgraphvect(Color5.label) "Line Color 5"
    set ascplot_xgraphvect(Color6) "black"
    set ascplot_xgraphvect(Color6.type) string
    set ascplot_xgraphvect(Color6.label) "Line Color 6"
    set ascplot_xgraphvect(Color7) "black"
    set ascplot_xgraphvect(Color7.type) string
    set ascplot_xgraphvect(Color7.label) "Line Color 7"
    set ascplot_xgraphvect(ZeroColor) "black"
    set ascplot_xgraphvect(ZeroColor.type) string
    set ascplot_xgraphvect(ZeroColor.label) "ZeroColor"
    set ascplot_xgraphvect(Border) "white"
    set ascplot_xgraphvect(Border.type) string
    set ascplot_xgraphvect(Border.label) "BorderColor"
  } else {
    set ascplot_xgraphvect(Color0) "red"
    set ascplot_xgraphvect(Color0.type) string
    set ascplot_xgraphvect(Color0.label) "Line Color 0"
    set ascplot_xgraphvect(Color1) "green"
    set ascplot_xgraphvect(Color1.type) string
    set ascplot_xgraphvect(Color1.label) "Line Color 1"
    set ascplot_xgraphvect(Color2) "blue"
    set ascplot_xgraphvect(Color2.type) string
    set ascplot_xgraphvect(Color2.label) "Line Color 2"
    set ascplot_xgraphvect(Color3) "yellow"
    set ascplot_xgraphvect(Color3.type) string
    set ascplot_xgraphvect(Color3.label) "Line Color 3"
    set ascplot_xgraphvect(Color4) "cyan"
    set ascplot_xgraphvect(Color4.type) string
    set ascplot_xgraphvect(Color4.label) "Line Color 4"
    set ascplot_xgraphvect(Color5) "sienna"
    set ascplot_xgraphvect(Color5.type) string
    set ascplot_xgraphvect(Color5.label) "Line Color 5"
    set ascplot_xgraphvect(Color6) "orange"
    set ascplot_xgraphvect(Color6.type) string
    set ascplot_xgraphvect(Color6.label) "Line Color 6"
    set ascplot_xgraphvect(Color7) "coral"
    set ascplot_xgraphvect(Color7.type) string
    set ascplot_xgraphvect(Color7.label) "Line Color 7"
    set ascplot_xgraphvect(ZeroColor) "white"
    set ascplot_xgraphvect(ZeroColor.type) string
    set ascplot_xgraphvect(ZeroColor.label) "ZeroColor"
    set ascplot_xgraphvect(Border) "black"
    set ascplot_xgraphvect(Border.type) string
    set ascplot_xgraphvect(Border.label) "BorderColor"
  }
}
# define setpos and other ascend procs if not here already
# for documentation see ascend files DisplayProc.tcl and generalk.tcl
if {"[info procs setpos]" == "" || "[info procs FileUniqueName]" == ""} {
  proc setpos {master {offsetx "70"} {offsety "70"}} {
    set xpos [expr [winfo rootx $master] + $offsetx]
    set ypos [expr [winfo rooty $master] + $offsety]
    return "+$xpos+$ypos"
  }
  proc getpos {master} {
    set list [split [wm geometry $master] +]
    set xpos [lindex $list 1]
    set ypos [lindex $list 2]
    return "+$xpos+$ypos"
  }
  proc FileUniqueName {{prefix "/tmp/"}} {
    set pidvar [pid]
    set datevar [clock format [clock seconds] -format %y%m%d%H%M%S]
    set fid "$prefix$pidvar\.$datevar"
    set ifid $fid
    set i 1
    while {[file exists $ifid]} {set ifid "$fid\.$i" ; incr i}
    return $ifid
  }
}


#---------------------------------------------------------------------
#---------------------------------------------------------------------
# an ascplot script language
#---------------------------------------------------------------------
#---------------------------------------------------------------------
#
# proc ascplot {keyword args}
#---------------------------------------------------------------------
# simple parser of ascplot commands. distributes according to
# keyword.
#---------------------------------------------------------------------
proc ascplot {keyword args} {
  set list [list \
    load \
    view \
    useset \
    independent \
    dependent \
    legend \
    insert \
    setcell \
    calc \
    write \
    title \
    xtitle \
    ytitle \
    help \
    show \
  ]
  if {[lsearch -exact $list $keyword] == -1} {
    error "ascplot: Unrecognized keyword $keyword."
  }
}


# Module: oktext.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
#
# (C) Benjamin A Allan, August 1995
# This widget is placed in the public domain.

# module contents
global moduleList
global autoLoadList
set moduleList(oktext.tcl) { $top}
set autoLoadList(oktext.tcl) {0}


# procedure to show window $top
# This widget displays a textbox and an ok button
# The text box fill itself with the string given
# in the font specified.
# Color are the class defaults.
# Usage:
# OKText toplevel string title grab font geometry height width
# toplevel: the widget name
# string: message to put in the box
# title: window title
# grab: 0 nograb, 1 grab until OK pressed. (not implemented)
# font: text font
# geometry: window geom to attempt
# height: lines in text
# width: width of text
# if toplevel is already up, it is destroyed first.
proc OKText {top message title grab font geom ht wid} {

  # build widget $top
  catch "destroy $top"
  toplevel $top

  # Window manager configurations
  global tk_version
  wm positionfrom $top ""
  wm sizefrom $top ""
  wm maxsize $top 1500 1500
  wm minsize $top 10 10
  wm geometry $top $geom
  wm title $top $title


  # build widget $top.ok_btn
  button $top.ok_btn \
    -text {OK} \
    -font $font \
    -command "destroy $top; update"

  # build widget $top.text_frm
  frame $top.text_frm \
    -relief {raised}

  # build widget $top.text_frm.scrollbar1
  scrollbar $top.text_frm.scrollbar1 \
    -command "$top.text_frm.text2 yview" \
    -relief {raised}

  # build widget $top.text_frm.text2
  text $top.text_frm.text2 \
    -exportselection {0} \
    -font $font \
    -height $ht \
    -width $wid \
    -wrap {word} \
    -yscrollcommand "$top.text_frm.scrollbar1 set"
  # bindings
  bind $top.text_frm.text2 <Any-Key> {catch {NoFunction} }

  # pack widget $top.text_frm
  pack append $top.text_frm \
    $top.text_frm.scrollbar1 {right frame center filly} \
    $top.text_frm.text2 {top frame center expand fill}

  # pack widget $top
  pack append $top \
    $top.text_frm {top frame center expand fill} \
    $top.ok_btn {top frame center fill}

  $top.text_frm.text2 insert end $message


}



# Internal procedures

# eof
#


#  mps.tcl: contains all the Tcl code connected with makeMPS
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.7 $
#  Last modified on: $Date: 1998/06/18 15:55:32 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: mps.tcl,v $
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

# This file contains all the Tcl code connected with makeMPS
#
# As a user, you may need to modify the Solv6_RunSolver and
# Solv6_LoadSolution files, depending on how the solvers
# are installed at your site.
#

# Module: mps.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
global ascGlobalVect
set moduleList(mps.tcl) {.mps}
set autoLoadList(mps.tcl) {1}

# ____________________________________________________________________________#
# Set up ShowWindow.mps                                                       #
# ____________________________________________________________________________#

# procedure to show window .mps
proc ShowWindow.mps {args} {
# xf ignore me 7

global ascGlobalVect

  # build widget .mps
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .mps"
  } {
    catch "destroy .mps"
  }
  toplevel .mps

  # Window manager configurations
  wm positionfrom .mps user
  wm sizefrom .mps user
  wm maxsize .mps 1024 768
  wm minsize .mps 0 0
  wm title .mps {makeMPS Parameters}
  wm geometry .mps [osgpos 381x216[setpos .solver 1 30]]
  wm protocol .mps WM_DELETE_WINDOW {Solv6_Ok}


  # build widget .mps.frame0
  frame .mps.frame0 \
    -borderwidth {3} \
    -width {231} \
    -height {194} \
    -relief {ridge}

  # build widget .mps.frame0.frame1
  frame .mps.frame0.frame1 \
    -borderwidth {1}   \
    -background $ascGlobalVect(tbg)


  # build widget .mps.frame0.frame1.label4
  label .mps.frame0.frame1.label4 \
    -borderwidth {0} \
    -text {MILP Solver:}  \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame0.frame1.radiobutton5
  radiobutton .mps.frame0.frame1.radiobutton5 \
    -anchor {w} \
    -borderwidth {1} \
    -command { Solv6_QOMILP_actions } \
    -padx {0} \
    -pady {0} \
    -text {QOMILP} \
    -value {QOMILP} \
    -variable {ascSolv6Vect(solver)}  \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame0.frame1.radiobutton6
  radiobutton .mps.frame0.frame1.radiobutton6 \
    -anchor {w} \
    -borderwidth {1} \
    -command {Solv6_lpsolve_actions } \
    -padx {0} \
    -pady {0} \
    -text {lpsolve} \
    -value {lpsolve} \
    -variable {ascSolv6Vect(solver)} \
    -background $ascGlobalVect(tbg)


  # pack widget .mps.frame0.frame1
  pack append .mps.frame0.frame1 \
    .mps.frame0.frame1.label4 {top frame center expand fill} \
    .mps.frame0.frame1.radiobutton5 {top frame w expand fill} \
    .mps.frame0.frame1.radiobutton6 {top frame center expand fill}

  # build widget .mps.frame0.frame2
  frame .mps.frame0.frame2 \
    -borderwidth {1}    \
    -background $ascGlobalVect(tbg)


  # build widget .mps.frame0.frame2.radiobutton9
  radiobutton .mps.frame0.frame2.radiobutton9 \
    -anchor {w} \
    -borderwidth {1} \
    -command {Solv6_OSL_actions} \
    -text {OSL} \
    -value {OSL} \
    -variable {ascSolv6Vect(solver)}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame0.frame2.radiobutton10
  radiobutton .mps.frame0.frame2.radiobutton10 \
    -anchor {w} \
    -borderwidth {1} \
    -command {Solv6_SCICONIC_actions} \
    -text {SCICONIC} \
    -value {SCICONIC} \
    -variable {ascSolv6Vect(solver)}  \
    -background $ascGlobalVect(tbg)


  # build widget .mps.frame0.frame2.radiobutton11
  radiobutton .mps.frame0.frame2.radiobutton11 \
    -anchor {w} \
    -borderwidth {1} \
    -command {# use OSL actions for generic here
              Solv6_OSL_actions} \
    -padx {0} \
    -pady {0} \
    -text {Generic} \
    -value {Generic} \
    -variable {ascSolv6Vect(solver)}    \
    -background $ascGlobalVect(tbg)

  # pack widget .mps.frame0.frame2
  pack append .mps.frame0.frame2 \
    .mps.frame0.frame2.radiobutton9 {top frame w expand fill} \
    .mps.frame0.frame2.radiobutton10 {top frame w expand fill} \
    .mps.frame0.frame2.radiobutton11 {top frame w expand fill}

  # pack widget .mps.frame0
  pack append .mps.frame0 \
    .mps.frame0.frame1 {left frame w expand fill} \
    .mps.frame0.frame2 {left frame center expand fill}

  # build widget .mps.frame12
  frame .mps.frame12 \
    -borderwidth {3} \
    -relief {ridge}     \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame12.frame
  frame .mps.frame12.frame \
    -borderwidth {1} \
    -relief {raised}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame12.frame.label4
  label .mps.frame12.frame.label4 \
    -borderwidth {0} \
    -padx {2} \
    -text {MPS Filename:}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame12.frame.entry5
  entry .mps.frame12.frame.entry5 \
    -borderwidth {1} \
    -relief {raised} \
    -textvariable {ascSolv6Vect(filename)} \
    -width {0}  \
    -background $ascGlobalVect(bg)

  # pack widget .mps.frame12.frame
  pack append .mps.frame12.frame \
    .mps.frame12.frame.label4 {left frame center} \
    .mps.frame12.frame.entry5 {top frame center expand fill}

  # pack widget .mps.frame12
  pack append .mps.frame12 \
    .mps.frame12.frame {top frame center expand fill}

  # build widget .mps.frame13
  frame .mps.frame13 \
    -borderwidth {3} \
    -relief {ridge}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.checkbutton15
  checkbutton .mps.frame13.checkbutton15 \
    -anchor {w} \
    -borderwidth {1} \
    -padx {0} \
    -pady {0} \
    -state {disabled} \
    -text {Use special ordered sets} \
    -variable {ascSolv6Vect(SOS)}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.checkbutton16
  checkbutton .mps.frame13.checkbutton16 \
    -anchor {w} \
    -borderwidth {1} \
    -state {disabled} \
    -text {Use semicontinuous variables} \
    -variable {ascSolv6Vect(semi)}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.frame
  frame .mps.frame13.frame \
    -relief {raised}  \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.frame.label4
  label .mps.frame13.frame.label4 \
    -borderwidth {1} \
    -padx {2} \
    -relief {raised} \
    -text {Valid Bound:}  \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.frame.entry5
  entry .mps.frame13.frame.entry5 \
    -relief {raised} \
    -textvariable {ascSolv6Vect(boval)}    \
    -background $ascGlobalVect(bg)

  # pack widget .mps.frame13.frame
  pack append .mps.frame13.frame \
    .mps.frame13.frame.label4 {left frame center} \
    .mps.frame13.frame.entry5 {top frame center expand fill}

  # build widget .mps.frame13.frame4
  frame .mps.frame13.frame4 \
    -relief {raised}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.frame4.label4
  label .mps.frame13.frame4.label4 \
    -borderwidth {1} \
    -padx {2} \
    -relief {raised} \
    -text {Termination Tolerance:}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame13.frame4.entry5
  entry .mps.frame13.frame4.entry5 \
    -relief {raised} \
    -textvariable {ascSolv6Vect(epsval)}    \
    -background $ascGlobalVect(bg)

  # pack widget .mps.frame13.frame4
  pack append .mps.frame13.frame4 \
    .mps.frame13.frame4.label4 {left frame center} \
    .mps.frame13.frame4.entry5 {top frame center expand fill}

  # build widget .mps.frame13.checkbutton0
  checkbutton .mps.frame13.checkbutton0 \
    -anchor {w} \
    -text {Solve relaxed problem} \
    -variable {ascSolv6Vect(relaxed)}  \
    -background $ascGlobalVect(tbg)

  # pack widget .mps.frame13
  pack append .mps.frame13 \
    .mps.frame13.checkbutton15 {top frame center expand fill} \
    .mps.frame13.checkbutton16 {top frame center expand fill} \
    .mps.frame13.frame {top frame center fill} \
    .mps.frame13.frame4 {top frame center fill} \
    .mps.frame13.checkbutton0 {top frame center expand fill}

  # build widget .mps.frame14
  frame .mps.frame14 \
    -borderwidth {3} \
    -relief {ridge}    \
    -background $ascGlobalVect(tbg)

  # build widget .mps.frame14.button0
  button .mps.frame14.button0 \
    -command { Solv6_Ok } \
    -text {OK}    \
    -background $ascGlobalVect(bg)

  # build widget .mps.frame14.button1
  button .mps.frame14.button1 \
    -command {#script for Make MPS file button here
              Solv6_MakeMPS} \
    -text {Make MPS file}     \
    -background $ascGlobalVect(bg)

  # build widget .mps.frame14.button2
  button .mps.frame14.button2 \
    -command {#script for Run solver button here
              Solv6_RunSolver} \
    -state {normal} \
    -text {Run solver}    \
    -background $ascGlobalVect(bg)

      # build widget .mps.frame14.button3
  button .mps.frame14.button3 \
    -command {#script for Load Solution button here
              Solv6_LoadSolution} \
    -text {Load solution}    \
    -background $ascGlobalVect(bg)

  # build widget .mps.frame14.button4
  button .mps.frame14.button4 \
    -command {#script for Help button here
              Help_button solver.makemps.parameters} \
    -text {Help}    \
    -background $ascGlobalVect(bg)

  # pack widget .mps.frame14
  pack append .mps.frame14 \
    .mps.frame14.button0 {left frame center expand fill} \
    .mps.frame14.button1 {left frame center expand fill} \
    .mps.frame14.button2 {left frame center expand fill} \
    .mps.frame14.button3 {left frame center expand fill} \
    .mps.frame14.button4 {left frame center expand fill}

  # pack widget .mps
  pack append .mps \
    .mps.frame0 {top frame n expand fill} \
    .mps.frame12 {top frame center expand fill} \
    .mps.frame13 {top frame center expand fill} \
    .mps.frame14 {top frame center expand fill}

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .mps"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

proc DestroyWindow.mps {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .mps]" != ""} {
      global xfShowWindow.mps
      set xfShowWindow.mps 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .mps; XFEditSetShowWindows"
    }
  } {
    catch "destroy .mps"
    update
  }
}

# ____________________________________________________________________________#
# Button handling procedures, that dim things as necessary                    #
# ____________________________________________________________________________#

#
# proc Solv6_QOMILP_actions {}
#----------------------------------------------------------------------
# activate dialog items as appropriate for QOMILP solver              #
# modified by CWS, 5/95                                               #
#----------------------------------------------------------------------

proc Solv6_QOMILP_actions {}  {

  global ascSolv6Vect

# disable SOS
.mps.frame13.checkbutton15 configure -state disabled
# disable semicontinuous
.mps.frame13.checkbutton16 configure -state disabled
#turn off SOS and semi
set ascSolv6Vect(SOS) 0
set ascSolv6Vect(semi) 0
# enable valid bounds
.mps.frame13.frame.label4 configure -foreground Black
.mps.frame13.frame.entry5 configure -state normal
.mps.frame13.frame.entry5 configure -foreground Black
# enable termination tolerance
.mps.frame13.frame4.label4 configure -foreground Black
.mps.frame13.frame4.entry5 configure -state normal
.mps.frame13.frame4.entry5 configure -foreground Black

}


#
# proc Solv6_lpsolve_actions {}
#----------------------------------------------------------------------
# activate dialog items as appropriate for lpsolve solver             #
# modified by CWS, 5/95                                               #
#----------------------------------------------------------------------

proc Solv6_lpsolve_actions {}  {

  global ascSolv6Vect

# disable SOS
.mps.frame13.checkbutton15 configure -state disabled
# disable semicontinuous
.mps.frame13.checkbutton16 configure -state disabled
#turn off buttons
set ascSolv6Vect(SOS) 0
set ascSolv6Vect(semi) 0
# disable valid bounds
.mps.frame13.frame.label4 configure -foreground Gray70
.mps.frame13.frame.entry5 configure -state disabled
.mps.frame13.frame.entry5 configure -foreground Gray70
# diable termination tolerance
.mps.frame13.frame4.label4 configure -foreground Gray70
.mps.frame13.frame4.entry5 configure -state disabled
.mps.frame13.frame4.entry5 configure -foreground Gray70

}

#
# proc Solv6_OSL_actions {}
#----------------------------------------------------------------------
# activate dialog items as appropriate for OSL solver                 #
# note: this is also used for the Generic solver                      #
# modified by CWS, 5/95                                               #
#----------------------------------------------------------------------

proc Solv6_OSL_actions {}  {

  global ascSolv6Vect

# enable SOS
.mps.frame13.checkbutton15 configure -state normal
# disable semicontinuous
.mps.frame13.checkbutton16 configure -state disabled
#turn off semi button
set ascSolv6Vect(semi) 0
# disable valid bounds
.mps.frame13.frame.label4 configure -foreground Gray70
.mps.frame13.frame.entry5 configure -state disabled
.mps.frame13.frame.entry5 configure -foreground Gray70
# disable termination tolerance
.mps.frame13.frame4.label4 configure -foreground Gray70
.mps.frame13.frame4.entry5 configure -state disabled
.mps.frame13.frame4.entry5 configure -foreground Gray70

}


#
# proc Solv6_SCICONIC_actions {}
#----------------------------------------------------------------------
# activate dialog items as appropriate for SICONIC solver             #
# modified by CWS, 5/95                                               #
#----------------------------------------------------------------------

proc Solv6_SCICONIC_actions {}  {

  global ascSolv6Vect

# enable SOS
.mps.frame13.checkbutton15 configure -state normal
# enable semicontinuous
.mps.frame13.checkbutton16 configure -state normal
# disable valid bounds
.mps.frame13.frame.label4 configure -foreground Gray70
.mps.frame13.frame.entry5 configure -state disabled
.mps.frame13.frame.entry5 configure -foreground Gray70
# disable termination tolerance
.mps.frame13.frame4.label4 configure -foreground Gray70
.mps.frame13.frame4.entry5 configure -state disabled
.mps.frame13.frame4.entry5 configure -foreground Gray70

}

# ____________________________________________________________________________#
# Main level control, for the five buttons                                    #
# ____________________________________________________________________________#

# What to do when the user clicks on the "Ok" button
#
# proc Solv6_Ok {}
#-----------------------------------------------------------------------
# Toplevel procedure for the "Make MPS file" button                    #
# modified by CWS, 5/95                                                #
#-----------------------------------------------------------------------
proc Solv6_Ok {} {

# just return if input is bad
if {[Solv6_ErrorCheck 0 ]} {return}

#Close up everything, as normally in SolverProc.tcl
 Solve_do_Parms close 6


}

# What to do when the user clicks on the "Make MPS file dialog"
#
# proc Solv6_MakeMPS {}
#-----------------------------------------------------------------------
# Toplevel procedure for the "Make MPS file" button                    #
# modified by CWS, 5/95                                                #
#-----------------------------------------------------------------------
proc Solv6_MakeMPS {} {

    global ascSolvVect

# if the current solver isn't makeMPS, switch to it
  if {6> [expr $ascSolvVect(numberofsolvers) -1]} {
      puts "Error: makeMPS is not available.\n"
      return
  }
  if {$ascSolvVect(available.6)} {
          Solve_do_Select 6
  } else {
    puts "Error: makeMPS is not available.\n"
    return
  }

# just return if input is bad
  if {[Solv6_ErrorCheck 1 ]} {return}

# Close up everything, as normally in SolverProc.tcl
  Solve_do_Parms close 6

# call optimize function, as from menu (w/o annoying dialog)
  Solve_Optimize

}


#
# proc Solv6_RunSolver {}
#-----------------------------------------------------------------------
# Toplevel procedure for the "Run Solver" button                       #
# modified by CWS, 5/95                                                #
#-----------------------------------------------------------------------
proc Solv6_RunSolver {} {

    global ascSolvVect ascSolv6Vect env ascSolvStatVect

# make the MPS file
  Solv6_MakeMPS

# run the appropriate script, depending on the solver
  switch $ascSolv6Vect(solver) {
      QOMILP     {
#       note: qomilp returns 1 so, can't error check
        catch {[exec qomilp $ascSolv6Vect(mpsname) 1 10000 > \
                $ascSolv6Vect(outname) 2> $ascSolv6Vect(errname) & ]}
      }
      lpsolve    {
        catch {[exec lp_solve -s -mps < $ascSolv6Vect(mpsname) > \
         $ascSolv6Vect(outname) 2> $ascSolv6Vect(errname) & ]}
      }
      OSL        {
        Solve_Alert "Sorry" \
        "OSL isn't hooked up to ASCEND yet.\n(The MPS file was created.)"
      }
      SCICONIC   {
#        create the sciconic.add file here
         set f [open sciconic.add w]
         puts $f "INFILE = '$ascSolv6Vect(mpsname)'    @ Specify the input file"
         puts $f "OUTFILE = '$ascSolv6Vect(outname)'   @ Specify the output file"
         puts $f "LINES = 100000        @ Print all one big page"
         puts $f "CONVERT               @ Read and convert the MPS file"

         if { $ascSolv6Vect(SOS) == 1 } {
         puts $f "SETUP  ( MARKER )     @ Create problem internally, with SOS"
         } else {
         puts $f "SETUP                 @ Create problem internally, no SOS"
         }
         puts $f "CRASH                 @ Presolve and find initial point"
         puts $f "PRIMAL                @ Use crased soln to find an initial basis"
         puts $f "GLOBAL                @ Do Branch and Bound"
         puts $f "@ Limit info printed in soln"
         puts $f "SELECT"
         puts $f "ROWS"
         puts $f "    XXXXXXXX"
         puts $f "COLUMNS"
         puts $f " NZ;@@@@@@@@"
         puts $f "ENDATA"
         puts $f "PUNCHSOLUTION         @ Write solution to OUTFILE"
         puts $f "STOP                  @ All done!"

         close $f
#        sciconic is really noisy, so send stdout to /dev/null
         catch {[exec sciconic < sciconic.add > /dev/null & ]}
      }
      Generic    {
        Solve_Alert "Sorry"  \
        "A Generic solver isn't hooked up to ASCEND yet.\n(The MPS file was created.)"
      }
  }

#remove output.err if it exists and is empty
 if { [file exists $ascSolv6Vect(errname)] &&
    ![file size $ascSolv6Vect(errname)] }  {
    catch {[file delete $ascSolv6Vect(errname)]} }


}


# This routine takes the output from lp_solve (*.out),
# and a name map file from ASCEND (*.map), and uses
# it to set the variable values in ASCEND
#
# Variables which are no longer defined generate errors
#
# by Craig Schmidt 6/21/95

proc Solv6_LoadSolution {} {

  global ascSolv6Vect

  # just return if input is bad (don't confirm overwrite, since read only use)
  if {[Solv6_ErrorCheck 0 ]} {return}

  if {![file exists $ascSolv6Vect(outname)]} {
      Solve_Alert "Error"  \
      "The file $ascSolv6Vect(outname) does not exist."
      return
  }

  if {![file exists $ascSolv6Vect(mapname)]} {
      Solve_Alert "Error"  \
      "The file $ascSolv6Vect(mapname) does not exist."
      return
  }

  set outf [open $ascSolv6Vect(outname) r]
  set mapf [open $ascSolv6Vect(mapname) r]

# Process the *.out file, creating array of non-zero values
  switch $ascSolv6Vect(solver) {
      QOMILP
      {
#           search thru file until done w/ header
	    while {[gets $outf line] >= 0} {
		 if {[regexp {^Nonzero elements:} $line]} {break}
	    }
            if {![regexp {^Nonzero elements:} $line]} {
                Solve_Alert "Error"  \
                "Couldn't find 'Nonzero elements:' in the \nheader of the QOMILP output file."
                close $outf
                close $mapf
                return
            }
	    while {[gets $outf line] >= 0} {
	      if {[regexp { +[0-9]+ (C[0-9]+) +([0-9eE+.-]+)} $line all save1 save2] } {
	          set nonzeros($save1) $save2
	      } else { break }
	    }
      }
      lpsolve
      {
	    gets $outf line
            if {![regexp {^Value of objective function:} $line]} {
                Solve_Alert "Error"  \
                "Couldn't find 'Value of objective function:'\n in the header of the lp_solve output file."
                close $outf
                close $mapf
                return
            }
	    while {[gets $outf line] >= 0} {
        	    regexp {(^C[0-9]+)\ +([0-9eE+.-]+)} $line all save1 save2
        	    set nonzeros($save1) $save2
	    }
      }
      OSL
      {
            Solve_Alert "Sorry" \
            "OSL isn't hooked up to ASCEND yet.\n(Nothing was changed.)"
            close $outf
            close $mapf
            return
      }
      SCICONIC
      {
#           search thru file until done w/ header
	    while {[gets $outf line] >= 0} {
		 if {[regexp {^     \.NUMBER\.  \.COLUMN\.} $line]} {break}
	    }
            if {![regexp {^     \.NUMBER\.  \.COLUMN\.} $line]} {
                Solve_Alert "Error"  \
                "Couldn't find '.NUMBER.  .COLUMN.' in the \nheader of the SCICONIC output file."
                close $outf
                close $mapf
                return
            }
	    while {[gets $outf line] >= 0} {
	      if {[regexp {.... +[0-9]+  (C[0-9]+)  .. +([0-9eE+.-]+)} $line all save1 save2] } {
	          set nonzeros($save1) $save2
	      } else { break }
	    }

      }
      Generic
      {
            Solve_Alert "Sorry"  \
            "A Generic solver isn't hooked up to ASCEND yet.\n(Nothing was changed.)"
            close $outf
            close $mapf
            return
      }
  }

  #eat the first 6 lines of the name map file
  gets $mapf line
  gets $mapf line
  gets $mapf line
  gets $mapf line
  gets $mapf line
  gets $mapf line

  # Read in the name map file, setting variable values
  while {[gets $mapf line] >= 0} {
	 regexp {(C[0-9]+) +(.+)} $line all save1 save2
    if {[info exists nonzeros($save1)]} {
	 if {[catch "ASSIGN $save2 $nonzeros($save1)" ]} {
           puts "Couldn't set value for undefined variable: $save2"
	 }
    } else {   #assoc element not defined, so value 0
	 if {[catch "ASSIGN $save2 0" ]} {
           puts "Couldn't set value for undefined variable: $save2"
	 }
    }
  }

  close $outf
  close $mapf

  #Close up everything, as normally in SolverProc.tcl
  Solve_do_Parms close 6

}
# end of Solv6_LoadSolution



# ____________________________________________________________________________#
# Procedures to open and close the dialog
# ____________________________________________________________________________#


#
# proc Solv6_ErrorCheck { confirm }
#-----------------------------------------------------------------------
# This routine verifies the user's input for correctness               #
# It returns a 0 if everything is Ok, else a 1                         #
# Will warn about overwriting files if confirm == 1                    #
# modified by CWS, 5/95                                                #
#-----------------------------------------------------------------------
proc Solv6_ErrorCheck { confirm } {

global ascSolv6Vect

    if { $ascSolv6Vect(dialogup) == 1 } {

#      if solver isn't QOMILP, set epsval and boval to nothing
       if {$ascSolv6Vect(solver) != "QOMILP"} {
           set ascSolv6Vect(epsval) ""
           set ascSolv6Vect(boval)  ""
       }

#      first make sure that the fill in numbers are empty or vaild numbers
       if {[string length $ascSolv6Vect(epsval)] != 0} {
          if {[catch {expr $ascSolv6Vect(epsval)}]}  {
             # need to release the grab
             #grab release .mps
             Solve_Alert "Input Error" "Termination Tolerance is not a number (or empty)."
             # turn it back on
             #grab set .mps
             return 1
          }
       }
       if {[string length $ascSolv6Vect(boval)] != 0}  {
          if {[catch {expr $ascSolv6Vect(boval)}]}  {
             # need to release the grab
             #grab release .mps
             Solve_Alert "Input Error" "Valid Bound is not a number (or empty)."
             # turn it back on
             #grab set .mps
             return 1
          }
       }
    }

#   error check the file name
    if {![Solv6_CleanName $ascSolv6Vect(filename) $confirm ]} {return 1}

    return 0

}

#
# proc Solve_OpenMakeMPS {}
#-----------------------------------------------------------------------
# popup the makeMPS window.                                            #
# modified by CWS, 5/95                                                #
#-----------------------------------------------------------------------
proc Solve_OpenMakeMPS {} {
  global ascSolvVect ascSolv6Vect

# dialogup is true when makeMPS dialog is visible
  set ascSolv6Vect(dialogup) 1

# position dialog box on the screen over the solver window
  set ascSolv6Vect(geometry) [osgpos 156x482[setpos .solver 229 42]]

# show the window
  ShowWindow.mps

# now that .mps.* widgets exist, we can set stuff about them
  switch $ascSolv6Vect(solver) {
      QOMILP     {Solv6_QOMILP_actions}
      lpsolve    {Solv6_lpsolve_actions}
      OSL        {Solv6_OSL_actions}
      SCICONIC   {Solv6_SCICONIC_actions}
      Generic    {Solv6_OSL_actions}
  }

#   reset dialog elements to blank if 0
    if {$ascSolv6Vect(bo) == 0 }    { set ascSolv6Vect(boval) {} }
    if {$ascSolv6Vect(eps) == 0 }   { set ascSolv6Vect(epsval) {} }

# note: the error alerts mess up the grab stuff
# don't let the user get away
#  grab set .mps

# hang out until the window goes away
  tkwait window .mps

}

#
# proc Solve_CloseMakeMPS {}
#----------------------------------------------------------------------------
# pop down the makeMPS window.                                              #
#----------------------------------------------------------------------------
proc Solve_CloseMakeMPS {} {
  global ascSolv6Vect

#  DEBUG:
#  puts "Starting Solve_CloseMakeMPS"

# the makeMPS dialog box fills out the values of :
#     ascSolv6Vect(SOS)
#     ascSolv6Vect(boval)
#     ascSolv6Vect(epsval)
#     ascSolv6Vect(filename)
#     ascSolv6Vect(semi)
#     ascSolv6Vect(solver)
#     ascSolv6Vect(relaxed)
#
# now, depending on the solver, you need to transform that into
# the correct set of subparameters

  switch $ascSolv6Vect(solver) {
     QOMILP {
	      set ascSolv6Vect(nonneg)   0
	      set ascSolv6Vect(obj)      3
	      set ascSolv6Vect(binary)   0
	      set ascSolv6Vect(integer)  0
	      set ascSolv6Vect(semi)     0
	      set ascSolv6Vect(sos1)     0
	      set ascSolv6Vect(sos2)     0
	      set ascSolv6Vect(sos3)     0
	      set ascSolv6Vect(bo)       1
	      set ascSolv6Vect(eps)      1

#             replace empty values with 0.0, and turn off bo or eps
              if {([string length $ascSolv6Vect(epsval)] == 0) &&  \
                  ($ascSolv6Vect(solver) == "QOMILP")} {
                    set ascSolv6Vect(eps) 0
                    set ascSolv6Vect(epsval) 0.0
              }
              if {([string length $ascSolv6Vect(boval)] == 0) &&  \
                  ($ascSolv6Vect(solver) == "QOMILP")} {
                    set ascSolv6Vect(bo) 0
                    set ascSolv6Vect(boval) 0.0
              }
     }

     lpsolve {
	      set ascSolv6Vect(nonneg)    1
	      set ascSolv6Vect(obj)       0
	      set ascSolv6Vect(binary)    0
	      set ascSolv6Vect(integer)   0
	      set ascSolv6Vect(semi)      0
	      set ascSolv6Vect(sos1)      0
	      set ascSolv6Vect(sos2)      0
	      set ascSolv6Vect(sos3)      0
	      set ascSolv6Vect(bo)        0
	      set ascSolv6Vect(eps)       0
	      set ascSolv6Vect(boval)     0
	      set ascSolv6Vect(epsval)    0
     }

     OSL     {
	      set ascSolv6Vect(nonneg)    0
	      set ascSolv6Vect(obj)       0
	      set ascSolv6Vect(binary)    0
	      set ascSolv6Vect(integer)   0
	      set ascSolv6Vect(semi)      0
	      set ascSolv6Vect(sos1)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(sos2)      0
	      set ascSolv6Vect(sos3)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(bo)        0
	      set ascSolv6Vect(eps)       0
	      set ascSolv6Vect(boval)     0
	      set ascSolv6Vect(epsval)    0
     }
     SCICONIC {
	      set ascSolv6Vect(nonneg)    0
	      set ascSolv6Vect(obj)       2
	      set ascSolv6Vect(binary)    1
	      set ascSolv6Vect(integer)   1
#	      set ascSolv6Vect(semi)      use current value
	      set ascSolv6Vect(sos1)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(sos2)      0
	      set ascSolv6Vect(sos3)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(bo)        0
	      set ascSolv6Vect(eps)       0
	      set ascSolv6Vect(boval)     0
	      set ascSolv6Vect(epsval)    0
     }
     Generic {
	      set ascSolv6Vect(nonneg)    0
	      set ascSolv6Vect(obj)       0
	      set ascSolv6Vect(binary)    0
	      set ascSolv6Vect(integer)   0
	      set ascSolv6Vect(semi)      0
	      set ascSolv6Vect(sos1)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(sos2)      0
	      set ascSolv6Vect(sos3)      $ascSolv6Vect(SOS)
	      set ascSolv6Vect(bo)        0
	      set ascSolv6Vect(eps)       0
	      set ascSolv6Vect(boval)     0
	      set ascSolv6Vect(epsval)    0
     }
  }

#   if the window is up, delete it
    if {$ascSolv6Vect(dialogup) == 1}  {DestroyWindow.mps}

    set ascSolv6Vect(dialogup) 0

}

# ____________________________________________________________________________#
# File name expansion and error checking
# ____________________________________________________________________________#

# little dialog that asks for conformation to overwrite
# yes =1, no = 0

proc Solv6_Overwrite {args} {

     global env

#    stick "and" between multiple files if any
     set name [join $args " and "]
#    see if it contains any ands
     if {[string first and $name] == -1} {
        set plural file
        set plural2 exists
        set plural3 it
     } else {
        set plural files
        set plural2 exist
        set plural3 them
     }

#    read in Yes/No dialog box template
     source "$env(ASCENDTK)/templates/Procedures/YesNoBox.t"

     global yesNoBox
     set yesNoBox(font) *time*18*

#    position the dialog with second parameter
     return [YesNoBox "The $plural $name $plural2. \nDo you want to overwrite $plural3?" \
            [osgpos 350x150[setpos .solver 15 55]]]

}


# This routine checks the filename the user enters for the mps file
# It does ~ expansion (although it always replaces ~anyuser with $HOME,
# it doen't do ~anyuser lookup), and it checks to see if the file already
# exists

# it returns 0 if an error occurs (name is invalid) or 1 if successful
# Parameters are name, the file name to clean up, and
# confirm, a boolean flag if the user should be asked if he wants to
# overwrite

proc Solv6_CleanName { name confirm } {

global ascSolv6Vect env

# only work on copy of name
set ascSolv6Vect(mpsname) $name

#  see if HOME is defined, so we can do tilde expansion
   if  {[info exists env(HOME)]} {

#       see if fileascSolv6Vect(mpsname) starts with a tilde, if so do expansion
        if {[string index $ascSolv6Vect(mpsname) 0] == "~"}  {

             set firstslash [string first "/" $ascSolv6Vect(mpsname)]
             if {$firstslash == -1} {
                 Solve_Alert "Input Error"  "The MPS file name $ascSolv6Vect(mpsname), contains a ~ but no / (e.g. ~/output.mps)"
                 return 0
             }

#            now we know everthing exists, so expand ascSolv6Vect(mpsname)
#            the slash stuff is to get rid of the cs76 if the
#            user enters ~cs76/test.mps

             set ascSolv6Vect(mpsname) $env(HOME)[string range $ascSolv6Vect(mpsname) $firstslash end]
       }
   }


#  add a .mps, if it doesn't already end with it
   if { [expr ![regexp {.mps$} $ascSolv6Vect(mpsname)]] }  {
        set ascSolv6Vect(mpsname) $ascSolv6Vect(mpsname).mps
   }

# get the map name from ascSolv6Vect(mpsname)  [i.e. *.map]
regsub {mps$} $ascSolv6Vect(mpsname) {map} ascSolv6Vect(mapname)

# get the error output name from ascSolv6Vect(mpsname)  [i.e. *.err]
regsub {mps$} $ascSolv6Vect(mpsname) {err} ascSolv6Vect(errname)

# get the regular output name from ascSolv6Vect(mpsname)  [i.e. *.out]
regsub {mps$} $ascSolv6Vect(mpsname) {out} ascSolv6Vect(outname)

#now see if the .mps or .map file exists, and get confirmation to overwrite
   if { [file exists $ascSolv6Vect(mpsname)] || [file exists $ascSolv6Vect(mapname)]}  {

      # check to make sure mps file is not a directory
      if {[file isdirectory $ascSolv6Vect(mpsname) ]}  {
           Solve_Alert "Input Error"  "The MPS file name $ascSolv6Vect(mpsname), is already the name of a directory"
           return 0
      }

      # check to make sure map file is not a directory
      if {[file isdirectory $ascSolv6Vect(mapname) ]}  {
           Solve_Alert "Input Error" "The variable map file name $ascSolv6Vect(mapname), is already the name of a directory"
           return 0
      }

      if { $confirm} {

	 # if both files exist, ask for confirmation to overwrite
	 if {[file isfile $ascSolv6Vect(mpsname) ] && [file isfile $ascSolv6Vect(mapname) ]}  {
              set result [Solv6_Overwrite $ascSolv6Vect(mpsname) $ascSolv6Vect(mapname)]
              if {!$result} {
        	 return 0
              }  else {
        	 return 1
              }
	 }

	 # if it's already a file, ask for confirmation to overwrite
	 if {[file isfile $ascSolv6Vect(mpsname) ]}  {
              set result [Solv6_Overwrite $ascSolv6Vect(mpsname)]
              if {!$result} { return 0 }
	 }

	 # if it's already a file, ask for confirmation to overwrite
	 if {[file isfile $ascSolv6Vect(mapname) ]}  {
              set result [Solv6_Overwrite $ascSolv6Vect(mapname)]
              if {!$result} { return 0 }
	 }

      }

   }

return 1

}
#end of Solv6_CleanName

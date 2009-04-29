# /************************************************************\  #
#		    Module: ascMonoEntry1.t
#                   written by Kirk Abbott
#                   Date: $Date: 1998/06/16 14:07:32 $
#                   Version: $Revision: 1.7 $
# \************************************************************/  #
#
# Module: ascMonoEntry1.t
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
#
# ascMonoEntry1id $Id: AscMonoEntry1.t,v 1.7 1998/06/16 14:07:32 ballan Exp $
#

# /************************************************************\  #
# AscMonoEntry1
# Generic EntryBox for one bit of information.
# Makes use of a global vector AscMonoEntry1
# Takes colors from internal resources
# Arguements :
# AscMonoEntry1Text  -- a label for the data -- default Variable Value
# AscMonoEntry1Geom  -- geometry  -- default 400x100
# AscMonoEntry1Info  -- information to be inserted in the box -- default ""
# AscMonoEntry1Label -- Command prompt -- default "Enter Value"
# Returns :
# Will leave the result in the global vector if Ok button is pressed or if
# the <Return> key is pressed when the focus is in the box, i.e will leave
# the number of the button pressed (1-3), and will attempt to invoke a
# command called OKCommand.ascMonoEntry1 or CancelCommand.ascEntry1.
# Will leave the value of the data in AscMonoEntry1Info.
# Will return also a formatted list of :the button number and the value of
# the data.This box is modal.
#
# user configurable properties
# AscMonoEntry1(font) -*-* (duh)
# dealing with the help button request is the callers problem.
# \************************************************************/  #

global AscMonoEntry1

set AscMonoEntry1(button) 0
set AscMonoEntry1(result) ""
set AscMonoEntry1(font) -*-* 
#
# procedures to show window .ascMonoEntry1
#
proc VShowWindow.ascMonoEntry1 {{AscMonoEntry1Text "Variable Value"} \
  {AscMonoEntry1Geom 400x100} \
  {AscMonoEntry1Info ""} \
  {AscMonoEntry1Label "Enter Value"} \
  {helpcommand "error no-help-yet sloppy-ui-author--not-a-bug"}} {
# xf ignore me 5

global AscMonoEntry1
  toplevel .ascMonoEntry1

  # Window manager configurations
  global tk_version
  wm positionfrom .ascMonoEntry1 ""
  wm sizefrom .ascMonoEntry1 ""
  wm minsize .ascMonoEntry1 400 100
  wm geometry .ascMonoEntry1 $AscMonoEntry1Geom
  wm title .ascMonoEntry1 "Enter $AscMonoEntry1Text"
  wm iconname .ascMonoEntry1 "Enter $AscMonoEntry1Text"


  # build widget .ascMonoEntry1.buttons_frm
  frame .ascMonoEntry1.buttons_frm


  # build widget .ascMonoEntry1.buttons_frm.ok_btn
  button .ascMonoEntry1.buttons_frm.ok_btn \
    -font $AscMonoEntry1(font) \
    -text {OK} \
    -width {10} \
    -command "
	global AscMonoEntry1
        set AscMonoEntry1(result) \
          \[.ascMonoEntry1.entry_frm.left_entry get\]
	OKCommand.ascMonoEntry1
	set AscMonoEntry1(button) 1
	catch {destroy .ascMonoEntry1}"

  # build widget .ascMonoEntry1.buttons_frm.cancel_btn
  button .ascMonoEntry1.buttons_frm.cancel_btn \
    -font $AscMonoEntry1(font) \
    -text {Cancel} \
    -width {10} \
    -command "
	global AscMonoEntry1
	CancelCommand.ascMonoEntry1
	set AscMonoEntry1(button) 2
	set AscMonoEntry1(result) {}
	catch {destroy .ascMonoEntry1}"

  # build widget .ascMonoEntry1.buttons_frm.help_btn
  button .ascMonoEntry1.buttons_frm.help_btn \
    -font $AscMonoEntry1(font) \
    -text {Help} \
    -width {10} \
    -command "
       global AscMonoEntry1
       set AscMonoEntry1(button) 3
       $helpcommand
       catch {destroy .ascMonoEntry1}"

  # pack widget .ascMonoEntry1.buttons_frm
  pack append .ascMonoEntry1.buttons_frm \
    .ascMonoEntry1.buttons_frm.ok_btn {left frame e expand} \
    .ascMonoEntry1.buttons_frm.cancel_btn {left frame center expand} \
    .ascMonoEntry1.buttons_frm.help_btn {left frame w expand}

  # build widget .ascMonoEntry1.entry_frm
  frame .ascMonoEntry1.entry_frm

  # build widget .ascMonoEntry1.entry_frm.left_entry
  entry .ascMonoEntry1.entry_frm.left_entry \
    -exportselection 0 \
    -font $AscMonoEntry1(font) \
    -width {33}
  #
  #Initial Entry
  .ascMonoEntry1.entry_frm.left_entry delete 0 end
  .ascMonoEntry1.entry_frm.left_entry insert end "$AscMonoEntry1Info"

  #bindings
  bind .ascMonoEntry1.entry_frm.left_entry <Return> "
    global AscMonoEntry1
    set AscMonoEntry1(result) \[%W get\]
    set AscMonoEntry1(button) 1
    OKCommand.ascMonoEntry1
    catch {destroy .ascMonoEntry1}"

  # build widget .ascMonoEntry1.entry_frm.value_label
  label .ascMonoEntry1.entry_frm.value_label \
    -width {15} \
    -font $AscMonoEntry1(font) \
    -text "$AscMonoEntry1Text"

  # build widget .ascMonoEntry1.entry_frm.bmcanvas
  set tl .ascMonoEntry1.entry_frm
  canvas $tl.bmcanvas -width 33 -height 27
  $tl.bmcanvas create bitmap 0 0 -background yellow \
     -anchor nw -bitmap grablock
  pack append $tl $tl.bmcanvas {left frame center}

  # pack widget .ascMonoEntry1.entry_frm
  pack append .ascMonoEntry1.entry_frm \
    .ascMonoEntry1.entry_frm.value_label {left frame center filly} \
    .ascMonoEntry1.entry_frm.left_entry {left frame center expand fill}

  if {[info exists AscMonoEntry1Label] && $AscMonoEntry1Label != ""} {
    # build widget .ascMonoEntry1.label_frm
    frame .ascMonoEntry1.label_frm

    # build widget .ascMonoEntry1.label_frm.box_label
    label .ascMonoEntry1.label_frm.box_label \
      -font $AscMonoEntry1(font) \
      -text "$AscMonoEntry1Label"

    # pack widget .ascMonoEntry1.label_frm
    pack append .ascMonoEntry1.label_frm \
      .ascMonoEntry1.label_frm.box_label {left frame center}

    pack append .ascMonoEntry1 \
      .ascMonoEntry1.label_frm {top frame center expand fill} \
  }
  # pack widget .ascMonoEntry1
  pack append .ascMonoEntry1 \
    .ascMonoEntry1.entry_frm {top frame center expand fill} \
    .ascMonoEntry1.buttons_frm {top frame center expand fill}

  #
  # Get Data and Catch response.
  #
  # wait for the box to be destroyed
  focus .ascMonoEntry1.entry_frm.left_entry
  update idletask
  bind .ascMonoEntry1 <Visibility> {ascKeepOnTop %W}
  grab .ascMonoEntry1
  tkwait window .ascMonoEntry1
  set data [list $AscMonoEntry1(button) $AscMonoEntry1(result)]
  return $data
} 


proc DestroyWindow.ascMonoEntry1 {} {# xf ignore me 5

    catch "destroy .ascMonoEntry1"
    update
}
proc OKCommand.ascMonoEntry1 {} {
  global AscMonoEntry1
}
proc CancelCommand.ascMonoEntry1 {} {
  global AscMonoEntry1
}
# eof
#










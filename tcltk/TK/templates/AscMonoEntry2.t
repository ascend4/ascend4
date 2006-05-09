# /************************************************************\  #
#		    Module: ascMonoEntry2.t
#                   written by Kirk Abbott
#                   Date:$Date: 1998/06/16 14:07:33 $
#                   Version:$Revision $
# \************************************************************/  #
#
# Module: ascMonoEntry2.t
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
#
# ascMonoEntry2id $Id: AscMonoEntry2.t,v 1.6 1998/06/16 14:07:33 ballan Exp $
# 

# /************************************************************\  #
# AscMonoEntry2
# Generic EntryBox for two pieces of information.
# Makes use of a global vector AscMonoEntry2
# Arguements :
# AscMonoEntry2Text  -- a label for the data -- default Variable Value
# AscMonoEntry2Geom  -- geometry  -- default 400x100
# AscMonoEntry2InfoL -- information to be inserted in left box -- default ""
# AscMonoEntry2InfoR -- information to be inserted in right box -- default ""
# Returns :
# Will leave the result in the global vector if Ok button is pressed or if
# the <Return> key is pressed when the focus is in any box, i.e will leave
# the number of the button pressed (1-3), and will attempt to invoke a
# command called OKCommand.ascMonoEntry2 or CancelCommand.ascEntry2.
# Will leave the values of the data in AscMonoEntry2InfoL and in 
# AscMonoEntry2InfoL for the left and right boxes respectively.
# Will return also a formatted list of : the button number, the left data
# and right data. This box is modal.
# Other :
# The tab key pressed in any entry box  will toggle between the left and
# right boxes.
# user configurable properties
# AscMonoEntry2(font) -*-* (duh)
# \************************************************************/  #


global AscMonoEntry2
set AscMonoEntry2(button) 0
set AscMonoEntry2(resultL) ""
set AscMonoEntry2(resultR) ""
set AscMonoEntry2(font) -*-* 

#
# procedures to show window .ascMonoEntry2
#
proc VShowWindow.ascMonoEntry2 { {AscMonoEntry2Text "Variable Value"}
  {AscMonoEntry2Geom 400x100} 
  {AscMonoEntry2InfoL ""} {AscMonoEntry2InfoR ""}} {

# xf ignore me 5

global AscMonoEntry2 
  toplevel .ascMonoEntry2

  # Window manager configurations
  global tk_version
  wm positionfrom .ascMonoEntry2 ""
  wm sizefrom .ascMonoEntry2 ""
  wm minsize .ascMonoEntry2 400 100
  wm geometry .ascMonoEntry2 $AscMonoEntry2Geom
  wm title .ascMonoEntry2 { }


  # build widget .ascMonoEntry2.buttons_frm
  frame .ascMonoEntry2.buttons_frm

  # build widget .ascMonoEntry2.buttons_frm.cancel_btn
  button .ascMonoEntry2.buttons_frm.cancel_btn \
    -font $AscMonoEntry2(font) \
    -text {Cancel} \
    -command "
	global AscMonoEntry2
	CancelCommand.ascMonoEntry2
	set AscMonoEntry2(button) 2
	set AscMonoEntry2(resultL) {}
	set AscMonoEntry2(resultR) {}
	catch {destroy .ascMonoEntry2}"

  # build widget .ascMonoEntry2.buttons_frm.help_btn
  button .ascMonoEntry2.buttons_frm.help_btn \
    -font $AscMonoEntry2(font) \
    -text {Help} \
    -command "
	global AscMonoEntry2
	set AscMonoEntry2(button) 3
	puts {Not yet implemented}"

  # build widget .ascMonoEntry2.buttons_frm.ok_btn
  button .ascMonoEntry2.buttons_frm.ok_btn \
    -font $AscMonoEntry2(font) \
    -text {OK} \
    -command "
	global AscMonoEntry2
        set AscMonoEntry2(resultL) \
          \[.ascMonoEntry2.entry_frm.left_entry get\]
        set AscMonoEntry2(resultR) \
          \[.ascMonoEntry2.entry_frm.right_entry get\]
	OKCommand.ascMonoEntry2
	set AscMonoEntry2(button) 1
	catch {destroy .ascMonoEntry2}"

  # pack widget .ascMonoEntry2.buttons_frm
  pack append .ascMonoEntry2.buttons_frm \
    .ascMonoEntry2.buttons_frm.ok_btn {left frame e expand} \
    .ascMonoEntry2.buttons_frm.cancel_btn {left frame center expand} \
    .ascMonoEntry2.buttons_frm.help_btn {left frame w expand}

  # build widget .ascMonoEntry2.entry_frm
  frame .ascMonoEntry2.entry_frm

  # build widget .ascMonoEntry2.entry_frm.left_entry
  entry .ascMonoEntry2.entry_frm.left_entry \
    -exportselection 0 \
    -font $AscMonoEntry2(font) \
    -width {25}
  #
  #Initial Entry
  .ascMonoEntry2.entry_frm.left_entry delete 0 end
  .ascMonoEntry2.entry_frm.left_entry insert end "$AscMonoEntry2InfoL"

  #bindings
  bind .ascMonoEntry2.entry_frm.left_entry <Return> "
    global AscMonoEntry2
    set AscMonoEntry2(resultL) \[.ascMonoEntry2.entry_frm.left_entry get\]
    set AscMonoEntry2(resultR) \[.ascMonoEntry2.entry_frm.right_entry get\]
    set AscMonoEntry2(button) 1
    OKCommand.ascMonoEntry2
    catch {destroy .ascMonoEntry2}"

  # build widget .ascMonoEntry2.entry_frm.right_entry
  entry .ascMonoEntry2.entry_frm.right_entry \
    -exportselection 0 \
    -font $AscMonoEntry2(font) \
    -width {25}
  #
  #Initial Entry
  .ascMonoEntry2.entry_frm.right_entry delete 0 end
  .ascMonoEntry2.entry_frm.right_entry insert end "$AscMonoEntry2InfoR"

  #bindings
  bind .ascMonoEntry2.entry_frm.right_entry <Return> "
    global AscMonoEntry2
    set AscMonoEntry2(resultL) \[.ascMonoEntry2.entry_frm.left_entry get\]
    set AscMonoEntry2(resultR) \[.ascMonoEntry2.entry_frm.right_entry get\]
    set AscMonoEntry2(button) 1
    OKCommand.ascMonoEntry2
    catch {destroy .ascMonoEntry2}"

  # pack widget .ascMonoEntry2.entry_frm
  pack append .ascMonoEntry2.entry_frm \
    .ascMonoEntry2.entry_frm.left_entry {left frame ne pady 4 expand filly} \
    .ascMonoEntry2.entry_frm.right_entry {right frame nw pady 4 expand filly}

  # build widget .ascMonoEntry2.label_frm
  frame .ascMonoEntry2.label_frm 

  # build widget .ascMonoEntry2.label_frm.box_label
  label .ascMonoEntry2.label_frm.box_label \
    -font $AscMonoEntry2(font) \
    -text "$AscMonoEntry2Text"

  # build widget .ascMonoEntry2.label_frm.bmcanvas
  set tl .ascMonoEntry2.label_frm
  canvas $tl.bmcanvas -width 33 -height 27
  $tl.bmcanvas create bitmap 0 0 -background yellow \
     -anchor nw -bitmap grablock
  pack append $tl $tl.bmcanvas {left frame center}

  # pack widget .ascMonoEntry2.label_frm
  pack append .ascMonoEntry2.label_frm \
    .ascMonoEntry2.label_frm.box_label {top frame center expand fillx}

  # pack widget .ascMonoEntry2
  pack append .ascMonoEntry2 \
    .ascMonoEntry2.label_frm {top frame center expand fill} \
    .ascMonoEntry2.entry_frm {top frame center expand fill} \
    .ascMonoEntry2.buttons_frm {top frame center expand fill}

  #
  # Get Data and Catch response.
  #
  # wait for the box to be destroyed
  focus .ascMonoEntry2.entry_frm.left_entry
  update idletask
  bind .ascMonoEntry2 <Visibility> {ascKeepOnTop %W}
  grab .ascMonoEntry2
  tkwait window .ascMonoEntry2
  set data  [list $AscMonoEntry2(button) \
	$AscMonoEntry2(resultL) $AscMonoEntry2(resultR)]
  return $data
}

proc DestroyWindow.ascMonoEntry2 {} {# xf ignore me 7

    catch "destroy .ascMonoEntry2"
    update
}
proc OKCommand.ascMonoEntry2 {} {
  global AscMonoEntry2
}
proc CancelCommand.ascMonoEntry2 {} {
  global AscMonoEntry2
}

# eof
#


# Program: ascListSelectBox.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
# Ben Allan may 19 1994 ballan@cs.cmu.edu
# $Revision: 1.10 $
# $Date: 1998/06/11 14:10:17 $
#
# requires ascKeepOnTop.tcl if a grabby selection is used
# 

global ascListSelectBox
set ascListSelectBox(button) 0
set ascListSelectBox(itemselected) ""
set ascListSelectBox(selectmode) single
set ascListSelectBox(toplevelname) ".asclistselect"
set ascListSelectBox(grab) 0
set ascListSelectBox(btn3name) Help
set ascListSelectBox(btn3command) {puts "Help pushed"}
set ascListSelectBox(btn3destroy) 0
set ascListSelectBox(btn4name) ""
set ascListSelectBox(btn4command) ""
set ascListSelectBox(btn4destroy) 0
set ascListSelectBox(btn5name) ""
set ascListSelectBox(btn5command) ""
set ascListSelectBox(btn5destroy) 0
set ascListSelectBox(font) -*-*
set ascListSelectBox(title) ListSelect
set ascListSelectBox(headline) "Choices:"
#
# proc AscListSelectBox {ascListSelectBoxInfo {ascListSelectBoxGeom 250x240}}
#---------------------------------------------------------------------------
# pretty descriptive name, eh?
# Returns number of button pressed, also setting array button to that value.
# Item(s) selected will be in ascListSelectBox(itemselected).
# The values of
# ascListSelectBox(selectmode) (as in listbox widgets)
# ascListSelectBox(grab)
# ascListSelectBox(toplevelname)
# ascListSelectBox(title) (window manager title)
# ascListSelectBox(headline) (box heading)
# ascListSelectBox(btn3name)
# ascListSelectBox(btn3command) should be a procedure name.
# ascListSelectBox(btn3destroy)
# ascListSelectBox(btn4name)
# ascListSelectBox(btn4command) should be a procedure name.
# ascListSelectBox(btn4destroy)
# ascListSelectBox(btn5name)
# ascListSelectBox(btn5command) should be a procedure name.
# ascListSelectBox(btn5destroy)
# are consulted at build time.
# If another box with the same toplevel name
#  exists,  build is aborted and cancel button is returned.
# If grab is true, the box will have a grab on it.
# grab or no grab, this beast may commandeer the command line.
# If btn3/4/5name is "", no btn3/4/5 will be built
# If btn3/4/5destroy is 1,
#  btn3/4/5 will do its command and destroy the window,else
#  btn3/4/5 will just do its command. In either case, the selection will be
#  in ascListSelectBox(itemselected) when btn3/4/5command is called.
# $btn3/4/5command should not require arguments.
# It is a good idea to set the  ascListSelectBox array every time
#  you call this, as others are likely using it also.
# Widget configurables:
# ascListSelectBox(font) -*-* (duh)
#---------------------------------------------------------------------------
proc AscListSelectBox {ascListSelectBoxInfo {ascListSelectBoxGeom 250x240}} {

  global ascListSelectBox

 # if no list or conflicting toplevel, return cancel
  if {$ascListSelectBoxInfo == "" || \
     [winfo exists $ascListSelectBox(toplevelname)]} {
    set ascListSelectBox(button) 2
    set ascListSelectBox(itemselected) ""
    catch {wm deiconify $ascListSelectBox(toplevelname)}
    catch {raise $ascListSelectBox(toplevelname)}
    return 2
  }
  set tl $ascListSelectBox(toplevelname)
  # build widget
  toplevel $tl

  # Window manager configurations
  #global tk_version
  wm positionfrom $tl user
  wm sizefrom $tl user
  wm minsize $tl 250 240
  wm geometry $tl $ascListSelectBoxGeom
  wm title $tl $ascListSelectBox(title)

  # build widget $tl.buttons_frm
  frame $tl.buttons_frm \
    -borderwidth 0

  # build widget $tl.buttons_frm.cancel_button
  button $tl.buttons_frm.cancel_button \
    -font $ascListSelectBox(font) \
    -text Cancel \
    -width 7 \
    -command "
	global ascListSelectBox
        set ascListSelectBox(itemselected) \"\"
	set ascListSelectBox(button) 2
  	destroy $tl"

  # build widget $tl.buttons_frm.btn3
  if {$ascListSelectBox(btn3name)!=""} {
    button $tl.buttons_frm.btn3 \
      -borderwidth 2 \
      -font $ascListSelectBox(font) \
      -text $ascListSelectBox(btn3name) \
      -width [string length $ascListSelectBox(btn3name)] \
      -command "
         global ascListSelectBox
         set ascListSelectBox(button) 3
         ascLSOKCommand $tl
         $ascListSelectBox(btn3command)"
    if {$ascListSelectBox(btn3destroy)} {
      $tl.buttons_frm.btn3 config -command "
         global ascListSelectBox
         set ascListSelectBox(button) 3
         ascLSOKCommand $tl
         $ascListSelectBox(btn3command)
         destroy $tl"
    }
  }

  # build widget $tl.buttons_frm.btn4
  if {$ascListSelectBox(btn4name)!=""} {
    button $tl.buttons_frm.btn4 \
      -borderwidth 2 \
      -font $ascListSelectBox(font) \
      -text $ascListSelectBox(btn4name) \
      -width [string length $ascListSelectBox(btn4name)] \
      -command "
         global ascListSelectBox
         set ascListSelectBox(button) 4
         ascLSOKCommand $tl
         $ascListSelectBox(btn4command)"
    if {$ascListSelectBox(btn4destroy)} {
      $tl.buttons_frm.btn4 config -command "
         global ascListSelectBox
         set ascListSelectBox(button) 4
         ascLSOKCommand $tl
         $ascListSelectBox(btn4command)
         destroy $tl"
    }
  }
  # build widget $tl.buttons_frm.btn5
  if {$ascListSelectBox(btn5name)!=""} {
    button $tl.buttons_frm.btn5 \
      -borderwidth 2 \
      -font $ascListSelectBox(font) \
      -text $ascListSelectBox(btn5name) \
      -width [string length $ascListSelectBox(btn5name)] \
      -command "
         global ascListSelectBox
         set ascListSelectBox(button) 5
         ascLSOKCommand $tl
         $ascListSelectBox(btn5command)"
    if {$ascListSelectBox(btn5destroy)} {
      $tl.buttons_frm.btn5 config -command "
         global ascListSelectBox
         set ascListSelectBox(button) 5
         ascLSOKCommand $tl
         $ascListSelectBox(btn5command)
         destroy $tl"
    }
  }
  # build widget $tl.buttons_frm.ok_button
  button $tl.buttons_frm.ok_button \
    -borderwidth 2 \
    -text {OK} \
    -font $ascListSelectBox(font) \
    -width 3 \
    -command "
       global ascListSelectBox
       ascLSOKCommand $tl
       set ascListSelectBox(button) 1
       destroy $tl"

  # pack widget $tl.buttons_frm
  pack append $tl.buttons_frm \
    $tl.buttons_frm.ok_button {left frame center expand fill} \
    $tl.buttons_frm.cancel_button {left frame center expand fill}
  if {$ascListSelectBox(btn3name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn3 {left frame center expand fill}
  }
  if {$ascListSelectBox(btn4name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn4 {left frame center expand fill}
  }
  if {$ascListSelectBox(btn5name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn5 {left frame center expand fill}
  }

  # build widget $tl.lbl_frm
  frame $tl.lbl_frm

  # build widget $tl.lbl_frm.main_label
  label $tl.lbl_frm.main_label \
    -text $ascListSelectBox(headline)

  # pack widget $tl.lbl_frm
  if {$ascListSelectBox(grab)} {
    canvas $tl.lbl_frm.bmcanvas -width 33 -height 27
    $tl.lbl_frm.bmcanvas create bitmap 0 0 \
       -background yellow \
       -anchor nw \
       -bitmap grablock
    pack append $tl.lbl_frm $tl.lbl_frm.bmcanvas {left frame w}
  }

  pack append $tl.lbl_frm \
    $tl.lbl_frm.main_label {top frame center pady 5 fillx}

  # build widget $tl.main_frm
  frame $tl.main_frm

  # build widget $tl.main_frm.lb_frame
  frame $tl.main_frm.lb_frame

  # build widget $tl.main_frm.lb_frame.scrollbar2
  scrollbar $tl.main_frm.lb_frame.scrollbar2 \
    -command "$tl.main_frm.lb_frame.listbox1 yview"

  # build widget $tl.main_frm.lb_frame.listbox1
  listbox $tl.main_frm.lb_frame.listbox1 \
    -exportselection 0 \
    -font $ascListSelectBox(font) \
    -selectmode $ascListSelectBox(selectmode) \
    -width 20 \
    -height 5 \
    -relief {sunken} \
    -yscrollcommand "$tl.main_frm.lb_frame.scrollbar2 set"

  # pack widget $tl.main_frm.lb_frame
  pack append $tl.main_frm.lb_frame \
    $tl.main_frm.lb_frame.scrollbar2 {right frame center filly} \
    $tl.main_frm.lb_frame.listbox1 {top frame center expand fill}

  # pack widget $tl.main_frm
  pack append $tl.main_frm \
    $tl.main_frm.lb_frame {top frame center expand fill}

  # pack widget $tl
  pack append $tl \
    $tl.lbl_frm {top frame center pady 5 fillx} \
    $tl.main_frm {top frame center expand fill} \
    $tl.buttons_frm {top frame center fill}

  proc DestroyWindow$tl {} "
    destroy $tl
    update"

  # Stuff the information given.
  if {$ascListSelectBoxInfo != ""} {
    foreach item $ascListSelectBoxInfo {
      $tl.main_frm.lb_frame.listbox1 insert end $item
    }
  }

  update idletask
  if {$ascListSelectBox(grab)} {
    bind $tl <Visibility> {ascKeepOnTop %W}
    grab $tl
  }
  # wait for the box to be destroyed
  tkwait window $tl
  return $ascListSelectBox(button)
}

proc ascLSOKCommand {tl} {
  global ascListSelectBox
  set ascListSelectBox(itemselected) \
    [ascGetListSelection $tl.main_frm.lb_frame.listbox1]
}
 #ifndef
if {[info proc ascGetListSelection]!="ascGetListSelection"} {
  #
  # returns the selection from a listbox widget as a list.
  proc ascGetListSelection {w} {
    set indices [$w curselection]
    set a ""
    foreach i $indices {
      lappend a [$w get $i]
    }
    return $a
  }
}

# eof

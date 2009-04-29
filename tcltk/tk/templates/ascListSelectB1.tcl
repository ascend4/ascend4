# Program: ascListSelectB1Box.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
# Ben Allan may 22 1996 ballan@cs.cmu.edu
# $Revision: 1.10 $
# $Date: 1998/06/11 14:10:24 $
# This box is like the ascListSelectB1Box.tcl file
# except that instead of OK and Cancel buttons it has just 1
# Dismiss button.
#
# requires ascKeepOnTop.tcl if a grabby selection is used
# 

global ascListSelectB1Box
set ascListSelectB1Box(button) 0
set ascListSelectB1Box(itemselected) ""
set ascListSelectB1Box(selectmode) browse
set ascListSelectB1Box(toplevelname) ".asclistselect"
set ascListSelectB1Box(grab) 0
set ascListSelectB1Box(btn2name) Help
set ascListSelectB1Box(btn2command) {puts "Help pushed"}
set ascListSelectB1Box(btn2destroy) 0
set ascListSelectB1Box(btn3name) ""
set ascListSelectB1Box(btn3command) ""
set ascListSelectB1Box(btn3destroy) 0
set ascListSelectB1Box(btn4name) ""
set ascListSelectB1Box(btn4command) ""
set ascListSelectB1Box(btn4destroy) 0
set ascListSelectB1Box(btn5name) ""
set ascListSelectB1Box(btn5command) ""
set ascListSelectB1Box(btn5destroy) 0
set ascListSelectB1Box(font) -*-*
set ascListSelectB1Box(title) ListSelect
set ascListSelectB1Box(headline) "Choices:"
#
# proc AscListSelectB1SelectAll {{toplevel ""}}
#---------------------------------------------------------------------------
# tags with sel tag everything in the list box for the toplevel
# given, which must be one of these AscListSelectB1Boxes.
# if toplevel not given, box in ascListSelectB1Box(toplevelname)
# will be tagged if it exists.
#---------------------------------------------------------------------------
proc AscListSelectB1SelectAll {{toplevel ""}} {
  global ascListSelectB1Box
  if {$toplevel == ""} {
    set toplevel "$ascListSelectB1Box(toplevelname)"
  }
  $toplevel.main_frm.lb_frame.listbox1 select set 0 end
}
#
# proc AscListSelectB1Box {ascListSelectB1BoxInfo \
# {ascListSelectB1BoxGeom 250x240}}
#---------------------------------------------------------------------------
# pretty descriptive name, eh?
# Returns number of button pressed, also setting array button to that value.
# Item(s) selected will be in ascListSelectB1Box(itemselected).
# The values of
# ascListSelectB1Box(selectmode)
# ascListSelectB1Box(grab)
# ascListSelectB1Box(toplevelname)
# ascListSelectB1Box(title) (window manager title)
# ascListSelectB1Box(headline) (box heading)
# ascListSelectB1Box(btn2name)
# ascListSelectB1Box(btn2command) should be a procedure name.
# ascListSelectB1Box(btn2destroy)
# ascListSelectB1Box(btn3name)
# ascListSelectB1Box(btn3command) should be a procedure name.
# ascListSelectB1Box(btn3destroy)
# ascListSelectB1Box(btn4name)
# ascListSelectB1Box(btn4command) should be a procedure name.
# ascListSelectB1Box(btn4destroy)
# ascListSelectB1Box(btn5name)
# ascListSelectB1Box(btn5command) should be a procedure name.
# ascListSelectB1Box(btn5destroy)
# are consulted at build time.
# If another box with the same toplevel name
#  exists,  build is aborted and cancel button is returned.
# If grab is true, the box will have a grab on it.
# grab or no grab, this beast may commandeer the command line.
# If btn2/3/4/5name is "", no btn2/3/4/5 will be built
# If btn2/3/4/5destroy is 1,
#  btn2/3/4/5 will do its command and destroy the window,else
#  btn2/3/4/5 will just do its command. In either case, the selection will be
#  in ascListSelectB1Box(itemselected) when btn2/3/4/5command is called.
# $btn2/3/4/5command should not require arguments.
# It is a good idea to set the  ascListSelectB1Box array every time
#  you call this, as others are likely using it also.
# Widget configurables:
# ascListSelectB1Box(font) -*-* (duh)
#---------------------------------------------------------------------------
proc AscListSelectB1Box {ascListSelectB1BoxInfo {ascListSelectB1BoxGeom 250x240}} {

  global ascListSelectB1Box

 # if no list or conflicting toplevel, return cancel
  if {$ascListSelectB1BoxInfo == ""} {
    set ascListSelectB1Box(button) 2
    set ascListSelectB1Box(itemselected) ""
    catch {
      wm deiconify $ascListSelectB1Box(toplevel);
      raise $ascListSelectB1Box
    }
    return 2
  }
  if {[winfo exists $ascListSelectB1Box(toplevelname)]} {
    destroy $ascListSelectB1Box(toplevelname)
  }
  set tl $ascListSelectB1Box(toplevelname)
  # build widget
  toplevel $tl

  # Window manager configurations
  #global tk_version
  wm positionfrom $tl user
  wm sizefrom $tl user
  wm minsize $tl 250 240
  wm geometry $tl $ascListSelectB1BoxGeom
  wm title $tl $ascListSelectB1Box(title)

  # build widget $tl.buttons_frm
  frame $tl.buttons_frm \
    -borderwidth 0

  # build widget $tl.buttons_frm.btn2
  if {$ascListSelectB1Box(btn2name)!=""} {
    button $tl.buttons_frm.btn2 \
      -borderwidth 1 \
      -font $ascListSelectB1Box(font) \
      -text $ascListSelectB1Box(btn2name) \
      -padx 0 \
      -width [string length $ascListSelectB1Box(btn2name)] \
      -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 2
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn2command)"
    if {$ascListSelectB1Box(btn2destroy)} {
      $tl.buttons_frm.btn2 config -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 2
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn2command)
         destroy $tl"
    }
  }

  # build widget $tl.buttons_frm.btn3
  if {$ascListSelectB1Box(btn3name)!=""} {
    button $tl.buttons_frm.btn3 \
      -font $ascListSelectB1Box(font) \
      -text $ascListSelectB1Box(btn3name) \
      -width [string length $ascListSelectB1Box(btn3name)] \
      -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 3
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn3command)"
    if {$ascListSelectB1Box(btn3destroy)} {
      $tl.buttons_frm.btn3 config -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 3
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn3command)
         destroy $tl"
    }
  }

  # build widget $tl.buttons_frm.btn4
  if {$ascListSelectB1Box(btn4name)!=""} {
    button $tl.buttons_frm.btn4 \
      -font $ascListSelectB1Box(font) \
      -text $ascListSelectB1Box(btn4name) \
      -padx 0 \
      -width [string length $ascListSelectB1Box(btn4name)] \
      -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 4
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn4command)"
    if {$ascListSelectB1Box(btn4destroy)} {
      $tl.buttons_frm.btn4 config -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 4
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn4command)
         destroy $tl"
    }
  }
  # build widget $tl.buttons_frm.btn5
  if {$ascListSelectB1Box(btn5name)!=""} {
    button $tl.buttons_frm.btn5 \
      -borderwidth 1 \
      -font $ascListSelectB1Box(font) \
      -text $ascListSelectB1Box(btn5name) \
      -padx 0 \
      -width [string length $ascListSelectB1Box(btn5name)] \
      -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 5
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn5command)"
    if {$ascListSelectB1Box(btn5destroy)} {
      $tl.buttons_frm.btn5 config -command "
         global ascListSelectB1Box
         set ascListSelectB1Box(button) 5
         ascLSB1OKCommand $tl
         $ascListSelectB1Box(btn5command)
         destroy $tl"
    }
  }
  # build widget $tl.buttons_frm.ok_button
  button $tl.buttons_frm.ok_button \
    -borderwidth 1 \
    -text {Dismiss} \
    -font $ascListSelectB1Box(font) \
    -padx 0 \
    -width 7 \
    -command "
       global ascListSelectB1Box
       ascLSB1OKCommand $tl
       set ascListSelectB1Box(button) 1
       destroy $tl"

  # these did have center expand fill before
  # pack widget $tl.buttons_frm
  pack append $tl.buttons_frm \
    $tl.buttons_frm.ok_button {left frame center fill}
  if {$ascListSelectB1Box(btn2name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn2 {left frame center fill}
  }
  if {$ascListSelectB1Box(btn3name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn3 {left frame center fill}
  }
  if {$ascListSelectB1Box(btn4name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn4 {left frame center fill}
  }
  if {$ascListSelectB1Box(btn5name)!=""} {
    pack append $tl.buttons_frm \
      $tl.buttons_frm.btn5 {left frame center fill}
  }

  # build widget $tl.lbl_frm
  frame $tl.lbl_frm

  # build widget $tl.lbl_frm.main_label
  label $tl.lbl_frm.main_label \
    -borderwidth {0} \
    -text $ascListSelectB1Box(headline)

  # pack widget $tl.lbl_frm
  if {$ascListSelectB1Box(grab)} {
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
    -font $ascListSelectB1Box(font) \
    -selectmode $ascListSelectB1Box(selectmode) \
    -width 30 \
    -height 5 \
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
  if {$ascListSelectB1BoxInfo != ""} {
    foreach item $ascListSelectB1BoxInfo {
      $tl.main_frm.lb_frame.listbox1 insert end $item
    }
  }

  update idletask
  if {$ascListSelectB1Box(grab)} {
    bind $tl <Visibility> {ascKeepOnTop %W}
    grab $tl
  }
  # wait for the box to be destroyed
  tkwait window $tl
  return $ascListSelectB1Box(button)
}

proc ascLSB1OKCommand {tl} {
  global ascListSelectB1Box
  set ascListSelectB1Box(itemselected) \
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

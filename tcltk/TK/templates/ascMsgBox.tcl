# ascMsgBox.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
# ----------------------------------------------------------------------
# multitline font independent self sizing, no response caught message
# widget.
# if you really wanta grab, you can have a grab.
# just has a dismiss button.
# Ben Allan May 16, 1994
# $Revision: 1.5 $
# $Date: 1998/04/25 18:18:04 $
#
# global configurables:
# ascMsgVect(grab) (0 (none)  or 1)
# ascMsgVect(btn_font)
# ascMsgVect(lbl_font)
# ascMsgVect(btn_label)
# ascMsgVect(toplevelname)
# ascMsgVect(title)
# ascMsgVect(position) +x+y only. size is determined internally.
# ----------------------------------------------------------------------

global ascMsgVect
set ascMsgVect(grab) 0
set ascMsgVect(btn_font) -*-*
set ascMsgVect(lbl_font) -*-*
set ascMsgVect(btn_label) OK
set ascMsgVect(toplevelname) .msg
set ascMsgVect(title) Message:
set ascMsgVect(position) +400+200

#
# proc ascMsgBox {mesg args} {
# ----------------------------------------------------------------------
# mesg is a string with or without newlines in it.
# The string will be broken at newlines and displayed accordingly.
# If you don't put in newlines, you will get a snake.
# nothing is done with args
# If a box is already up with the same toplevel name, it is redrawn,
# and the screen position is taken from the old one.
# ----------------------------------------------------------------------
proc ascMsgBox {mesg args} {
  global ascMsgVect
  if {$ascMsgVect(toplevelname)==""} {set ascMsgVect(toplevelname) .msg}
  set tl $ascMsgVect(toplevelname)
  # build widget $tl or restuff it
  if {[winfo exists $tl]} {
    foreach i [winfo children $tl] { catch {destroy $i} }
  } else {
    toplevel $tl
    # Window manager configurations
    global tk_version
    wm positionfrom $tl user
    wm geometry $tl $ascMsgVect(position)
  }
  wm title $tl $ascMsgVect(title)
  wm sizefrom $tl user

  if {$ascMsgVect(grab)} { 
    catch {destroy $tl.bmcanvas}
    canvas $tl.bmcanvas -width 33 -height 27
    $tl.bmcanvas create bitmap 0 0 -background yellow \
      -anchor nw -bitmap grablock
    pack append $tl $tl.bmcanvas {top frame center expand fillx}
  }

  # build widget $tl.label set
  set labels [split $mesg \n]
  set nl [llength $labels]
  set maxw 0
  foreach i $labels {
    if {[string length $i] > $maxw} {set maxw [string length $i]}
  }
  for {set i 0} {$i < $nl} {incr i} {
    label $tl.label$i \
      -text "[lindex $labels $i]" \
      -font $ascMsgVect(lbl_font) \
      -width $maxw
    pack append $tl $tl.label$i {top frame center expand fill}
  }
  # build widget $tl.ok_btn
  button $tl.ok_btn \
    -text $ascMsgVect(btn_label) \
    -font $ascMsgVect(btn_font) \
    -command "destroy $tl"
  # pack widget $tl
  pack append $tl $tl.ok_btn {top frame center expand fill}
  if {$ascMsgVect(grab)} { catch {grab set $tl} }
}

# eof
#


#
# a function to keep a window unobscured.
# Whatever widget this is bound to, it keeps the associated toplevel on top.
# By associated we mean for the window .a.b.c, the toplevel is .a
# and if .a.b.c ismapped, we keep it that way.
# Use this carefully or you may get into a race condition where
# two windows keep obscuring each other. Not normally a problem in
# well designed dialog sequences.
#
# Children of the toplevel (children meaning 'by name', so popups count)
# are not considered to obscure their toplevel.
#
# Most 'standard dialogs' are a toplevel.
#
 # ifndef ascKeepOnTop
if {[string compare [info proc ascKeepOnTop] ""] == 0} {
  proc ascKeepOnTop {w} {
    if {![winfo exists $w] || ![winfo ismapped $w]} {return}
    set tl [winfo toplevel $w]
    set g [winfo geometry $tl]
    scan $g "%dx%d+%d+%d" w h x y
    # shrink into the window a little
    incr x
    incr y
    incr w -2
    incr h -2
    set c1 "[winfo containing $x $y]"
    set c2 "[winfo containing [expr $x+$w] $y]"
    set c3 "[winfo containing $x [expr $y+$h]]"
    set c4 "[winfo containing [expr $x+$w] [expr $y+$h]]"
    if {$c1 == "" || $c2 == "" || $c3 == "" || $c4 == "" ||
        [asctoplevelname $c1] != $tl ||
        [asctoplevelname $c2] != $tl ||
        [asctoplevelname $c4] != $tl ||
        [asctoplevelname $c4] != $tl} {
      raise $tl
    }
  }

  # returns the first part of the widget name
  # or the empty string if the widget name is not well formed
  # or the toplevel implied by the first part does not exist.
  #
  proc asctoplevelname {w} {
    return "[winfo toplevel $w]"
 #  if {![winfo exists $w] || [string index $w 0] != "."} {return ""}
 #  set tl [lindex [split $w .] 1]
 #  if {![winfo exists .$tl]} {return ""}
 #  set cl [lindex [.$tl configure -class] 3]
 #  if {$cl != "Toplevel"} {return ""}
 #  return .$tl
  }
}

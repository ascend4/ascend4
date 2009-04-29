# ascparm.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
# ----------------------------------------------------------------------
# Flexible parameters (real, string, int, bool, bin) entry window with sanity
# checking built in.
# May 2, 1994. Benjamin Allan.
# Given a global array variable, build an appropriate window to get good
# data.
# Error handling is done in 2 associated procedures (ascParSane,
# ascParPageClose using Tk builtin function error. This may be revised
# to suit your application.
# Note: I have incorporated the MenuPopup bindings (under local names)
# so that this is stand alone. the standard bindings found in xf for
# popups have been renamed to AP_MenuPop* from MenuPop*. If the menu
# behavior in Tk 4.x is changed, these routines need to be updated.
# updated 7/97 ben allan.
# I have also added a call to ascKeepOnTop, which is applied to the
# parameter page if grab is specified.
# $Revision: 1.23 $
# $Date: 1999/06/22 00:44:18 $
# ----------------------------------------------------------------------


global ascParPageVect
 # set demo vars
set ascParPageVect(namelist) "mywstring mystringlist myreallylonglonglongbool myreal myint mybin"
set ascParPageVect(title) "My_window_title"
set ascParPageVect(toplevel) ".mywindow"
set ascParPageVect(maxlines) "10"
set ascParPageVect(npages) "2"
set ascParPageVect(myint) 2
set ascParPageVect(myint.lo) 2
set ascParPageVect(myint.type) int
set ascParPageVect(mybin) 2
set ascParPageVect(mybin.hi) 3
set ascParPageVect(mybin.type) bin
set ascParPageVect(myreallylonglonglongbool) 1
set ascParPageVect(myreallylonglonglongbool.page) 2
set ascParPageVect(myreallylonglonglongbool.type) bool
set ascParPageVect(mystringlist.page) 2
set ascParPageVect(myreal) 1.34e-2
set ascParPageVect(myreal.type) real
set ascParPageVect(mywstring) "bite me!"
set ascParPageVect(mywstring.choices) ""
set ascParPageVect(mystringlist) "bite me!"
set ascParPageVect(mystringlist.choices) "fred barney"
set ascParPageVect(grab) "1"
set ascParPageVect(helpcommand) "puts no_help"
set ascParPageVect(whenokcommand) "puts ok_pressed"
set ascParPageVect(usercheckcommand) "puts userchecked"

#
# proc ascParPage {{PA} {position "+10+10"} {page 1} {sort 1} {first 1}}
# ----------------------------------------------------------------------
# Selfdemo:  ascParPage ascParPageVect
#  This procedure creates a window of entryL's and toggle buttons to
#  get user input for the vector {PA}. {PA} has to know a little
#  about itself and how it should be windowed, for example:
#
#  ${PA}(namelist) "myint mybool myreal mywstring mystringlist"
#  ${PA}(title) "My_window_title"
#  ${PA}(toplevel) ".mywindow"
#  ${PA}(maxlines) "10"
#  this is a per page limit
#  ${PA}(npages) "1";#  max we will look at, overrides maxlines
#  ${PA}(grab) "0" ;# should the window block the application
#  ${PA}(cancellable) ;# should a cancel button be offered. If so, then
#                     changes do not automatically occur as they would
#                     without a cancel option.
#                     If cancellable and the user selects cancel,
#                     usercheckcommand (below) is not used and $PA(__,cancelled)
#                     Is set to 1. The client need not reset PA(__,cancelled).
#  ${PA}(usercheckcommand) "puts nousercheck"
#  called with the name of the array after all the range checks are passed.
#  If the user detects something not good about the array state, they
#  should exit their checking function by a call to tcl error with an
#  appropriate message. Supplying a usercheckcommand is optional.
#  ${PA}(whenokcommand) "puts values_in_bounds"
#  done when input checks
#  ${PA}(helpcommand) "puts no_help"
#  done when help is pushed
#
#  If ${PA}(onesize) is true, all pages will be as long as the longest page,
#  otherwise the window will resize itself depending on item count on that
#  page. The pages will vary by a few pixels if the number of booleans on the
#  page is different.
#
#  If cancellable is not set, it is assumed to be 1.
#
#  If these are not set, dumb defaults will be applied for all.
#  If you specify garbage for the above, the window may not build
#  correctly. This is your fault.
#
#  position should be any string valid for the wm geometry command in tk
#  page is the page to displaying, if there are more than 1 pages
#
#  If sort is 1, each page will be ordered int/real/string/bin/bool,
#  else the natural order given by namelist will be preserved.
#
#  Each element in ${PA}(namelist) will be given a line in the
#  input window and used as a widget name, so each should start with
#  a lowercase letter. On exit, ${PA}(name) will be assigned a value
#  from the user input.
#  Bounds are applied to real and integer variables and error messages
#  generated sometime before the whenokcommand is executed. Similarly, strings
#  are checked against a choice list if one is provided.
#  This check/execute cycle takes place when the user pushes the ok button.
#  The window is destroyed after check is passed and before whenokcommand
#  is called. Arguments must be sane before window is closed, but if the
#  window is up with a bad value, any place else accessing the same
#  global variable will still see the bad value. Grab may be of use in
#  reducing this kind of error.
#
#  Input format is controlled by the following modifiers in ${PA}:
#  ${PA}($name.type) [bool,int,real,string]
#  ${PA}($name.lo) ${PA}($name.hi)  input range limits for real,int
#  ${PA}($name.choices) list of valid strings for $name. if null, any
#  string value is valid.
#  ${PA}($name.label) descriptive text for widget. if null, label :=name
#  ${PA}($name.help) balloon help for name. optional.
#  ${PA}($name.page) number of page a variable should be displayed on,
#  if $name.page > ${PA}(npages), will be reset to the last page
#  ***The sanity of type, lo, hi, choices will not be checked if they are ***
#  specified. Some defaults will be assumed if they left unspecified.
#  Defaults: type- string, lo- -infinity, hi- +infinity, choices- anything
#
#  The line widgets are built using the following parameters in ascParPageVect:
#  pagebg
#  pagefg
#  entrybg  (the active version of fg/bg will be inverse of this pair
#  entryfg
#  entryrelief
#  entryborder
#  entrywidth
#    entrywidth may be overridden by entrywidth being defined in ${PA}
#  checkfg  (the active version of fg/bg will be inverse of this pair
#  checkbg
#  checksel
#  btn_font
#  lbl_font
 set ascParPageVect(pagefg) black
 set ascParPageVect(pagebg) white
 set ascParPageVect(entrybg) white
 set ascParPageVect(entryfg) black
 set ascParPageVect(entryrelief) groove
 set ascParPageVect(entryborder) 3
 set ascParPageVect(entrywidth) 12
 set ascParPageVect(checkbg) white
 set ascParPageVect(checkfg) black
 set ascParPageVect(checksel) black
 global tcl_platform
 if {$tcl_platform(platform)== "unix"} {
   if {![info exists ascParPageVect(btn_font)]} {
     set ascParPageVect(btn_font) {helvetica 12 bold}
   }
   if {![info exists ascParPageVect(lbl_font)]} {
     set ascParPageVect(lbl_font) {helvetica 12 bold}
   }
 } else {
   set ascParPageVect(btn_font) system
   set ascParPageVect(lbl_font) system
 }
 set ascParPageVect(onesize) 1
#
 # aliased operations on vectors are tricky:
 # reading:  set "${ary}(subscript)" returns the value we
 # would incorrectly think of as $alias(subscript)
 # setting:  set "${ary}(subscript)" value works well
 # fortunately most of this is fixed in tcl8 with upvar #0
# ----------------------------------------------------------------------
proc ascParPage {{ap ascParPageVect} {position "+10+10"} {page 1} {sort 1} {first 1}} {
  global ascParPageVect
  upvar #0 $ap PA
  global $ap


 # preprocess the vector {PA}
  if {[info exists PA(namelist)]==0} {
    error "ascParPage called with vector missing namelist"
  }
  set lentrywidth "$ascParPageVect(entrywidth)"
  catch {set lentrywidth $PA(entrywidth)}
  set PA(__,cancelled) 0
  if {[info exists PA(cancellable)]==0} { set PA(cancellable) 1 }
  if {[info exists PA(npages)]==0} { set PA(npages) 1 }
  if {[info exists PA(maxlines)]==0} { set PA(maxlines) 15 }
  if {[info exists PA(toplevel)]==0} { set PA(toplevel) apptop }
  if {[info exists PA(title)]==0} { set PA(title) $ap}
  if {[info exists PA(onesize)]==0} { set PA(onesize) 1}
  if {[info exists PA(grab)]==0} { set PA(grab) 1}
  if {$PA(npages) <0} {set PA(npages) 1}
  if {$page > $PA(npages)} { set page 1}
  set nlist $PA(namelist)
  # nlist used way too much to array deref all the time
  foreach i $nlist {
    if {[info exists PA($i)] == 0}       {set PA($i) 1}
    if {[info exists PA($i.type)] == 0}  {set PA($i.type) string}
    if {[info exists PA($i.label)] == 0} {set PA($i.label) $i}
    if {[info exists PA($i.page)] == 0}  {set PA($i.page) 1}
    if {[info exists PA($i.choices)] == 0 && $PA($i.type)=="string"} {
      set PA($i.choices) ""
    }
    if {$PA(cancellable) && ([info exists PA($i.new)] == 0 || $first)} {
      set PA($i.new) $PA($i)
    }
    if {$PA($i.page) > $PA(npages)} { set PA($i.page) $PA(npages) }
  }
  set npages $PA(npages)
  # get longest page length
  if {[info exists PA(maxppitems)] == 0} {
    set PA(itemsperpage) 0
    set thispage 0
    set max 0
    for {set i 1} {$i <= $npages} {incr i} {
      set thispage 0
      foreach l $nlist {
        if {$i == $PA($l.page)} {incr thispage}
      }
      if {$thispage > $max } {set max $thispage}
      lappend PA(itemsperpage) $thispage
    }
    set PA(maxppitems) $max
  }
  set lwidth 0
  set tl $PA(toplevel)
  # build widget .$PA(toplevel)
  if {[winfo exists $tl]} {
    set upalready 1
    raise $tl
  } else {
    set upalready 0
  }
  if {!$upalready} {
  # get page width in chars. it is the greater of 6 + len biggest boolean label
  #   and (entrywidth + len biggest non-boolean label)
    set bwidth 0
    set ewidth 0
    foreach i $nlist {
      set type $PA($i.type)
      if {$type =="bool" &&
          [string length $PA($i.label)] > $bwidth} {
        set bwidth [string length $PA($i.label)]
      }
      if {($type =="real" || $type =="int" || $type =="string") && \
          [string length $PA($i.label)] > $ewidth } {
        set ewidth [string length $PA($i.label)]
      }
    }
    set lwidth [expr $ewidth + $lentrywidth +5]
    if { $ewidth + $lentrywidth < $bwidth+6} {
      set lwidth [expr $bwidth+6]
    }

    toplevel $tl

    # Window manager configurations
    global tk_version
    wm positionfrom $tl user
    wm sizefrom $tl user
    wm iconname $tl $PA(title)
    wm title $tl $PA(title)
    wm geometry $tl $position

    # build all the widgets, visible or invisible
    set NEW ""
    if {$PA(cancellable)} {set NEW .new}
    foreach i $nlist {
      set type $PA($i.type)
      if {$type=="string" && [info exists PA($i.choices)] && \
           $PA($i.choices) != ""} {
        set type menustring
      }
      if {$type =="int" ||  $type =="real" || \
          $type =="string" || $type =="bin"} {
        # build widget .$PA(toplevel).val_frm_$i
        frame $tl.val_frm_$i

        # build widget .$PA(toplevel).val_frm_$i.entry5
        entry $tl.val_frm_$i.entry5 \
          -font $ascParPageVect(lbl_font) \
          -borderwidth $ascParPageVect(entryborder) \
          -exportselection 0 \
          -textvariable "${ap}($i$NEW)" \
          -width $lentrywidth
        bind $tl.val_frm_$i.entry5 <Leave> "ascParSane %W $ap $PA(cancellable)"

        # int real bin string
        # build widget .$PA(toplevel).val_frm_$i.label4
        label $tl.val_frm_$i.label4 \
          -font $ascParPageVect(lbl_font) \
          -borderwidth {0} \
          -text $PA($i.label)
        if {[info exists PA($i.help)] && [string length $PA($i.help)]} {
          ascparpagebind_labelhelp $tl.val_frm_$i.label4 \
            $ap $i.help $PA($i.label)
        } 

        # pack widget .$PA(toplevel).val_frm_$i
        pack append $tl.val_frm_$i \
          $tl.val_frm_$i.label4 {left frame w fillx} \
          $tl.val_frm_$i.entry5 {right frame e filly}
      }
      if {$type =="menustring" } {
        # build widget .$PA(toplevel).val_frm_$i
        frame $tl.val_frm_$i

        # build widget .$PA(toplevel).val_frm_$i.entry5
        entry $tl.val_frm_$i.entry5 \
          -font $ascParPageVect(lbl_font) \
          -exportselection 0 \
          -borderwidth $ascParPageVect(entryborder) \
          -textvariable "${ap}($i$NEW)" \
          -width $lentrywidth
        bind $tl.val_frm_$i.entry5 <Leave> "ascParSane %W $ap $PA(cancellable)"

        # popup m1 bindings
        bind $tl.val_frm_$i.entry5 <Button-1> \
          "AP_MenuPopupPost $tl.val_frm_$i.entry5.menu0 %X %Y"
        bind $tl.val_frm_$i.entry5 <ButtonRelease-1> \
          "AP_MenuPopupRelease $tl.val_frm_$i.entry5.menu0 %W"

        # build widget $tl.val_frm_$i.entry5.menu0
        menu $tl.val_frm_$i.entry5.menu0 \
          -tearoff 0
        foreach me $PA($i.choices) {
          $tl.val_frm_$i.entry5.menu0 add command \
            -font $ascParPageVect(lbl_font) \
            -command "set ${ap}($i$NEW) \{$me\}" \
            -label $me
        }
        # bindings
        bind $tl.val_frm_$i.entry5.menu0 <B1-Motion> \
          "AP_MenuPopupMotion $tl.val_frm_$i.entry5.menu0 %W %X %Y"
        bind $tl.val_frm_$i.entry5.menu0 <ButtonRelease-1> \
          "AP_MenuPopupRelease $tl.val_frm_$i.entry5.menu0 %W"

        # stringlist label
        # build widget .$PA(toplevel).val_frm_$i.label4
        label $tl.val_frm_$i.label4 \
          -font $ascParPageVect(lbl_font) \
          -borderwidth {0} \
          -padx {2} \
          -text $PA($i.label)
        if {[info exists PA($i.help)] && [string length $PA($i.help)]} {
          ascparpagebind_labelhelp $tl.val_frm_$i.label4 \
            $ap $i.help $PA($i.label)
        }

        # pack widget .$PA(toplevel).val_frm_$i
        pack append $tl.val_frm_$i \
          $tl.val_frm_$i.label4 {left frame w fillx} \
          $tl.val_frm_$i.entry5 {right frame e filly}
      }
      if {$type =="bool"} {
        # build widget .$PA(toplevel).bool_frm_$i
        frame $tl.bool_frm_$i

        # build widget .$PA(toplevel).bool_frm_$i.btn2
        checkbutton $tl.bool_frm_$i.btn2 \
          -borderwidth {1} \
          -text {} \
          -variable "${ap}($i$NEW)"

        # boolean label
        # build widget .$PA(toplevel).bool_frm_$i.lbl1
        label $tl.bool_frm_$i.lbl1 \
          -font $ascParPageVect(lbl_font) \
          -borderwidth $ascParPageVect(entryborder) \
          -text $PA($i.label)
        if {[info exists PA($i.help)] && [string length $PA($i.help)]} {
          ascparpagebind_labelhelp $tl.bool_frm_$i.lbl1 \
            $ap $i.help $PA($i.label)
        }

        # pack widget .${PA}(toplevel).bool_frm_$i
        pack append $tl.bool_frm_$i \
          $tl.bool_frm_$i.btn2 {left frame w filly} \
          $tl.bool_frm_$i.lbl1 {top frame w filly}
      }
    }
#end for loop widget creation
    if {$PA(cancellable)} {
      # build widget .${PA}(toplevel).btn_cancel
      button $tl.btn_cancel \
        -font $ascParPageVect(btn_font) \
        -command "ascParPageCancel $ap" \
        -text {  Cancel  }
    }
    # build widget .${PA}(toplevel).btn_hlp
    button $tl.btn_hlp \
      -font $ascParPageVect(btn_font) \
      -command "$PA(helpcommand)" \
      -text {  Help  }

    # build widget .$PA(toplevel).btn_more
    button $tl.btn_more \
      -font $ascParPageVect(btn_font) \
      -text {More}

    # build widget $tl.btn_ok
    button $tl.btn_ok \
      -font $ascParPageVect(btn_font) \
      -command "ascParPageClose $ap" \
      -text {  OK   }
  }
# end widget construction

  foreach i [winfo children $tl] {pack forget $i}
  catch {foreach i [winfo children $tl.pad] {pack forget $i} }

#pack current page set in int/real/string/bin/bool order
    # pack widget .$PA(toplevel)
  if {$PA(grab)} {
    catch {destroy $tl.bmcanvas}
    canvas $tl.bmcanvas -width 33 -height 27
    $tl.bmcanvas create bitmap 0 0 -background yellow \
       -anchor nw -bitmap grablock
    pack append $tl $tl.bmcanvas {top frame center expand fillx}
  }
  if {$sort} {
    foreach j "int real string bin bool" {
      foreach i $nlist {
        set type $PA($i.type)
        if {$PA($i.page)==$page && $j==$type} {
          switch $type {
          int -
          real -
          string -
          bin {
                pack append $tl \
                    $tl.val_frm_$i {top frame center fill}
              }
          bool {
                 pack append  $tl \
                     $tl.bool_frm_$i {top frame w filly}
               }
          }
        }
      }
    }
  } else { ;# pack in user specified order
    foreach i $nlist {
      if {$PA($i.page)==$page} {
        switch $PA($i.type) {
        int -
        real -
        string -
        bin {
              pack append $tl $tl.val_frm_$i {top frame center fill}
            }
        bool {
               pack append $tl $tl.bool_frm_$i {top frame w filly}
             }
        }
      }
    }
  }
  set padbg [$tl cget -background]
  catch {
    # build widget $tl.pad
    frame $tl.pad
    $tl.pad configure -background $padbg -borderwidth 0
  }
  set en 0
  if {$PA(onesize)} {
    set en [expr $PA(maxppitems) - [lindex $PA(itemsperpage) $page]]
  }

  # build widget $tl.pad.entry$ep
  if {$PA(onesize)} {
    for {set ep 0} {$ep<$en} {incr ep} {
      catch {
        entry $tl.pad.entry$ep \
          -state disabled \
          -borderwidth $ascParPageVect(entryborder) \
          -exportselection 0 \
          -font $ascParPageVect(lbl_font) \
          -relief flat \
          -highlightthickness 0 \
          -background $padbg \
          -width $lwidth
      }
    }
  }

  # build widget $tl.pad.entry
  catch {entry $tl.pad.entry \
      -state disabled \
      -exportselection 0 \
      -font $ascParPageVect(lbl_font) \
      -borderwidth $ascParPageVect(entryborder) \
      -relief flat \
      -highlightthickness 0 \
      -background $padbg \
      -width $lwidth
  }

  pack append $tl.pad $tl.pad.entry {top frame center expand fill}
  if {$PA(onesize)} {
    for {set ep 0} {$ep<$en} {incr ep} {
       pack append $tl.pad $tl.pad.entry$ep {top frame center expand fill}
    }
  }

  pack append $tl \
      $tl.pad {top frame n fill } \
      $tl.btn_ok {left frame sw} 
  if {$PA(cancellable)} {
    pack append $tl \
        $tl.btn_cancel {right frame se} 
  }
  if {[info exists PA(helpcommand)] && [string length $PA(helpcommand)]} {
    pack append $tl \
      $tl.btn_hlp {right frame se}
  }
  # else don't offer it if it does nothing.
  if {$PA(npages) >1} {
  pack append $tl \
      $tl.btn_more {top frame center expand fill}
  $tl.btn_more config \
    -command "ascParPage $ap \"\" [incr page] $sort 0"
  }
  if {$PA(grab)} {
    bind $tl <Visibility> {ascKeepOnTop %W}
    tkwait window $tl
  }
}

# w is the widget, aname the vect, iname the entry.help subscript and label
# the label content.
#######-------new balloon that really works, esp w. 8.1 ----------############
proc ascparpagebind_labelhelp {w aname iname label} {
  upvar #0 $aname PA
  global $aname

  vTcl:set_balloon $w $PA($iname)
}

#
# proc ascparpagebinary {bv}
# ----------------------------------------------------------------------
# returns a doublet {$sane $br}
# sane is 1 if bv given becomes a binary number
# sane is 0 otherwise
# the br returned will have any non-binary digits # hacked out
# ----------------------------------------------------------------------
proc ascparpagebinary {bv} {
  regsub -all \[^01\] $bv "" br
  if {![info exists br] || [string length $br]==0} {
    return [list 0 {}]
  } else {
    return [list 1 $br]
  }
}

#
# proc ascParSane {subscriptORwidget arrayvar cancellable}
# ----------------------------------------------------------------------
# Return 0 if the value entered meets specs, or 1 if out of range,
# 2 if unconvertible input given, 0 if value is "UNDEFINED"
# If variable is of unknown type or boolean, returns 0.
# Name starts with . (is a widget) the variable of the widget
# (entry assumed) is used, otherwise name is treated as if
# element part of PA (without a .new extension).
# On string sanity, the user entered val will be string matched
# (lsearch) against choice list if given, and then the variable value
# will be set to the match from the choice list.
# Numeric variables will be truncated if integer is called for.
# ----------------------------------------------------------------------
proc ascParSane {name ap cancellable} {
  upvar #0 $ap PA
  global $ap
 # we want to arrive at:
 # base is variable to be checkeds subscript as in namelist
 # name is actual full global variable name of the user's input (maybe *.new)
 # val is the user's input value.
  set val ""
  set base ""
  if {[string range $name 0 0] == "."} { # if widget
    # name is the widget variable
    set name [$name cget -textvariable]
    global $name
    set base [string trim [lindex [split $name (] 1] )]
    if {$cancellable} { # strip .new from base
      set base [string range $base 0 [expr [string length $base] - 5]]
    }
  } else {
    set base $name
    set name $ap
    if {$cancellable} {
      append name ( $base .new )
    } else {
      append name ( $base )
    }
  }
  set val [set $name]
  if {$val == "UNDEFINED"} {return 0}
  set type $PA($base.type)
  switch $type {
    {bin} {
      set bstat "[ascparpagebinary $val]"
      set saneval "[lindex $bstat 1]"
      if {[lindex $bstat 0]} {
        set $name $saneval
        if {[info exists PA($base.hi)]} {
          if {[string length $saneval] > $PA($base.hi)} {
             puts stderr "$PA($base.label) is too long.";
             puts stderr "Length limit: $PA($base.hi)";
             return 1
          }
        }
        return 0
      } else {
        return 2
      }
    }
    {int} {
      if { [scan $val %d saneval] } {
        set $name $saneval
        if {[info exists PA($base.lo)]} {
          if {$saneval < $PA($base.lo)} {
            puts stderr "$PA($base.label) is too low ($saneval)";
            puts stderr "lower limit: $PA($base.lo)";
            return 1
          }
        }
        if {[info exists PA($base.hi)]} {
          if {$saneval > $PA($base.hi)} {
             puts stderr "$PA($base.label) is too high ($saneval)";
             puts stderr "upper limit: $PA($base.hi)";
             return 1
          }
        }
        return 0
      } else {
        return 2
      }
    }
    {real} {
      if { [scan $val %g saneval] } {
        set $name $saneval
        if {[info exists PA($base.lo)]} {
          if {$saneval < $PA($base.lo)} {
            puts stderr "$PA($base.label) is too low ($saneval)";
            puts stderr "lower limit: $PA($base.lo)";
            return 1
          }
        }
        if {[info exists PA($base.hi)]} {
          if {[expr $saneval > $PA($base.hi)]} {
            puts stderr "$PA($base.label) is too high ($saneval)";
            puts stderr "upper limit: $PA($base.hi)";
            return 1
          }
        }
        return 0
      } else {
        return 2
      }
    }
    {string} {
      if {$PA($base.choices)!=""} {
        set match [lsearch -exact $PA($base.choices) $val]
        if {$match== "-1"} {
          puts stderr  "$PA($base.label) not among choices:";
          set c $PA($base.choices)
          foreach i $c {puts stderr $i}
          return 1
        } else {
          set $name [lindex $PA($base.choices) $match]
          return 0
        }
      }
    }
    default {return 0}
  }
  return 1
}

# ---------------------------------------------------
# sets the cancellation flag and closes the window
# ---------------------------------------------------
proc ascParPageCancel {ap} {
  upvar #0 $ap PA
  global $ap
  set PA(__,cancelled) 1
  ascParPageClose $ap
}

# ----------------------------------------------------------------------
# checks sanity and handles error reporting for all variables
# before closing window.
# if cancelling close, just destroys window.
# ----------------------------------------------------------------------
proc ascParPageClose {ap} {
  upvar #0 $ap PA
  global $ap
  if {$PA(cancellable) && $PA(__,cancelled)} {
    destroy $PA(toplevel)
    return
  }
  foreach i $PA(namelist) {
    if {[set code [ascParSane $i $ap $PA(cancellable)]]} {
      switch $PA($i.type) {
        {bin} {
          if {$code=="2"} {
            error "Entered value for $i is non-binary."
          } else {
            set lo "0 <"
            set hi 32
            catch { set hi "< $PA($i.hi)"}
            set binhi 1
            for {set bk 1} {$bk < $hi} {incr bk} {
              append binhi 1
            }
            error "Entered value out of range: $lo $PA($i.label) $binhi."
          }
        }
        {int} -
        {real} {
          if {$code=="2"} {
            error "Entered value for $i is non-numeric."
          } else {
            set lo "-infinity <"
            set hi "< infinity"
            catch { set lo "$PA($i.lo) <"}
            catch { set hi "< $PA($i.hi)"}
            error "Entered value out of range: $lo $PA($i.label) $hi."
          }
        }
        {string} {
          if {$PA($i.choices)!=""} {
            set val $PA($i)
            set match [lsearch -exact $PA($i.choices) $val]
            if {$match== "-1"} {
              set irname $PA($i.label);
              set c $PA($i.choices)
              set report "Entered value for $irname not found in choices: $c"
              error $report
            }
          }
        }
        default { puts "\aInsanity ignored on unknown type."}
      }
    }
  }
  set msg ""
  if {$PA(cancellable)} {
    foreach i $PA(namelist) {
      set PA($i) $PA($i.new)
    }
  }
  if {[info exists PA(usercheckcommand)] && \
      [catch {$PA(usercheckcommand) $ap} parpagemsg]} {
    error $parpagemsg
  }
  destroy $PA(toplevel)
  eval $PA(whenokcommand)
}

#
# AP_MenuPopup
#----------------------------------------------------------------
# The following binding functions have been simply renamed from
# a version of xf2.2/tcl7.1/tk3.4 so this file stands alone.
#----------------------------------------------------------------
# Procedure: AP_MenuPopupMotion
##########
# Procedure: AP_MenuPopupMotion
# Description: handle the popup menu motion
# Arguments: xfMenu - the topmost menu
#            xfW - the menu
#            xfX - the root x coordinate
#            xfY - the root x coordinate
# Returns: none
# Sideeffects: none
##########
proc AP_MenuPopupMotion { xfMenu xfW xfX xfY} {
  global tk_popupPriv

  if {"[info commands $xfW]" != "" && [winfo ismapped $xfW] &&
      "[winfo class $xfW]" == "Menu" &&
      [info exists tk_popupPriv($xfMenu,focus)] &&
      "$tk_popupPriv($xfMenu,focus)" != "" &&
      [info exists tk_popupPriv($xfMenu,grab)] &&
      "$tk_popupPriv($xfMenu,grab)" != ""} {
    set xfPopMinX [winfo rootx $xfW]
    set xfPopMaxX [expr $xfPopMinX+[winfo width $xfW]]
    if {$xfX >= $xfPopMinX && $xfX <= $xfPopMaxX} {
      $xfW activate @[expr $xfY-[winfo rooty $xfW]]
      if {![catch "$xfW entryconfig @[expr $xfY-[winfo rooty $xfW]] -menu" result]} {
        if {"[lindex $result 4]" != ""} {
          foreach binding [bind $xfMenu] {
            bind [lindex $result 4] $binding [bind $xfMenu $binding]
          }
        }
      }
    } {
      $xfW activate none
    }
  }
}

##########
# Procedure: AP_MenuPopupPost
# Description: post the popup menu
# Arguments: xfMenu - the menu
#            xfX - the root x coordinate
#            xfY - the root x coordinate
# Returns: none
# Sideeffects: none
##########
proc AP_MenuPopupPost { xfMenu xfX xfY} {
  global tk_popupPriv

  if {"[info commands $xfMenu]" != ""} {
    if {![info exists tk_popupPriv($xfMenu,focus)]} {
      set tk_popupPriv($xfMenu,focus) [focus]
    } {
      if {"$tk_popupPriv($xfMenu,focus)" == ""} {
        set tk_popupPriv($xfMenu,focus) [focus]
      }
    }
    set tk_popupPriv($xfMenu,grab) $xfMenu

    catch "$xfMenu activate none"
    catch "$xfMenu post $xfX $xfY"
    catch "focus $xfMenu"
    catch "grab -global $xfMenu"
  }
}

##########
# Procedure: AP_MenuPopupRelease
# Description: remove the popup menu
# Arguments: xfMenu - the topmost menu widget
#            xfW - the menu widget
# Returns: none
# Sideeffects: none
##########
proc AP_MenuPopupRelease { xfMenu xfW} {
  global tk_popupPriv
  global tk_version

  if {"[info commands $xfW]" != "" && [winfo ismapped $xfW] &&
      "[winfo class $xfW]" == "Menu" &&
      [info exists tk_popupPriv($xfMenu,focus)] &&
      "$tk_popupPriv($xfMenu,focus)" != "" &&
      [info exists tk_popupPriv($xfMenu,grab)] &&
      "$tk_popupPriv($xfMenu,grab)" != ""} {
    if {$tk_version >= 3.0} {
      catch "grab release $tk_popupPriv($xfMenu,grab)"
    } {
      catch "grab none"
    }
    catch "focus $tk_popupPriv($xfMenu,focus)"
    set tk_popupPriv($xfMenu,focus) ""
    set tk_popupPriv($xfMenu,grab) ""
    if {"[$xfW index active]" != "none"} {
      $xfW invoke active; catch "$xfMenu unpost"
    }
  }
  catch "$xfMenu unpost"
}



# eof
#


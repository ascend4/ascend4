# Module: ascfontbox.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.3
#
# By Benjamin Allan
# June 5, 1997
# $Revision: 1.8 $
# $Author: mthomas $
# $Date: 1997/06/17 16:00:30 $
# Unlike the rest of the ascend distribution, this widget is
# placed in the public domain.
#
# This module implements a simple tk8 based font selection
# window with an editable text sample.
# The global variable ascFontVect(selected) has the most recently
# selected font at any given time.
# If an answer is required in a particular time, rather than
# through a trace, the function ascFontGet can be called for
# a modal selection process.

# module contents
global moduleList
global ascFontVect
global autoLoadList
set moduleList(ascfontsel.tcl) { .ascfontsel}
set autoLoadList(ascfontsel.tcl) {1}

proc ascFontGet {} {
  global ascFontVect
  ShowWindow.ascfontsel
  ascFont_set_Defaults
  tkwait window .ascfontsel
  if {$ascFontVect(exit_button) == "exit"} {
    return $ascFontVect(selected)
  } else {
    return ""
  }
}

set ascFontVect(family) courier
proc ascFont_set_Defaults {} {
  global ascFontVect
  if {[info exists ascFontVect(inited)]==0} {
    set ascFontVect(inited) 1
    set ascFontVect(windowname) .ascfontsel
    set ascFontVect(normal) 1
    set ascFontVect(italic) 0
    set ascFontVect(bold) 0
    set ascFontVect(underline) 0
    set ascFontVect(overstrike) 0
    set ascFontVect(roman) 0
    set ascFontVect(size) 12
 # broken Sun fonts
    lappend ascFontVect(badlist) \
      {fangsong ti} \
      gothic \
      mincho \
      nil \
      {open look cursor} \
      {song ti} \
      {open look glyph}
    foreach i [lsort [font families] ] {
      if {[lsearch -exact $ascFontVect(badlist) $i] == -1} {
        lappend ascFontVect(goodlist) $i
      }
    }
  }
}

proc ascFont_resetquietly {a s op} {
  global ascFontVect
  trace vdelete ascFontVect w ascFont_configure
  if {$ascFontVect(normal) && $s == "normal"} {
    set ascFontVect(italic) 0
    set ascFontVect(bold) 0
    set ascFontVect(underline) 0
    set ascFontVect(overstrike) 0
    set ascFontVect(roman) 0
  } else {
    if {$s == "italic" || $s =="bold" || $s =="underline" || \
        $s == "overstrike" || $s == "roman"} {
      set ascFontVect(normal) 0
    }
  }
  trace variable ascFontVect w ascFont_configure
}

proc ascFont_fillsamples {} {
  global ascFontVect
  set fontlist $ascFontVect(goodlist)
  set ln 1
  set t .ascfontsel.samp_frm.list2
  $t configure -state normal
  $t delete 1.0 end
  foreach i $fontlist {
    $t insert end $i
    set str "\{$i\}"
    append str " 12 normal"
    $t insert end " 12 normal\n"
    set ascFontVect(font.$ln) "\{$i\}"
    $t tag add font$ln $ln.0 $ln.end
    $t tag configure font$ln -font $str
    $t tag bind font$ln <Button-1> "
      set ascFontVect(family) \$ascFontVect(font.$ln)
    "
    incr ln
  }
  $t configure -state disabled
}

proc ascFont_configure {a s op} {
  global ascFontVect
  set name $ascFontVect(family)
  append name " $ascFontVect(size) "
  if {$ascFontVect(normal)} {
    ascFont_resetquietly $a $s $op
  }
  foreach i {normal italic bold underline overstrike roman} {
    if {$ascFontVect($i)} {
      append name "$i "
    }
  }
  .ascfontsel.frame.entry5 delete 0 end
  .ascfontsel.frame.entry5 insert end $name
  .ascfontsel.test_frm.text2 configure -font $name
  .ascfontsel.test_frm.text2 delete 1.0 end
  regsub @@@ $ascFontVect(sample) $name sample
  .ascfontsel.test_frm.text2 insert end $sample
  return $name
}

proc ascFont_cancel {} {
  global ascFontVect
  set ascFontVect(selected) [ascFont_configure 0 0 0]
  trace vdelete ascFontVect w ascFont_configure
  set ascFontVect(exit_button) {cancel}
  DestroyWindow.ascfontsel
}

proc ascFont_exit {} {
  global ascFontVect
  set ascFontVect(selected) [ascFont_configure 0 0 0]
  trace vdelete ascFontVect w ascFont_configure
  set ascFontVect(exit_button) {exit}
  DestroyWindow.ascfontsel
}

# procedure to show window .ascfontsel
proc ShowWindow.ascfontsel {args} {# xf ignore me 7

  # build widget .ascfontsel
  catch "destroy .ascfontsel"
  toplevel .ascfontsel
  global ascFontVect

  catch {trace vdelete ascFontVect w ascFont_configure}
  ascFont_set_Defaults
  set fontlist $ascFontVect(goodlist)
  # Window manager configurations
  wm positionfrom .ascfontsel ""
  wm sizefrom .ascfontsel program
  wm iconname .ascfontsel {ASCEND Font Picker}
  wm maxsize .ascfontsel 1000 900
  wm geometry .ascfontsel 350x350
  wm minsize .ascfontsel 0 0
  wm title .ascfontsel {ASCEND Font Picker}
  wm protocol .ascfontsel WM_DELETE_WINDOW {ascFont_cancel}


  # build widget .ascfontsel.mb_frm
  frame .ascfontsel.mb_frm

  # build widget .ascfontsel.mb_frm.font_btn
  menubutton .ascfontsel.mb_frm.font_btn \
    -menu {.ascfontsel.mb_frm.font_btn.m} \
    -text {Font}

  # build widget .ascfontsel.mb_frm.font_btn.m
  menu .ascfontsel.mb_frm.font_btn.m \
    -tearoffcommand .ASCFONTSEL.MB_FRM.FONT_BTN.M \
    -tearoff 0

  foreach i $fontlist {
    .ascfontsel.mb_frm.font_btn.m add radiobutton \
      -label $i \
      -value \{$i\} \
      -variable ascFontVect(family)
  }
  # build widget .ascfontsel.mb_frm.style_btn
  menubutton .ascfontsel.mb_frm.style_btn \
    -menu {.ascfontsel.mb_frm.style_btn.m} \
    -text {Styles}

  # build widget .ascfontsel.mb_frm.style_btn.m
  menu .ascfontsel.mb_frm.style_btn.m \
    -tearoffcommand .ASCFONTSEL.MB_FRM.STYLE_BTN.M \
    -tearoff 0

  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {normal} \
    -variable ascFontVect(normal)
  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {italic} \
    -variable ascFontVect(italic)
  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {bold} \
    -variable ascFontVect(bold)
  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {underline} \
    -variable ascFontVect(underline)
  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {overstrike} \
    -variable ascFontVect(overstrike)
  .ascfontsel.mb_frm.style_btn.m add checkbutton \
    -label {roman} \
    -variable ascFontVect(roman)

  # build widget .ascfontsel.mb_frm.ok_btn
  button .ascfontsel.mb_frm.ok_btn \
    -command ascFont_exit \
    -text {OK}

  # build widget .ascfontsel.mb_frm.cancel_btn
  button .ascfontsel.mb_frm.cancel_btn \
    -command ascFont_cancel \
    -text {Cancel}

  # pack widget .ascfontsel.mb_frm
  pack append .ascfontsel.mb_frm \
    .ascfontsel.mb_frm.font_btn {left frame center fillx} \
    .ascfontsel.mb_frm.style_btn {left frame center fillx} \
    .ascfontsel.mb_frm.ok_btn {right frame center fillx} \
    .ascfontsel.mb_frm.cancel_btn {right frame center fillx}

  # build widget .ascfontsel.frame
  frame .ascfontsel.frame

  # build widget .ascfontsel.frame.label4
  label .ascfontsel.frame.label4 \
    -text {Font selected:}

  # build widget .ascfontsel.frame.entry5
  entry .ascfontsel.frame.entry5 \
    -exportselection 0

  # pack widget .ascfontsel.frame
  pack append .ascfontsel.frame \
    .ascfontsel.frame.label4 {left frame center} \
    .ascfontsel.frame.entry5 {top frame center expand fill}

  # build widget .ascfontsel.scale9
  scale .ascfontsel.scale9 \
    -label {Number of points} \
    -orient {horizontal} \
    -from 4 \
    -to 48 \
    -tickinterval 4 \
    -bigincrement 4 \
    -variable ascFontVect(size) \
    -showvalue {0}

  # build widget .ascfontsel.test_frm
  frame .ascfontsel.test_frm

  # build widget .ascfontsel.test_frm.scrollbar1
  scrollbar .ascfontsel.test_frm.scrollbar1 \
    -command {.ascfontsel.test_frm.text2 yview}

  # build widget .ascfontsel.test_frm.text2
  text .ascfontsel.test_frm.text2 \
    -exportselection {0} \
    -height 5 \
    -wrap {word} \
    -yscrollcommand {.ascfontsel.test_frm.scrollbar1 set}

  # pack widget .ascfontsel.test_frm
  pack append .ascfontsel.test_frm \
    .ascfontsel.test_frm.scrollbar1 {right frame center filly} \
    .ascfontsel.test_frm.text2 {top frame center expand fill}


  # build widget .ascfontsel.samp_frm
  frame .ascfontsel.samp_frm

  # build widget .ascfontsel.samp_frm.scrollbar1
  scrollbar .ascfontsel.samp_frm.scrollbar1 \
    -command {.ascfontsel.samp_frm.list2 yview}

  # build widget .ascfontsel.samp_frm.list2
  text .ascfontsel.samp_frm.list2 \
    -exportselection {0} \
    -height 4 \
    -wrap none \
    -yscrollcommand {.ascfontsel.samp_frm.scrollbar1 set}

  # pack widget .ascfontsel.samp_frm
  pack append .ascfontsel.samp_frm \
    .ascfontsel.samp_frm.scrollbar1 {right frame center filly} \
    .ascfontsel.samp_frm.list2 {top frame center expand fill}

  # pack widget .ascfontsel
  pack append .ascfontsel \
    .ascfontsel.mb_frm {top frame center fillx} \
    .ascfontsel.test_frm {top frame n fillx} \
    .ascfontsel.samp_frm {top frame n expand fill} \
    .ascfontsel.scale9 {top frame center fillx} \
    .ascfontsel.frame {top frame center fill} \

  .ascfontsel.frame.entry5 insert end {courier}
  set ascFontVect(sample) {This is a sample of text in @@@, or the closest match. You can edit this text. MODEL foo() 1234567890_= !@#$%^&*()-+ <>,.?/{}[]~\|`'" qwertyuiop asdfghjkl zxcvbnm QWERTYUIOP ASDFGHJKL ZXCVBNM }
  ascFont_fillsamples
  ascFont_configure 0 0 0
}

proc DestroyWindow.ascfontsel {} {# xf ignore me 7
    catch "destroy .ascfontsel"
    update
}


# User defined procedures


# Internal procedures

# eof
#


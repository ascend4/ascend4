#                        main.tcl
#                        by Robert S. Huss
#                        Part of the Ascend Library
#
#This file is part of the Ascend library.
#
#Copyright (C) 1994
#
#The Ascend library is free software; you can redistribute
#it and/or modify it under the terms of the GNU General Public License as
#published by the Free Software Foundation; either version 2 of the
#License, or (at your option) any later version.
#
#The Ascend Language Interpreter is distributed in hope that it will be
#useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#General Public License for more details.
#
#You should have received a copy of the GNU General Public License along with
#the program; if not, write to the Free Software Foundation, Inc., 675
#Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.

#  $Date: 1996/05/23 21:37:46 $
#  $Revision: 1.2 $
#  $Author: mthomas $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/main.tcl,v $

#!/usr/contributed/bin/wish -f
# Program: main
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module inclusion
global env
global xfLoadPath
global xfLoadInfo
set xfLoadInfo 0
set xfLoadPath "$env(ASCENDDIST)/models/examples/flexible_design"

global argc
global argv
global tkVersion
set tmpArgv ""
for {set counter 0} {$counter < $argc} {incr counter 1} {
  case [string tolower [lindex $argv $counter]] in {
    {-xfloadpath} {
      incr counter 1
      set xfLoadPath "[lindex $argv $counter]:$xfLoadPath"
    }
    {-xfstartup} {
      incr counter 1
      source [lindex $argv $counter]
    }
    {-xfbindfile} {
      incr counter 1
      set env(XF_BIND_FILE) "[lindex $argv $counter]"
    }
    {-xfcolorfile} {
      incr counter 1
      set env(XF_COLOR_FILE) "[lindex $argv $counter]"
    }
    {-xfcursorfile} {
      incr counter 1
      set env(XF_CURSOR_FILE) "[lindex $argv $counter]"
    }
    {-xffontfile} {
      incr counter 1
      set env(XF_FONT_FILE) "[lindex $argv $counter]"
    }
    {-xfmodelmono} {
      if {$tkVersion >= 3.0} {
        tk colormodel . monochrome
      }
    }
    {-xfmodelcolor} {
      if {$tkVersion >= 3.0} {
        tk colormodel . color
      }
    }
    {-xfloading} {
      set xfLoadInfo 1
    }
    {-xfnoloading} {
      set xfLoadInfo 0
    }
    {default} {
      lappend tmpArgv [lindex $argv $counter]
    }
  }
}
set argv $tmpArgv
set argc [llength $tmpArgv]
unset counter
unset tmpArgv


# User defined procedures


# Procedure: ClearList
proc ClearList { listWidget} {
# xf ignore me 5
##########
# Procedure: ClearList
# Description: clear listbox widget
# Arguments: listWidget - the widget to clear
# Returns: none
# Sideeffects: the list widget is cleared
##########

  if {[$listWidget size] > 0} {
    $listWidget delete 0 end
  }
}


# Procedure: ColorBox
proc ColorBox { {colorBoxFileColor "/usr/local/lib/xf/lib/Colors"} {colorBoxMessage "Color"} {colorBoxEntryW ""} {colorBoxTargetW ""}} {
# xf ignore me 5
##########
# Procedure: ColorBox
# Description: select a color
# Arguments: {colorBoxFileColor} - the color file with all colornames
#            {colorBoxMessage} - a message to display
#            {colorBoxEntryW} - the widget name for the resulting color name
#            {colorBoxTargetW} - the widget we configure
# Returns: colorname, or nothing
# Sideeffects: none
##########
# 
# global colorBox(activeBackground) - active background color
# global colorBox(activeForeground) - active foreground color
# global colorBox(background) - background color
# global colorBox(font) - text font
# global colorBox(foreground) - foreground color
# global colorBox(palette) - a palette of colors
# global colorBox(scrollActiveForeground) - scrollbar active background color
# global colorBox(scrollBackground) - scrollbar background color
# global colorBox(scrollForeground) - scrollbar foreground color
# global colorBox(scrollSide) - side where scrollbar is located

  global colorBox

  set colorBox(colorName) ""
  set colorBox(paletteNr) 0

  set tmpButtonOpt ""
  set tmpFrameOpt ""
  set tmpMessageOpt ""
  set tmpScaleOpt ""
  set tmpScrollOpt ""
  if {"$colorBox(activeBackground)" != ""} {
    append tmpButtonOpt "-activebackground \"$colorBox(activeBackground)\" "
  }
  if {"$colorBox(activeForeground)" != ""} {
    append tmpButtonOpt "-activeforeground \"$colorBox(activeForeground)\" "
  }
  if {"$colorBox(background)" != ""} {
    append tmpButtonOpt "-background \"$colorBox(background)\" "
    append tmpFrameOpt "-background \"$colorBox(background)\" "
    append tmpMessageOpt "-background \"$colorBox(background)\" "
    append tmpScaleOpt "-background \"$colorBox(background)\" "
  }
  if {"$colorBox(font)" != ""} {
    append tmpButtonOpt "-font \"$colorBox(font)\" "
    append tmpMessageOpt "-font \"$colorBox(font)\" "
  }
  if {"$colorBox(foreground)" != ""} {
    append tmpButtonOpt "-foreground \"$colorBox(foreground)\" "
    append tmpMessageOpt "-foreground \"$colorBox(foreground)\" "
    append tmpScaleOpt "-foreground \"$colorBox(foreground)\" "
  }
  if {"$colorBox(scrollActiveForeground)" != ""} {
    append tmpScaleOpt "-activeforeground \"$colorBox(scrollActiveForeground)\" "
    append tmpScrollOpt "-activeforeground \"$colorBox(scrollActiveForeground)\" "
  }
  if {"$colorBox(scrollBackground)" != ""} {
    append tmpScrollOpt "-background \"$colorBox(scrollBackground)\" "
  }
  if {"$colorBox(scrollForeground)" != ""} {
    append tmpScaleOpt "-sliderforeground \"$colorBox(scrollForeground)\" "
    append tmpScrollOpt "-foreground \"$colorBox(scrollForeground)\" "
  }

  # get color file name
  if {!([file exists $colorBoxFileColor] &&
        [file readable $colorBoxFileColor])} {
    set colorBoxFileColor ""
  }
  if {"$colorBoxFileColor" == ""} {
    global env
    if {[info exists env(XF_COLOR_FILE)]} {
      if {[file exists $env(XF_COLOR_FILE)] &&
          [file readable $env(XF_COLOR_FILE)]} {
        set colorBoxFileColor $env(XF_COLOR_FILE)
      }
    }
  }
  if {"$colorBoxMessage" == ""} {
    set colorBoxMessage "Color"
  }

  # save the the current widget color
  if {"$colorBoxTargetW" != ""} {
    if {[catch "$colorBoxTargetW config -[string tolower $colorBoxMessage]" result]} {
      set colorBoxSavedColor ""
    } {
      set colorBoxSavedColor [lindex $result 4]
    }
  } {
    set colorBoxSavedColor ""
  }

  # look if there is already a color window
  if {"[info commands .colorBox]" == ""} {
    # build widget structure

    # start build of toplevel
    if {"[info commands XFDestroy]" != ""} {
      catch {XFDestroy .colorBox}
    } {
      catch {destroy .colorBox}
    }
    toplevel .colorBox  -borderwidth 0
    catch ".colorBox config $tmpFrameOpt"
    wm geometry .colorBox 400x250
    wm title .colorBox {Color box}
    wm maxsize .colorBox 1000 1000
    wm minsize .colorBox 100 100
    # end build of toplevel

    set colorBox(oldWidget) $colorBoxEntryW

    frame .colorBox.frame1  -borderwidth 0  -relief raised
    catch ".colorBox.frame1 config $tmpFrameOpt"
 
    button .colorBox.frame1.ok  -text "OK"
    catch ".colorBox.frame1.ok config $tmpButtonOpt"

    button .colorBox.frame1.cancel  -text "Cancel"
    catch ".colorBox.frame1.cancel config $tmpButtonOpt"

    frame .colorBox.frame2  -borderwidth 0  -relief raised
    catch ".colorBox.frame2 config $tmpFrameOpt"
 
    radiobutton .colorBox.frame2.rgb  -command "ColorBoxShowSlides $colorBoxMessage \"$colorBoxTargetW\""  -text "RGB"  -variable colorBox(type)
    catch ".colorBox.frame2.rgb config $tmpButtonOpt"

    radiobutton .colorBox.frame2.hsv  -command "ColorBoxShowSlides $colorBoxMessage \"$colorBoxTargetW\""  -text "HSV"  -variable colorBox(type)
    catch ".colorBox.frame2.hsv config $tmpButtonOpt"

    radiobutton .colorBox.frame2.list  -command "ColorBoxShowSlides $colorBoxMessage \"$colorBoxTargetW\""  -text "List"  -variable colorBox(type)
    catch ".colorBox.frame2.list config $tmpButtonOpt"

    frame .colorBox.palette  -borderwidth 0  -relief raised
    catch ".colorBox.palette config $tmpFrameOpt"
 
    set counter 0
    foreach element $colorBox(palette) {
      button .colorBox.palette.palette$counter  -command "ColorBoxSetPalette $colorBoxMessage \"$colorBoxTargetW\" $counter"  -width 3
      catch ".colorBox.palette.palette$counter config  -activebackground \"$element\"  -background \"$element\""

      pack append .colorBox.palette .colorBox.palette.palette$counter {left fill expand}
      incr counter
    }

    scale .colorBox.red  -background "red"  -from 0  -label "Red"  -orient horizontal  -relief raised  -sliderlength 15  -to 255  -width 8
    catch ".colorBox.red config $tmpScaleOpt"

    scale .colorBox.green  -background "green"  -from 0  -label "Green"  -orient horizontal  -relief raised  -sliderlength 15  -to 255  -width 8
    catch ".colorBox.green config $tmpScaleOpt"

    scale .colorBox.blue  -background "blue"  -from 0  -label "Blue"  -orient horizontal  -relief raised  -sliderlength 15  -to 255  -width 8
    catch ".colorBox.blue config $tmpScaleOpt"

    scale .colorBox.h  -from 0  -label "Hue"  -orient horizontal  -relief raised  -sliderlength 15  -to 1000  -width 8
    catch ".colorBox.h config $tmpScaleOpt"

   scale .colorBox.s  -from 0  -label "Saturation * 100"  -orient horizontal  -relief raised  -sliderlength 15  -to 1000  -width 8
    catch ".colorBox.s config $tmpScaleOpt"

    scale .colorBox.v  -from 0  -label "Value"  -orient horizontal  -relief raised  -sliderlength 15  -to 1000  -width 8
    catch ".colorBox.v config $tmpScaleOpt"

    label .colorBox.demo  -relief raised  -text "This text shows the results :-)"
    catch ".colorBox.demo config $tmpMessageOpt"

    frame .colorBox.current  -borderwidth 0  -relief raised
    catch ".colorBox.current config $tmpFrameOpt"

    label .colorBox.current.labelcurrent  -relief raised
    catch ".colorBox.current.labelcurrent config $tmpMessageOpt"

    entry .colorBox.current.current  -relief raised
    catch ".colorBox.current.current config $tmpMessageOpt"

    frame .colorBox.colors  -borderwidth 0  -relief raised
    catch ".colorBox.colors config $tmpFrameOpt"

    scrollbar .colorBox.colors.vscroll  -relief raised  -command ".colorBox.colors.colors yview"
    catch ".colorBox.colors.vscroll config $tmpScrollOpt"

    scrollbar .colorBox.colors.hscroll  -orient horiz  -relief raised  -command ".colorBox.colors.colors xview"
    catch ".colorBox.colors.hscroll config $tmpScrollOpt"

    listbox .colorBox.colors.colors  -exportselection false  -relief raised  -xscrollcommand ".colorBox.colors.hscroll set"  -yscrollcommand ".colorBox.colors.vscroll set"
    catch ".colorBox.colors.colors config $tmpMessageOpt"

    # read color file
    if {"$colorBoxFileColor" != ""} {
      if {[catch "open $colorBoxFileColor r" colorInFile]} {
        set colorBoxFileColor ""
        if {"[info commands AlertBox]" != ""} {
          AlertBox "$colorInFile"
        } {
          puts stderr "$colorInFile"
        }
      } {
        set colorReadList [read $colorInFile]
        close $colorInFile
        foreach colorLine [split $colorReadList "\n"] {
          if {"[string trim $colorLine]" != ""} {
            set colorNewLine [lrange $colorLine 3 end]
            append colorNewLine " " [format #%02x [lindex $colorLine 0]]
            append colorNewLine [format %02x [lindex $colorLine 1]]
            append colorNewLine [format %02x [lindex $colorLine 2]]
            .colorBox.colors.colors insert end $colorNewLine
          }
        }
      }
    }

    # bindings
    bind .colorBox.colors.colors <ButtonPress-1> "
      ColorBoxSelectColor %W $colorBoxMessage \"$colorBoxTargetW\" %y"
    bind .colorBox.colors.colors <Button1-Motion> "
      ColorBoxSelectColor %W $colorBoxMessage \"$colorBoxTargetW\" %y"
    bind .colorBox.colors.colors <Shift-ButtonPress-1> "
      ColorBoxSelectColor %W $colorBoxMessage \"$colorBoxTargetW\" %y"
    bind .colorBox.colors.colors <Shift-Button1-Motion> "
      ColorBoxSelectColor %W $colorBoxMessage \"$colorBoxTargetW\" %y"
  } {
    if {"[winfo class $colorBox(oldWidget)]" == "Text"} {
      catch "$colorBox(oldWidget) delete 1.0 end"
      catch "$colorBox(oldWidget) insert 1.0 [.colorBox.current.current get]"
    } {
      if {"[winfo class $colorBox(oldWidget)]" == "Entry"} {
        catch "$colorBox(oldWidget) delete 0 end"
        catch "$colorBox(oldWidget) insert 0 [.colorBox.current.current get]"
      }
    }

    set colorBox(oldWidget) $colorBoxEntryW
  }
   
  .colorBox.frame1.ok config  -command "
      global colorBox
      set colorBox(colorName) \[.colorBox.current.current get\]
      if {\"$colorBoxEntryW\" != \"\"} {
        if {\"\[winfo class $colorBoxEntryW\]\" == \"Text\"} {
          catch \"$colorBoxEntryW delete 1.0 end\"
          catch \"$colorBoxEntryW insert 1.0 \\\"\$colorBox(colorName)\\\"\"
        } {
          if {\"\[winfo class $colorBoxEntryW\]\" == \"Entry\"} {
            catch \"$colorBoxEntryW delete 0 end\"
            catch \"$colorBoxEntryW insert 0 \\\"\$colorBox(colorName)\\\"\"
          }
        }
      }
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .colorBox}
      } {
        catch {destroy .colorBox}
      }"

  .colorBox.frame1.cancel config  -command "
      global colorBox
      set colorBox(colorName) {}
      if {\"$colorBoxTargetW\" != \"\"} {
        catch \"$colorBoxTargetW config -\[string tolower $colorBoxMessage\] $colorBoxSavedColor\"
      }
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .colorBox}
      } {
        catch {destroy .colorBox}
      }"

  .colorBox.red config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.green config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.blue config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.h config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.s config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.v config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""

  .colorBox.current.labelcurrent config  -text "$colorBoxMessage:"

  # bindings
  bind .colorBox.current.current <Return> "
    ColorBoxSetPaletteList \[.colorBox.current.current get\]
    ColorBoxSetColor $colorBoxMessage \"$colorBoxTargetW\" text \[.colorBox.current.current get\]"

  bind .colorBox.colors.colors <Double-1> "
    ColorBoxSelectColor %W $colorBoxMessage \"$colorBoxTargetW\" %y
    global colorBox
    set colorBox(colorName) \[.colorBox.current.current get\]
    if {\"$colorBoxEntryW\" != \"\"} {
      if {\"\[winfo class $colorBoxEntryW\]\" == \"Text\"} {
        catch \"$colorBoxEntryW delete 1.0 end\"
        catch \"$colorBoxEntryW insert 1.0 \\\"\$colorBox(colorName)\\\"\"
      } {
        if {\"\[winfo class $colorBoxEntryW\]\" == \"Entry\"} {
          catch \"$colorBoxEntryW delete 0 end\"
          catch \"$colorBoxEntryW insert 0 \\\"\$colorBox(colorName)\\\"\"
        }
      }
    }
    if {\"\[info commands XFDestroy\]\" != \"\"} {
      catch {XFDestroy .colorBox}
    } {
      catch {destroy .colorBox}
    }"

  # set up current value
  .colorBox.current.current delete 0 end
  if {"$colorBoxEntryW" != ""} {
    if {"[winfo class $colorBoxEntryW]" == "Text"} {
      .colorBox.current.current insert 0 [$colorBoxEntryW get 1.0 end]
    } {
      if {"[winfo class $colorBoxEntryW]" == "Entry"} {
        .colorBox.current.current insert 0 [$colorBoxEntryW get]
      }
    }
  }
  if {"[.colorBox.current.current get]" != ""} {
    ColorBoxSetColor $colorBoxMessage $colorBoxTargetW text [.colorBox.current.current get]
  }
    
  # packing
  pack append .colorBox.frame1  .colorBox.frame1.ok {left fill expand}  .colorBox.frame1.cancel {left fill expand}
  pack append .colorBox.frame2  .colorBox.frame2.rgb {left fill expand}  .colorBox.frame2.hsv {left fill expand}  .colorBox.frame2.list {left fill expand}
  pack append .colorBox.current  .colorBox.current.labelcurrent {left}  .colorBox.current.current {left fill expand}
  pack append .colorBox.colors  .colorBox.colors.vscroll "$colorBox(scrollSide) filly"  .colorBox.colors.hscroll {bottom fillx}  .colorBox.colors.colors {left fill expand}

  ColorBoxShowSlides $colorBoxMessage $colorBoxTargetW

  catch "wm deiconify .colorBox"

  if {"$colorBoxEntryW" == ""} {
    # wait for the box to be destroyed
    update idletask
    grab .colorBox
    tkwait window .colorBox

    return $colorBox(colorName)
  }
}


# Procedure: ColorBoxHSVToRGB
proc ColorBoxHSVToRGB { colorBoxHue colorBoxSat colorBoxValue} {
# xf ignore me 6
# The HSV <-> RGB converting routines are from the
# tcolor demo that is part of the demo site of Tk.

  set colorBoxV [format %.0f [expr 65535.0*$colorBoxValue]]
  if {$colorBoxSat == 0} {
    return "$colorBoxV $colorBoxV $colorBoxV"
  } else {
    set colorBoxHue [expr $colorBoxHue*6.0]
    if {$colorBoxHue >= 6.0} {
      set colorBoxHue 0.0
    }
    scan $colorBoxHue. %d i
    set colorBoxF [expr $colorBoxHue-$i]
    set colorBoxP [format %.0f [expr {65535.0*$colorBoxValue*(1 - $colorBoxSat)}]]
    set colorBoxQ [format %.0f [expr {65535.0*$colorBoxValue*(1 - ($colorBoxSat*$colorBoxF))}]]
    set colorBoxT [format %.0f [expr {65535.0*$colorBoxValue*(1 - ($colorBoxSat*(1 - $colorBoxF)))}]]
    case $i  0 {return "$colorBoxV $colorBoxT $colorBoxP"}  1 {return "$colorBoxQ $colorBoxV $colorBoxP"}  2 {return "$colorBoxP $colorBoxV $colorBoxT"}  3 {return "$colorBoxP $colorBoxQ $colorBoxV"}  4 {return "$colorBoxT $colorBoxP $colorBoxV"}  5 {return "$colorBoxV $colorBoxP $colorBoxQ"}
    error "i value $i is out of range"
  }
}


# Procedure: ColorBoxRGBToHSV
proc ColorBoxRGBToHSV { colorBoxRed colorBoxGreen colorBoxBlue} {
# xf ignore me 6
# The HSV <-> RGB converting routines are from the
# tcolor demo that is part of the demo site of Tk.

  if {$colorBoxRed > $colorBoxGreen} {
    set colorBoxMax $colorBoxRed.0
    set colorBoxMin $colorBoxGreen.0
  } else {
    set colorBoxMax $colorBoxGreen.0
    set colorBoxMin $colorBoxRed.0
  }
  if {$colorBoxBlue > $colorBoxMax} {
    set colorBoxMax $colorBoxBlue.0
  } else {
    if {$colorBoxBlue < $colorBoxMin} {
      set colorBoxMin $colorBoxBlue.0
    }
  }
  set range [expr $colorBoxMax-$colorBoxMin]
  if {$colorBoxMax == 0} {
    set colorBoxSat 0
  } else {
    set colorBoxSat [expr {($colorBoxMax-$colorBoxMin)/$colorBoxMax}]
  }
  if {$colorBoxSat == 0} {
    set colorBoxHue 0
  } else {
    set colorBoxRC [expr {($colorBoxMax - $colorBoxRed)/$range}]
    set colorBoxGC [expr {($colorBoxMax - $colorBoxGreen)/$range}]
    set colorBoxBC [expr {($colorBoxMax - $colorBoxBlue)/$range}]
    if {$colorBoxRed == $colorBoxMax} {
      set colorBoxHue [expr {.166667*($colorBoxBC - $colorBoxGC)}]
    } else {
      if {$colorBoxGreen == $colorBoxMax} {
        set colorBoxHue [expr {.166667*(2 + $colorBoxRC - $colorBoxBC)}]
      } else {
        set colorBoxHue [expr {.166667*(4 + $colorBoxGC - $colorBoxRC)}]
      }
    }
  }
  return [list $colorBoxHue $colorBoxSat [expr {$colorBoxMax/65535}]]
}


# Procedure: ColorBoxSelectColor
proc ColorBoxSelectColor { colorW colorBoxMessage colorBoxTargetW colorY} {
# xf ignore me 6

  set colorNearest [$colorW nearest $colorY]
  if {$colorNearest >= 0} {
    $colorW select from $colorNearest
    $colorW select to $colorNearest
    set colorTmpValue [$colorW get $colorNearest]
    set colorCurrentColor [lrange $colorTmpValue 0  [expr [llength $colorTmpValue]-2]]
    set colorCurrentValue [lrange $colorTmpValue  [expr [llength $colorTmpValue]-1] end]

    scan [string range $colorCurrentValue 1 2] "%x" colorBoxValue
    .colorBox.red set $colorBoxValue
    scan [string range $colorCurrentValue 3 4] "%x" colorBoxValue
    .colorBox.green set $colorBoxValue
    scan [string range $colorCurrentValue 5 6] "%x" colorBoxValue
    .colorBox.blue set $colorBoxValue

    .colorBox.current.current delete 0 end
    .colorBox.current.current insert 0 $colorCurrentColor
    ColorBoxSetColor $colorBoxMessage $colorBoxTargetW list $colorCurrentColor
    ColorBoxSetPaletteList $colorCurrentColor
  }
}


# Procedure: ColorBoxSetColor
proc ColorBoxSetColor { colorBoxMessage colorBoxTargetW colorBoxType colorBoxValue} {
# xf ignore me 6
  global colorBox

  .colorBox.red config  -command "NoFunction"
  .colorBox.green config  -command "NoFunction"
  .colorBox.blue config  -command "NoFunction"
  .colorBox.h config  -command "NoFunction"
  .colorBox.s config  -command "NoFunction"
  .colorBox.v config  -command "NoFunction"

  set colorBoxSetColor ""
  if {"$colorBoxValue" != ""} {
    if {"$colorBoxType" != "text"} {
      .colorBox.current.current delete 0 end
      .colorBox.current.current insert 0 $colorBoxValue
    }
    if {[string match "*oreground*" $colorBoxMessage]} {
      catch ".colorBox.demo config -foreground $colorBoxValue"
    } {
      catch ".colorBox.demo config -background $colorBoxValue"
    }
    if {"$colorBoxTargetW" != ""} {
      catch "$colorBoxTargetW config -[string tolower $colorBoxMessage]  $colorBoxValue"
    }
  }
  case $colorBoxType in {
    {text palette} {
      if {[string match "*oreground*" $colorBoxMessage]} {
        set red [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -foreground] 4]] 0]/256]
        set green [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -foreground] 4]] 1]/256]
        set blue [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -foreground] 4]] 2]/256]
      } {
        set red [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -background] 4]] 0]/256]
        set green [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -background] 4]] 1]/256]
        set blue [expr [lindex [winfo rgb .colorBox.demo [lindex [.colorBox.demo config -background] 4]] 2]/256]
      }
      if {"$colorBox(type)" == "rgb"} {
        .colorBox.red set $red
        .colorBox.green set $green
        .colorBox.blue set $blue
      } {
        if {"$colorBox(type)" == "hsv"} {
          set colorBoxHSV [ColorBoxRGBToHSV [expr $red*256] [expr $green*256] [expr $blue*256]]
          .colorBox.h set [format %.0f [expr [lindex $colorBoxHSV 0]*1000.0]]
          .colorBox.s set [format %.0f [expr [lindex $colorBoxHSV 1]*1000.0]]
          .colorBox.v set [format %.0f [expr [lindex $colorBoxHSV 2]*1000.0]]
        }
      }
    }
  }
  .colorBox.red config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""
  .colorBox.green config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""
  .colorBox.blue config  -command "ColorBoxSetRGBColor $colorBoxMessage \"$colorBoxTargetW\""
  .colorBox.h config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""
  .colorBox.s config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""
  .colorBox.v config  -command "ColorBoxSetHSVColor $colorBoxMessage \"$colorBoxTargetW\""
}


# Procedure: ColorBoxSetHSVColor
proc ColorBoxSetHSVColor { colorBoxMessage colorBoxTargetW colorBoxValue} {
# xf ignore me 6
  global colorBox

  set colorBoxRGB [ColorBoxHSVToRGB [expr [.colorBox.h get]/1000.0] [expr [.colorBox.s get]/1000.0] [expr [.colorBox.v get]/1000.0]]
  ColorBoxSetColor $colorBoxMessage $colorBoxTargetW hsv  [format #%04x%04x%04x [lindex $colorBoxRGB 0] [lindex $colorBoxRGB 1] [lindex $colorBoxRGB 2]]
  ColorBoxSetPaletteList [format #%04x%04x%04x [lindex $colorBoxRGB 0] [lindex $colorBoxRGB 1] [lindex $colorBoxRGB 2]]
}


# Procedure: ColorBoxSetPalette
proc ColorBoxSetPalette { colorBoxMessage colorBoxTargetW colorBoxElement} {
# xf ignore me 6
  global colorBox

  set colorBox(paletteNr) $colorBoxElement
  ColorBoxSetColor $colorBoxMessage $colorBoxTargetW palette  [lindex [.colorBox.palette.palette$colorBoxElement config -background] 4]
}


# Procedure: ColorBoxSetPaletteList
proc ColorBoxSetPaletteList { colorBoxValue} {
# xf ignore me 6
  global colorBox

  catch ".colorBox.palette.palette$colorBox(paletteNr) config  -activebackground $colorBoxValue"
  catch ".colorBox.palette.palette$colorBox(paletteNr) config  -background $colorBoxValue"
  set colorBox(palette)  [lreplace $colorBox(palette) $colorBox(paletteNr) $colorBox(paletteNr)  $colorBoxValue]
}


# Procedure: ColorBoxSetRGBColor
proc ColorBoxSetRGBColor { colorBoxMessage colorBoxTargetW colorBoxValue} {
# xf ignore me 6
  global colorBox

  ColorBoxSetColor $colorBoxMessage $colorBoxTargetW rgb  [format #%02x%02x%02x [.colorBox.red get]  [.colorBox.green get] [.colorBox.blue get]]
  ColorBoxSetPaletteList [format #%02x%02x%02x [.colorBox.red get]  [.colorBox.green get] [.colorBox.blue get]]
}


# Procedure: ColorBoxShowSlides
proc ColorBoxShowSlides { colorBoxMessage colorBoxTargetW} {
# xf ignore me 6
  global colorBox

  catch "pack unpack .colorBox.frame1"
  catch "pack unpack .colorBox.frame2"
  catch "pack unpack .colorBox.current"
  catch "pack unpack .colorBox.demo"
  catch "pack unpack .colorBox.h"
  catch "pack unpack .colorBox.s"
  catch "pack unpack .colorBox.v"
  catch "pack unpack .colorBox.red"
  catch "pack unpack .colorBox.green"
  catch "pack unpack .colorBox.blue"
  catch "pack unpack .colorBox.colors"
  case $colorBox(type) in {
    {rgb} {
      pack append .colorBox  .colorBox.frame1 {bottom fillx}  .colorBox.frame2 {bottom fillx}  .colorBox.current {bottom fillx}  .colorBox.palette {bottom fillx}  .colorBox.red {top fillx}  .colorBox.green {top fillx}  .colorBox.blue {top fillx}  .colorBox.demo {bottom fill expand}
    }
    {hsv} {
      pack append .colorBox  .colorBox.frame1 {bottom fillx}  .colorBox.frame2 {bottom fillx}  .colorBox.current {bottom fillx}  .colorBox.palette {bottom fillx}  .colorBox.h {top fillx}  .colorBox.s {top fillx}  .colorBox.v {top fillx}  .colorBox.demo {bottom fill expand}
    }
    {list} {
      pack append .colorBox  .colorBox.frame1 {bottom fillx}  .colorBox.frame2 {bottom fillx}  .colorBox.current {bottom fillx}  .colorBox.palette {bottom fillx}  .colorBox.demo {bottom fillx}  .colorBox.colors {top fill expand}
    }
  }
  if {[string match "*oreground*" $colorBoxMessage]} {
    ColorBoxSetColor $colorBoxMessage $colorBoxTargetW text  [lindex [.colorBox.demo config -foreground] 4]
  } {
    ColorBoxSetColor $colorBoxMessage $colorBoxTargetW text  [lindex [.colorBox.demo config -background] 4]
  }
}


# Procedure: InputBoxInternal
proc InputBoxInternal { inputBoxMessage inputBoxCommandOk inputBoxCommandCancel inputBoxGeometry inputBoxTitle lineNum} {
# xf ignore me 6
  global inputBox

  set tmpButtonOpt ""
  set tmpFrameOpt ""
  set tmpMessageOpt ""
  set tmpScaleOpt ""
  set tmpScrollOpt ""
  if {"$inputBox(activeBackground)" != ""} {
    append tmpButtonOpt "-activebackground \"$inputBox(activeBackground)\" "
  }
  if {"$inputBox(activeForeground)" != ""} {
    append tmpButtonOpt "-activeforeground \"$inputBox(activeForeground)\" "
  }
  if {"$inputBox(background)" != ""} {
    append tmpButtonOpt "-background \"$inputBox(background)\" "
    append tmpFrameOpt "-background \"$inputBox(background)\" "
    append tmpMessageOpt "-background \"$inputBox(background)\" "
  }
  if {"$inputBox(font)" != ""} {
    append tmpButtonOpt "-font \"$inputBox(font)\" "
    append tmpMessageOpt "-font \"$inputBox(font)\" "
  }
  if {"$inputBox(foreground)" != ""} {
    append tmpButtonOpt "-foreground \"$inputBox(foreground)\" "
    append tmpMessageOpt "-foreground \"$inputBox(foreground)\" "
  }
  if {"$inputBox(scrollActiveForeground)" != ""} {
    append tmpScrollOpt "-activeforeground \"$inputBox(scrollActiveForeground)\" "
  }
  if {"$inputBox(scrollBackground)" != ""} {
    append tmpScrollOpt "-background \"$inputBox(scrollBackground)\" "
  }
  if {"$inputBox(scrollForeground)" != ""} {
    append tmpScrollOpt "-foreground \"$inputBox(scrollForeground)\" "
  }

  # start build of toplevel
  if {"[info commands XFDestroy]" != ""} {
    catch {XFDestroy $inputBox(toplevelName)}
  } {
    catch {destroy $inputBox(toplevelName)}
  }
  toplevel $inputBox(toplevelName)  -borderwidth 0
  catch "$inputBox(toplevelName) config $tmpFrameOpt"
  if {[catch "wm geometry $inputBox(toplevelName) $inputBoxGeometry"]} {
    wm geometry $inputBox(toplevelName) 350x150
  }
  wm title $inputBox(toplevelName) $inputBoxTitle
  wm maxsize $inputBox(toplevelName) 1000 1000
  wm minsize $inputBox(toplevelName) 100 100
  # end build of toplevel

  message $inputBox(toplevelName).message1  -anchor "$inputBox(anchor)"  -justify "$inputBox(justify)"  -relief raised  -text "$inputBoxMessage"
  catch "$inputBox(toplevelName).message1 config $tmpMessageOpt"

  set xfTmpWidth  [string range $inputBoxGeometry 0 [expr [string first x $inputBoxGeometry]-1]]
  if {"$xfTmpWidth" != ""} {
    # set message size
    catch "$inputBox(toplevelName).message1 configure  -width [expr $xfTmpWidth-10]"
  } {
    $inputBox(toplevelName).message1 configure  -aspect 1500
  }

  frame $inputBox(toplevelName).frame0  -borderwidth 0  -relief raised
  catch "$inputBox(toplevelName).frame0 config $tmpFrameOpt"

  frame $inputBox(toplevelName).frame1  -borderwidth 0  -relief raised
  catch "$inputBox(toplevelName).frame1 config $tmpFrameOpt"

  if {$lineNum == 1} {
    scrollbar $inputBox(toplevelName).frame1.hscroll  -orient "horizontal"  -relief raised  -command "$inputBox(toplevelName).frame1.input view"
    catch "$inputBox(toplevelName).frame1.hscroll config $tmpScrollOpt"

    entry $inputBox(toplevelName).frame1.input  -relief raised  -scrollcommand "$inputBox(toplevelName).frame1.hscroll set"
    catch "$inputBox(toplevelName).frame1.input config $tmpMessageOpt"

    $inputBox(toplevelName).frame1.input insert 0  $inputBox($inputBox(toplevelName),inputOne)
    
    # bindings
    bind $inputBox(toplevelName).frame1.input <Return> "
      global inputBox
      set inputBox($inputBox(toplevelName),inputOne) \[$inputBox(toplevelName).frame1.input get\]
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy $inputBox(toplevelName)}
      } {
        catch {destroy $inputBox(toplevelName)}
      }
      $inputBoxCommandOk"
    
    # packing
    pack append $inputBox(toplevelName).frame1  $inputBox(toplevelName).frame1.hscroll {bottom fill}  $inputBox(toplevelName).frame1.input {top fill expand}
  } {
    text $inputBox(toplevelName).frame1.input  -relief raised  -wrap none  -borderwidth 2  -yscrollcommand "$inputBox(toplevelName).frame1.vscroll set"
    catch "$inputBox(toplevelName).frame1.input config $tmpMessageOpt"

    scrollbar $inputBox(toplevelName).frame1.vscroll  -relief raised  -command "$inputBox(toplevelName).frame1.input yview"
    catch "$inputBox(toplevelName).frame1.vscroll config $tmpScrollOpt"

    $inputBox(toplevelName).frame1.input insert 1.0  $inputBox($inputBox(toplevelName),inputMulti)

    # bindings
    bind $inputBox(toplevelName).frame1.input <Control-Return> "
      global inputBox
      set inputBox($inputBox(toplevelName),inputMulti) \[$inputBox(toplevelName).frame1.input get 1.0 end\]
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy $inputBox(toplevelName)}
      } {
        catch {destroy $inputBox(toplevelName)}
      }
      $inputBoxCommandOk"
    bind $inputBox(toplevelName).frame1.input <Meta-Return> "
      global inputBox
      set inputBox($inputBox(toplevelName),inputMulti) \[$inputBox(toplevelName).frame1.input get 1.0 end\]
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy $inputBox(toplevelName)}
      } {
        catch {destroy $inputBox(toplevelName)}
      }
      $inputBoxCommandOk"

    # packing
    pack append $inputBox(toplevelName).frame1  $inputBox(toplevelName).frame1.vscroll "$inputBox(scrollSide) filly"  $inputBox(toplevelName).frame1.input {left fill expand}
  }
  
  button $inputBox(toplevelName).frame0.button0  -text "OK"  -command "
      global inputBox
      if {$lineNum == 1} {
        set inputBox($inputBox(toplevelName),inputOne) \[$inputBox(toplevelName).frame1.input get\]
      } {
        set inputBox($inputBox(toplevelName),inputMulti) \[$inputBox(toplevelName).frame1.input get 1.0 end\]
      }
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy $inputBox(toplevelName)}
      } {
        catch {destroy $inputBox(toplevelName)}
      }
      $inputBoxCommandOk"
  catch "$inputBox(toplevelName).frame0.button0 config $tmpButtonOpt"

  button $inputBox(toplevelName).frame0.button1  -text "Cancel"  -command "
      global inputBox
      if {$lineNum == 1} {
        set inputBox($inputBox(toplevelName),inputOne) \"\"
      } {
        set inputBox($inputBox(toplevelName),inputMulti) \"\"
      }
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy $inputBox(toplevelName)}
      } {
        catch {destroy $inputBox(toplevelName)}
      }
      $inputBoxCommandCancel"
  catch "$inputBox(toplevelName).frame0.button1 config $tmpButtonOpt"

  pack append $inputBox(toplevelName).frame0  $inputBox(toplevelName).frame0.button0 {left fill expand}  $inputBox(toplevelName).frame0.button1 {left fill expand}

  pack append $inputBox(toplevelName)  $inputBox(toplevelName).frame0 {bottom fill}  $inputBox(toplevelName).frame1 {bottom fill expand}  $inputBox(toplevelName).message1 {top fill}
}


# Procedure: InputBoxMulti
proc InputBoxMulti { {inputBoxMessage "Input box:"} {inputBoxCommandOk ""} {inputBoxCommandCancel ""} {inputBoxGeometry "350x150"} {inputBoxTitle "Input box"}} {
# xf ignore me 5
##########
# Procedure: InputBoxMulti
# Description: show input box with one text line
# Arguments: {inputBoxMessage} - message to display
#            {inputBoxCommandOk} - the command to call after ok
#            {inputBoxCommandCancel} - the command to call after cancel
#            {inputBoxGeometry} - the geometry for the window
#            {inputBoxTitle} - the title for the window
# Returns: The entered text
# Sideeffects: none
# Notes: there exist also a function called:
#          InputBoxOne - to enter one line text
##########
#
# global inputBox(activeBackground) - active background color
# global inputBox(activeForeground) - active foreground color
# global inputBox(anchor) - anchor for message box
# global inputBox(background) - background color
# global inputBox(erase) - erase previous text
# global inputBox(font) - message font
# global inputBox(foreground) - foreground color
# global inputBox(justify) - justify for message box
# global inputBox(scrollActiveForeground) - scrollbar active background color
# global inputBox(scrollBackground) - scrollbar background color
# global inputBox(scrollForeground) - scrollbar foreground color
# global inputBox(scrollSide) - side where scrollbar is located
# global inputBox(toplevelName) - the toplevel name
# global inputBox(toplevelName,inputMulti) - the text in the text widget

  global inputBox

  if {"$inputBoxGeometry" == ""} {
    set inputBoxGeometry 350x150
  }
  if {$inputBox(erase)} {
    set inputBox($inputBox(toplevelName),inputMulti) ""
  } {
    if {![info exists inputBox($inputBox(toplevelName),inputMulti)]} {
      set inputBox($inputBox(toplevelName),inputMulti) ""
    }
  }
  InputBoxInternal $inputBoxMessage $inputBoxCommandOk $inputBoxCommandCancel $inputBoxGeometry $inputBoxTitle 2

  # wait for the box to be destroyed
  update idletask
  grab $inputBox(toplevelName)
  tkwait window $inputBox(toplevelName)

  return $inputBox($inputBox(toplevelName),inputMulti)
}


# Procedure: InputBoxOne
proc InputBoxOne { {inputBoxMessage "Input box:"} {inputBoxCommandOk ""} {inputBoxCommandCancel ""} {inputBoxGeometry "350x150"} {inputBoxTitle "Input box"}} {
# xf ignore me 5
##########
# Procedure: InputBoxOne
# Description: show input box with one text line
# Arguments: {inputBoxMessage} - message to display
#            {inputBoxCommandOk} - the command to call after ok
#            {inputBoxCommandCancel} - the command to call after cancel
#            {inputBoxGeometry} - the geometry for the window
#            {inputBoxTitle} - the title for the window
# Returns: The entered text
# Sideeffects: none
# Notes: there exist also a function called:
#          InputBoxMulti - to enter multiline text
##########
#
# global inputBox(activeBackground) - active background color
# global inputBox(activeForeground) - active foreground color
# global inputBox(anchor) - anchor for message box
# global inputBox(background) - background color
# global inputBox(erase) - erase previous text
# global inputBox(font) - message font
# global inputBox(foreground) - foreground color
# global inputBox(justify) - justify for message box
# global inputBox(scrollActiveForeground) - scrollbar active background color
# global inputBox(scrollBackground) - scrollbar background color
# global inputBox(scrollForeground) - scrollbar foreground color
# global inputBox(scrollSide) - side where scrollbar is located
# global inputBox(toplevelName) - the toplevel name
# global inputBox(toplevelName,inputOne) - the text in the entry widget

  global inputBox

  if {$inputBox(erase)} {
    set inputBox($inputBox(toplevelName),inputOne) ""
  } {
    if {![info exists inputBox($inputBox(toplevelName),inputOne)]} {
      set inputBox($inputBox(toplevelName),inputOne) ""
    }
  }
  InputBoxInternal $inputBoxMessage $inputBoxCommandOk $inputBoxCommandCancel $inputBoxGeometry $inputBoxTitle 1

  # wait for the box to be destroyed
  update idletask
  grab $inputBox(toplevelName)
  tkwait window $inputBox(toplevelName)

  return $inputBox($inputBox(toplevelName),inputOne)
}



# Procedure: collFSBox
proc collFSBox { {collFSBoxMessage "Select file:"} {collFSBoxFileName ""} {collFSBoxActionOk ""} {collFSBoxActionCancel ""}} {
# xf ignore me 5
##########
# Procedure: collFSBox
# Description: show file selector box
# Arguments: collFSBoxMessage - the text to display
#            collFSBoxFileName - a file name that should be selected
#            collFSBoxActionOk - the action that should be performed on ok
#            collFSBoxActionCancel - the action that should be performed on cancel
# Returns: the filename that was selected, or nothing
# Sideeffects: none
##########
# 
# global collFSBox(activeBackground) - active background color
# global collFSBox(activeForeground) - active foreground color
# global collFSBox(background) - background color
# global collFSBox(font) - text font
# global collFSBox(foreground) - foreground color
# global collFSBox(extensions) - scan directory for extensions
# global collFSBox(scrollActiveForeground) - scrollbar active background color
# global collFSBox(scrollBackground) - scrollbar background color
# global collFSBox(scrollForeground) - scrollbar foreground color
# global collFSBox(scrollSide) - side where scrollbar is located

  global collFSBox

  set tmpButtonOpt ""
  set tmpFrameOpt ""
  set tmpMessageOpt ""
  set tmpScaleOpt ""
  set tmpScrollOpt ""
  if {"$collFSBox(activeBackground)" != ""} {
    append tmpButtonOpt "-activebackground \"$collFSBox(activeBackground)\" "
  }
  if {"$collFSBox(activeForeground)" != ""} {
    append tmpButtonOpt "-activeforeground \"$collFSBox(activeForeground)\" "
  }
  if {"$collFSBox(background)" != ""} {
    append tmpButtonOpt "-background \"$collFSBox(background)\" "
    append tmpFrameOpt "-background \"$collFSBox(background)\" "
    append tmpMessageOpt "-background \"$collFSBox(background)\" "
  }
  if {"$collFSBox(font)" != ""} {
    append tmpButtonOpt "-font \"$collFSBox(font)\" "
    append tmpMessageOpt "-font \"$collFSBox(font)\" "
  }
  if {"$collFSBox(foreground)" != ""} {
    append tmpButtonOpt "-foreground \"$collFSBox(foreground)\" "
    append tmpMessageOpt "-foreground \"$collFSBox(foreground)\" "
  }
  if {"$collFSBox(scrollActiveForeground)" != ""} {
    append tmpScrollOpt "-activeforeground \"$collFSBox(scrollActiveForeground)\" "
  }
  if {"$collFSBox(scrollBackground)" != ""} {
    append tmpScrollOpt "-background \"$collFSBox(scrollBackground)\" "
  }
  if {"$collFSBox(scrollForeground)" != ""} {
    append tmpScrollOpt "-foreground \"$collFSBox(scrollForeground)\" "
  }

  if {[file exists [file tail $collFSBoxFileName]] &&
      [IsAFile [file tail $collFSBoxFileName]]} {
    set collFSBox(name) [file tail $collFSBoxFileName]
  } {
    set collFSBox(name) ""
  }
  if {[file exists $collFSBoxFileName] && [IsADir $collFSBoxFileName]} {
    set collFSBox(path) $collFSBoxFileName
  } {
    if {"[file dirname $collFSBoxFileName]" != "."} {
      set collFSBox(path) [file dirname $collFSBoxFileName]
    }
  }
  if {$collFSBox(showPixmap)} {
    set collFSBox(path) [string trimleft $collFSBox(path) @]
  }
  if {"$collFSBox(path)" != "" && [file exists $collFSBox(path)] &&
      [IsADir $collFSBox(path)]} {
    set collFSBox(internalPath) $collFSBox(path)
  } {
    if {"$collFSBox(internalPath)" == "" ||
        ![file exists $collFSBox(internalPath)]} {
      set collFSBox(internalPath) [pwd]
    }
  }
  # build widget structure

  # start build of toplevel
  if {"[info commands XFDestroy]" != ""} {
    catch {XFDestroy .collFSBox}
  } {
    catch {destroy .collFSBox}
  }
  toplevel .collFSBox  -borderwidth 0
  catch ".collFSBox config $tmpFrameOpt"
  wm geometry .collFSBox 350x300 
  wm title .collFSBox {File select box}
  wm maxsize .collFSBox 1000 1000
  wm minsize .collFSBox 100 100
  # end build of toplevel

  label .collFSBox.message1  -anchor c  -relief raised  -text "$collFSBoxMessage"
  catch ".collFSBox.message1 config $tmpMessageOpt"

  frame .collFSBox.frame1  -borderwidth 0  -relief raised
  catch ".collFSBox.frame1 config $tmpFrameOpt"

  button .collFSBox.frame1.ok  -text "OK"  -command "
      global collFSBox
      set collFSBox(name) \[.collFSBox.file.file get\]
      if {$collFSBox(showPixmap)} {
        set collFSBox(path) @\[.collFSBox.path.path get\]
      } {
        set collFSBox(path) \[.collFSBox.path.path get\]
      }
      set collFSBox(internalPath) \[.collFSBox.path.path get\]
      $collFSBoxActionOk
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .collFSBox}
      } {
        catch {destroy .collFSBox}
      }"
  catch ".collFSBox.frame1.ok config $tmpButtonOpt"

  button .collFSBox.frame1.rescan  -text "Rescan"  -command {
      global collFSBox
      collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)}
  catch ".collFSBox.frame1.rescan config $tmpButtonOpt"

  button .collFSBox.frame1.cancel  -text "Cancel"  -command "
      global collFSBox
      set collFSBox(name) {}
      set collFSBox(path) {}
      $collFSBoxActionCancel
      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .collFSBox}
      } {
        catch {destroy .collFSBox}
      }"
  catch ".collFSBox.frame1.cancel config $tmpButtonOpt"

  if {$collFSBox(showPixmap)} {
    frame .collFSBox.frame2  -borderwidth 0  -relief raised
    catch ".collFSBox.frame2 config $tmpFrameOpt"

    scrollbar .collFSBox.frame2.scrollbar3  -command {.collFSBox.frame2.canvas2 xview}  -orient {horizontal}  -relief {raised}
    catch ".collFSBox.frame2.scrollbar3 config $tmpScrollOpt"

    scrollbar .collFSBox.frame2.scrollbar1  -command {.collFSBox.frame2.canvas2 yview}  -relief {raised}
    catch ".collFSBox.frame2.scrollbar1 config $tmpScrollOpt"

    canvas .collFSBox.frame2.canvas2  -confine {true}  -relief {raised}  -scrollregion {0c 0c 20c 20c}  -width {100}  -xscrollcommand {.collFSBox.frame2.scrollbar3 set}  -yscrollcommand {.collFSBox.frame2.scrollbar1 set}
    catch ".collFSBox.frame2.canvas2 config $tmpFrameOpt"

    .collFSBox.frame2.canvas2 addtag currentBitmap withtag [.collFSBox.frame2.canvas2 create bitmap 5 5 -anchor nw]
  }

  frame .collFSBox.path  -borderwidth 0  -relief raised
  catch ".collFSBox.path config $tmpFrameOpt"

  frame .collFSBox.path.paths  -borderwidth 2  -relief raised
  catch ".collFSBox.path.paths config $tmpFrameOpt"

  menubutton .collFSBox.path.paths.paths  -borderwidth 0  -menu ".collFSBox.path.paths.paths.menu"  -relief flat  -text "Pathname:"
  catch ".collFSBox.path.paths.paths config $tmpButtonOpt"

  menu .collFSBox.path.paths.paths.menu
  catch ".collFSBox.path.paths.paths.menu config $tmpButtonOpt"

  .collFSBox.path.paths.paths.menu add command  -label "[string trimright $collFSBox(internalPath) {/@}]"  -command "
       global collFSBox
       collFSBoxFSShow \[.collFSBox.path.path get\]  \[.collFSBox.pattern.pattern get\] \$collFSBox(all)
       .collFSBox.path.path delete 0 end
       .collFSBox.path.path insert 0 [string trimright $collFSBox(internalPath) {/@}]"

  entry .collFSBox.path.path  -relief raised
  catch ".collFSBox.path.path config $tmpMessageOpt"

  if {![IsADir $collFSBox(internalPath)]} {
    set $collFSBox(internalPath) [pwd]
  }
  .collFSBox.path.path insert 0 $collFSBox(internalPath)

  frame .collFSBox.pattern  -borderwidth 0  -relief raised
  catch ".collFSBox.pattern config $tmpFrameOpt"

  frame .collFSBox.pattern.patterns  -borderwidth 2  -relief raised
  catch ".collFSBox.pattern.patterns config $tmpFrameOpt"

  menubutton .collFSBox.pattern.patterns.patterns  -borderwidth 0  -menu ".collFSBox.pattern.patterns.patterns.menu"  -relief flat  -text "Selection pattern:"
  catch ".collFSBox.pattern.patterns.patterns config $tmpButtonOpt"

  menu .collFSBox.pattern.patterns.patterns.menu
  catch ".collFSBox.pattern.patterns.patterns.menu config $tmpButtonOpt"

  .collFSBox.pattern.patterns.patterns.menu add checkbutton  -label "Scan extensions"  -variable collFSBox(extensions)  -command {
      global collFSBox
      collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)}

  entry .collFSBox.pattern.pattern  -relief raised
  catch ".collFSBox.pattern.pattern config $tmpMessageOpt"

  .collFSBox.pattern.pattern insert 0 $collFSBox(pattern)
  
  frame .collFSBox.files  -borderwidth 0  -relief raised
  catch ".collFSBox.files config $tmpFrameOpt"

  scrollbar .collFSBox.files.vscroll  -relief raised  -command ".collFSBox.files.files yview"
  catch ".collFSBox.files.vscroll config $tmpScrollOpt"

  scrollbar .collFSBox.files.hscroll  -orient horiz  -relief raised  -command ".collFSBox.files.files xview"
  catch ".collFSBox.files.hscroll config $tmpScrollOpt"

  listbox .collFSBox.files.files  -exportselection false  -relief raised  -xscrollcommand ".collFSBox.files.hscroll set"  -yscrollcommand ".collFSBox.files.vscroll set"
  catch ".collFSBox.files.files config $tmpMessageOpt"

  frame .collFSBox.file  -borderwidth 0  -relief raised
  catch ".collFSBox.file config $tmpFrameOpt"

  label .collFSBox.file.labelfile  -relief raised  -text "Filename:"
  catch ".collFSBox.file.labelfile config $tmpMessageOpt"

  entry .collFSBox.file.file  -relief raised
  catch ".collFSBox.file.file config $tmpMessageOpt"

  .collFSBox.file.file delete 0 end
  .collFSBox.file.file insert 0 $collFSBox(name)
  
  checkbutton .collFSBox.pattern.all  -offvalue 0  -onvalue 1  -text "Show all files"  -variable collFSBox(all)  -command {
      global collFSBox
      collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)}
  catch ".collFSBox.pattern.all config $tmpButtonOpt"

  collFSBoxFSShow $collFSBox(internalPath) $collFSBox(pattern) $collFSBox(all)

  # bindings
  bind .collFSBox.files.files <Double-Button-1> "
    collFSBoxFSFileSelectDouble %W $collFSBox(showPixmap) \{$collFSBoxActionOk\} %y"
  bind .collFSBox.files.files <ButtonPress-1> "
    collFSBoxFSFileSelect %W $collFSBox(showPixmap) %y"
  bind .collFSBox.files.files <Button1-Motion> "
    collFSBoxFSFileSelect %W $collFSBox(showPixmap) %y"
  bind .collFSBox.files.files <Shift-Button1-Motion> "
    collFSBoxFSFileSelect %W $collFSBox(showPixmap) %y"
  bind .collFSBox.files.files <Shift-ButtonPress-1> "
    collFSBoxFSFileSelect %W $collFSBox(showPixmap) %y"

  bind .collFSBox.path.path <Tab> {
    collFSBoxFSNameComplete path}
  bind .collFSBox.path.path <Return> {
    global tkVersion
    global collFSBox
    collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)
    collFSBoxFSInsertPath
    if {$tkVersion >= 3.0} {
      .collFSBox.file.file icursor end
    } {
      .collFSBox.file.file cursor end
    }
    focus .collFSBox.file.file}
  catch "bind .collFSBox.path.path <Up> {}"
  bind .collFSBox.path.path <Down> {
    global tkVersion
    if {$tkVersion >= 3.0} {
      .collFSBox.file.file icursor end
    } {
      .collFSBox.file.file cursor end
    }
    focus .collFSBox.file.file}

  bind .collFSBox.file.file <Tab> {
    collFSBoxFSNameComplete file}
  bind .collFSBox.file.file <Return> "
    global collFSBox
    set collFSBox(name) \[.collFSBox.file.file get\]
    if {$collFSBox(showPixmap)} {
      set collFSBox(path) @\[.collFSBox.path.path get\]
    } {
      set collFSBox(path) \[.collFSBox.path.path get\]
    }
    set collFSBox(internalPath) \[.collFSBox.path.path get\]
    $collFSBoxActionOk
    if {\"\[info commands XFDestroy\]\" != \"\"} {
      catch {XFDestroy .collFSBox}
    } {
      catch {destroy .collFSBox}
    }"
  bind .collFSBox.file.file <Up> {
    global tkVersion
    if {$tkVersion >= 3.0} {
      .collFSBox.path.path icursor end
    } {
      .collFSBox.path.path cursor end
    }
    focus .collFSBox.path.path}
  bind .collFSBox.file.file <Down> {
    global tkVersion
    if {$tkVersion >= 3.0} {
      .collFSBox.pattern.pattern icursor end
    } {
      .collFSBox.pattern.pattern cursor end
    }
    focus .collFSBox.pattern.pattern}

  bind .collFSBox.pattern.pattern <Return> {
    global collFSBox
    collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)}
  bind .collFSBox.pattern.pattern <Up> {
    global tkVersion
    if {$tkVersion >= 3.0} {
      .collFSBox.file.file icursor end
    } {
      .collFSBox.file.file cursor end
    }
    focus .collFSBox.file.file}
  catch "bind .collFSBox.pattern.pattern <Down> {}"

  # packing
  pack append .collFSBox.files  .collFSBox.files.vscroll "$collFSBox(scrollSide) filly"  .collFSBox.files.hscroll {bottom fillx}  .collFSBox.files.files {left fill expand}
  pack append .collFSBox.file  .collFSBox.file.labelfile {left}  .collFSBox.file.file {left fill expand}
  pack append .collFSBox.frame1  .collFSBox.frame1.ok {left fill expand}  .collFSBox.frame1.rescan {left fill expand}  .collFSBox.frame1.cancel {left fill expand}
  pack append .collFSBox.path.paths  .collFSBox.path.paths.paths {left}
  pack append .collFSBox.pattern.patterns  .collFSBox.pattern.patterns.patterns {left}
  pack append .collFSBox.path  .collFSBox.path.paths {left}  .collFSBox.path.path {left fill expand}
  pack append .collFSBox.pattern  .collFSBox.pattern.patterns {left}  .collFSBox.pattern.all {right fill}  .collFSBox.pattern.pattern {left fill expand}
  if {$collFSBox(showPixmap)} {
    pack append .collFSBox.frame2  .collFSBox.frame2.scrollbar1 {left filly}  .collFSBox.frame2.canvas2 {top expand fill}  .collFSBox.frame2.scrollbar3 {top fillx} 

    pack append .collFSBox  .collFSBox.message1 {top fill}  .collFSBox.frame1 {bottom fill}  .collFSBox.pattern {bottom fill}  .collFSBox.file {bottom fill}  .collFSBox.path {bottom fill}  .collFSBox.frame2 {right fill}  .collFSBox.files {left fill expand}
  } {
    pack append .collFSBox  .collFSBox.message1 {top fill}  .collFSBox.frame1 {bottom fill}  .collFSBox.pattern {bottom fill}  .collFSBox.file {bottom fill}  .collFSBox.path {bottom fill}  .collFSBox.files {left fill expand}
  }

  if {"$collFSBoxActionOk" == "" && "$collFSBoxActionCancel" == ""} {
    # wait for the box to be destroyed
    update idletask
    grab .collFSBox
    tkwait window .collFSBox

    if {"[string trim $collFSBox(path)]" != "" ||
        "[string trim $collFSBox(name)]" != ""} {
      if {"[string trimleft [string trim $collFSBox(name)] /]" == ""} {
        return [string trimright [string trim $collFSBox(path)] /]
      } {
        return [string trimright [string trim $collFSBox(path)] /]/[string trimleft [string trim $collFSBox(name)] /]
      }
    }
  }
}


# Procedure: collFSBoxBindSelectOne
proc collFSBoxBindSelectOne { collFSBoxW collFSBoxY} {
# xf ignore me 6

  set collFSBoxNearest [$collFSBoxW nearest $collFSBoxY]
  if {$collFSBoxNearest >= 0} {
    $collFSBoxW select from $collFSBoxNearest
    $collFSBoxW select to $collFSBoxNearest
  }
}


# Procedure: collFSBoxFSFileSelect
proc collFSBoxFSFileSelect { collFSBoxW collFSBoxShowPixmap collFSBoxY} {
# xf ignore me 6
  global collFSBox

  collFSBoxBindSelectOne $collFSBoxW $collFSBoxY
  set collFSBoxNearest [$collFSBoxW nearest $collFSBoxY]
  if {$collFSBoxNearest >= 0} {
    set collFSBoxTmpEntry [$collFSBoxW get $collFSBoxNearest]
    if {"[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "/" ||
        "[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "@"} {
      set collFSBoxFileName [string range $collFSBoxTmpEntry 0  [expr [string length $collFSBoxTmpEntry]-2]]
      if {![IsADir [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]] &&
          ![IsASymlink [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]]} {
        set collFSBoxFileName $collFSBoxTmpEntry
      }
    } {
      if {"[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "*"} {
        set collFSBoxFileName [string range $collFSBoxTmpEntry 0  [expr [string length $collFSBoxTmpEntry]-2]]
        if {![file executable $collFSBox(internalPath)/$collFSBoxFileName]} {
          set collFSBoxFileName $collFSBoxTmpEntry
        }
      } {
        set collFSBoxFileName $collFSBoxTmpEntry
      }
    }
    if {![IsADir [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]]} {
      set collFSBox(name) $collFSBoxFileName
      .collFSBox.file.file delete 0 end
      .collFSBox.file.file insert 0 $collFSBox(name)
      if {$collFSBoxShowPixmap} {
        catch ".collFSBox.frame2.canvas2 itemconfigure currentBitmap -bitmap \"@$collFSBox(internalPath)/$collFSBox(name)\""
      }
    }
  }
}


# Procedure: collFSBoxFSFileSelectDouble
proc collFSBoxFSFileSelectDouble { collFSBoxW collFSBoxShowPixmap collFSBoxAction collFSBoxY} {
# xf ignore me 6
  global collFSBox

  collFSBoxBindSelectOne $collFSBoxW $collFSBoxY
  set collFSBoxNearest [$collFSBoxW nearest $collFSBoxY]
  if {$collFSBoxNearest >= 0} {
    set collFSBoxTmpEntry [$collFSBoxW get $collFSBoxNearest]
    if {"$collFSBoxTmpEntry" == "../"} {
      set collFSBoxTmpEntry [string trimright [string trim $collFSBox(internalPath)] "@/"]
      if {"$collFSBoxTmpEntry" == ""} {
        return
      }
      collFSBoxFSShow [file dirname $collFSBoxTmpEntry]  [.collFSBox.pattern.pattern get] $collFSBox(all)
      .collFSBox.path.path delete 0 end
      .collFSBox.path.path insert 0 $collFSBox(internalPath)
    } {
      if {"[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "/" ||
          "[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "@"} {
        set collFSBoxFileName [string range $collFSBoxTmpEntry 0  [expr [string length $collFSBoxTmpEntry]-2]]
        if {![IsADir [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]] &&
            ![IsASymlink [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]]} {
          set collFSBoxFileName $collFSBoxTmpEntry
        }
      } {
        if {"[string index $collFSBoxTmpEntry  [expr [string length $collFSBoxTmpEntry]-1]]" == "*"} {
          set collFSBoxFileName [string range $collFSBoxTmpEntry 0  [expr [string length $collFSBoxTmpEntry]-2]]
          if {![file executable $collFSBox(internalPath)/$collFSBoxFileName]} {
            set collFSBoxFileName $collFSBoxTmpEntry
          }
        } {
          set collFSBoxFileName $collFSBoxTmpEntry
        }
      }
      if {[IsADir [string trimright $collFSBox(internalPath)/$collFSBoxFileName @]]} {
        set collFSBox(internalPath) "[string trimright $collFSBox(internalPath) {/@}]/$collFSBoxFileName"
        collFSBoxFSShow $collFSBox(internalPath)  [.collFSBox.pattern.pattern get] $collFSBox(all)
        .collFSBox.path.path delete 0 end
        .collFSBox.path.path insert 0 $collFSBox(internalPath)
      } {
        set collFSBox(name) $collFSBoxFileName
        if {$collFSBoxShowPixmap} {
          set collFSBox(path) @$collFSBox(internalPath)
        } {
          set collFSBox(path) $collFSBox(internalPath)
        }
        if {"$collFSBoxAction" != ""} {
          eval "global collFSBox; $collFSBoxAction"
        }
        if {"[info commands XFDestroy]" != ""} {
          catch {XFDestroy .collFSBox}
        } {
          catch {destroy .collFSBox}
        }
      }
    }
  }
}


# Procedure: collFSBoxFSInsertPath
proc collFSBoxFSInsertPath {} {
# xf ignore me 6
  global collFSBox

  set collFSBoxLast [.collFSBox.path.paths.paths.menu index last]
  set collFSBoxNewEntry [string trimright [.collFSBox.path.path get] "/@"]
  for {set collFSBoxCounter 0} {$collFSBoxCounter <= $collFSBoxLast} {incr collFSBoxCounter 1} {
    if {"$collFSBoxNewEntry" ==  "[lindex [.collFSBox.path.paths.paths.menu entryconfigure  $collFSBoxCounter -label] 4]"} {
      return
    }
  }
  if {$collFSBoxLast < 9} {
    .collFSBox.path.paths.paths.menu add command  -label "$collFSBoxNewEntry"  -command "
        global collFSBox
        collFSBoxFSShow $collFSBoxNewEntry  \[.collFSBox.pattern.pattern get\] \$collFSBox(all)
        .collFSBox.path.path delete 0 end
        .collFSBox.path.path insert 0 $collFSBoxNewEntry"
  } {
    for {set collFSBoxCounter 0} {$collFSBoxCounter < $collFSBoxLast} {incr collFSBoxCounter 1} {
      .collFSBox.path.paths.paths.menu entryconfigure  $collFSBoxCounter -label  [lindex [.collFSBox.path.paths.paths.menu entryconfigure  [expr $collFSBoxCounter+1] -label] 4]
      .collFSBox.path.paths.paths.menu entryconfigure $collFSBoxCounter  -command "
          global collFSBox
          collFSBoxFSShow [lindex [.collFSBox.path.paths.paths.menu entryconfigure  [expr $collFSBoxCounter+1] -label] 4]  \[.collFSBox.pattern.pattern get\] \$collFSBox(all)
          .collFSBox.path.path delete 0 end
          .collFSBox.path.path insert 0 [lindex  [.collFSBox.path.paths.paths.menu entryconfigure  [expr $collFSBoxCounter+1] -label] 4]"
    }
    .collFSBox.path.paths.paths.menu entryconfigure $collFSBoxLast  -label "$collFSBoxNewEntry"
    .collFSBox.path.paths.paths.menu entryconfigure $collFSBoxCounter  -command "
        global collFSBox
        collFSBoxFSShow \[.collFSBox.path.path get\]  \[.collFSBox.pattern.pattern get\] \$collFSBox(all)
        .collFSBox.path.path delete 0 end
        .collFSBox.path.path insert 0 $collFSBoxNewEntry"
  }
}


# Procedure: collFSBoxFSNameComplete
proc collFSBoxFSNameComplete { collFSBoxType} {
# xf ignore me 6
  global tkVersion
  global collFSBox

  set collFSBoxNewFile ""
  if {"$collFSBoxType" == "path"} {
    set collFSBoxDirName [file dirname [.collFSBox.path.path get]]
    set collFSBoxFileName [file tail [.collFSBox.path.path get]]
  } {
    set collFSBoxDirName [file dirname [.collFSBox.path.path get]/]
    set collFSBoxFileName [file tail [.collFSBox.file.file get]]
  }

  set collFSBoxNewFile ""
  if {[IsADir [string trimright $collFSBoxDirName @]]} {
    catch "glob -nocomplain $collFSBoxDirName/${collFSBoxFileName}*" collFSBoxResult
    foreach collFSBoxCounter $collFSBoxResult {
      if {"$collFSBoxNewFile" == ""} {
        set collFSBoxNewFile [file tail $collFSBoxCounter]
      } {
        if {"[string index [file tail $collFSBoxCounter] 0]" !=
            "[string index $collFSBoxNewFile 0]"} {
          set collFSBoxNewFile ""
          break
        }
        set collFSBoxCounter1 0
        set collFSBoxTmpFile1 $collFSBoxNewFile
        set collFSBoxTmpFile2 [file tail $collFSBoxCounter]
        set collFSBoxLength1 [string length $collFSBoxTmpFile1]
        set collFSBoxLength2 [string length $collFSBoxTmpFile2]
        set collFSBoxNewFile ""
        if {$collFSBoxLength1 > $collFSBoxLength2} {
          set collFSBoxLength1 $collFSBoxLength2
        }
        while {$collFSBoxCounter1 < $collFSBoxLength1} {
          if {"[string index $collFSBoxTmpFile1 $collFSBoxCounter1]" ==  "[string index $collFSBoxTmpFile2 $collFSBoxCounter1]"} {
            append collFSBoxNewFile [string index $collFSBoxTmpFile1 $collFSBoxCounter1]
          } {
            break
          }
          incr collFSBoxCounter1 1
        }
      }
    }
  }
  if {"$collFSBoxNewFile" != ""} {
    if {[IsADir [string trimright $collFSBoxDirName/$collFSBoxNewFile @]] ||
        ![IsAFile [string trimright $collFSBoxDirName/$collFSBoxNewFile @]]} {
      if {[IsADir [string trimright $collFSBoxDirName/$collFSBoxNewFile @]]} {
        if {"$collFSBoxDirName" == "/"} {
          .collFSBox.path.path delete 0 end
          .collFSBox.path.path insert 0 "/[string trimright [string trim $collFSBoxNewFile /] @]/"
        } {
          .collFSBox.path.path delete 0 end
          .collFSBox.path.path insert 0 "[string trimright $collFSBoxDirName /]/[string trimright [string trim $collFSBoxNewFile /] @]/"
        }
        collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)
        collFSBoxFSInsertPath
      } {
        .collFSBox.path.path delete 0 end
        .collFSBox.path.path insert 0 "[string trimright $collFSBoxDirName /]/[string trimright [string trim $collFSBoxNewFile /] @]"
      }
    } {
      .collFSBox.path.path delete 0 end
      .collFSBox.path.path insert 0 "[string trimright $collFSBoxDirName {@/}]/"
      .collFSBox.file.file delete 0 end
      .collFSBox.file.file insert 0 $collFSBoxNewFile
      if {$tkVersion >= 3.0} {
        .collFSBox.file.file icursor end
      } {
        .collFSBox.file.file cursor end
      }
      focus .collFSBox.file.file
    }
  }
}


# Procedure: collFSBoxFSShow
proc collFSBoxFSShow { collFSBoxPath collFSBoxPattern collFSBoxAll} {
# xf ignore me 6
  global collFSBox

  set tmpButtonOpt ""
  if {"$collFSBox(activeBackground)" != ""} {
    append tmpButtonOpt "-activebackground \"$collFSBox(activeBackground)\" "
  }
  if {"$collFSBox(activeForeground)" != ""} {
    append tmpButtonOpt "-activeforeground \"$collFSBox(activeForeground)\" "
  }
  if {"$collFSBox(background)" != ""} {
    append tmpButtonOpt "-background \"$collFSBox(background)\" "
  }
  if {"$collFSBox(font)" != ""} {
    append tmpButtonOpt "-font \"$collFSBox(font)\" "
  }
  if {"$collFSBox(foreground)" != ""} {
    append tmpButtonOpt "-foreground \"$collFSBox(foreground)\" "
  }

  set collFSBox(pattern) $collFSBoxPattern
  if {[file exists $collFSBoxPath] && [file readable $collFSBoxPath] &&
      [IsADir $collFSBoxPath]} {
    set collFSBox(internalPath) $collFSBoxPath
  } {
    if {[file exists $collFSBoxPath] && [file readable $collFSBoxPath] &&
        [IsAFile $collFSBoxPath]} {
      set collFSBox(internalPath) [file dirname $collFSBoxPath]
      .collFSBox.file.file delete 0 end
      .collFSBox.file.file insert 0 [file tail $collFSBoxPath]
      set collFSBoxPath $collFSBox(internalPath)
    } {
      while {"$collFSBoxPath" != "" && "$collFSBoxPath" != "/" &&
             ![file isdirectory $collFSBoxPath]} {
        set collFSBox(internalPath) [file dirname $collFSBoxPath]
         set collFSBoxPath $collFSBox(internalPath)
      }
    }
  }
  if {"$collFSBoxPath" == ""} {
    set collFSBoxPath "/"
    set collFSBox(internalPath) "/"
  }
  .collFSBox.path.path delete 0 end
  .collFSBox.path.path insert 0 $collFSBox(internalPath)

  if {[.collFSBox.files.files size] > 0} {
    .collFSBox.files.files delete 0 end
  }
  if {$collFSBoxAll} {
    if {[catch "exec ls -F -a $collFSBoxPath" collFSBoxResult]} {
      puts stderr "$collFSBoxResult"
    }
  } {
    if {[catch "exec ls -F $collFSBoxPath" collFSBoxResult]} {
      puts stderr "$collFSBoxResult"
    }
  }
  set collFSBoxElementList [lsort $collFSBoxResult]

  foreach collFSBoxCounter [winfo children .collFSBox.pattern.patterns.patterns] {
    if {[string length [info commands XFDestroy]] > 0} {
      catch {XFDestroy $collFSBoxCounter}
    } {
      catch {destroy $collFSBoxCounter}
    }
  }
  menu .collFSBox.pattern.patterns.patterns.menu
  catch ".collFSBox.pattern.patterns.patterns.menu config $tmpButtonOpt"

  if {$collFSBox(extensions)} {
    .collFSBox.pattern.patterns.patterns.menu add command  -label "*"  -command {
        global collFSBox
        set collFSBox(pattern) "*"
        .collFSBox.pattern.pattern delete 0 end
        .collFSBox.pattern.pattern insert 0 $collFSBox(pattern)
        collFSBoxFSShow [.collFSBox.path.path get] $collFSBox(pattern)  $collFSBox(all)}
  }

  if {"$collFSBoxPath" != "/"} {
    .collFSBox.files.files insert end "../"
  }
  foreach collFSBoxCounter $collFSBoxElementList {
    if {[string match $collFSBoxPattern $collFSBoxCounter] ||
        [IsADir [string trimright $collFSBoxPath/$collFSBoxCounter "/@"]]} {
      if {"$collFSBoxCounter" != "../" &&
          "$collFSBoxCounter" != "./"} {
        .collFSBox.files.files insert end $collFSBoxCounter
      }
    }

    if {$collFSBox(extensions)} {
      catch "file rootname $collFSBoxCounter" collFSBoxRootName
      catch "file extension $collFSBoxCounter" collFSBoxExtension
      set collFSBoxExtension [string trimright $collFSBoxExtension "/*@"]
      if {"$collFSBoxExtension" != "" && "$collFSBoxRootName" != ""} {
        set collFSBoxInsert 1
        set collFSBoxLast [.collFSBox.pattern.patterns.patterns.menu index last]
        for {set collFSBoxCounter1 0} {$collFSBoxCounter1 <= $collFSBoxLast} {incr collFSBoxCounter1 1} {
          if {"*$collFSBoxExtension" ==  "[lindex [.collFSBox.pattern.patterns.patterns.menu entryconfigure  $collFSBoxCounter1 -label] 4]"} {
            set collFSBoxInsert 0
          }
        }
	if {$collFSBoxInsert} {
          .collFSBox.pattern.patterns.patterns.menu add command  -label "*$collFSBoxExtension"  -command "
              global collFSBox
              set collFSBox(pattern) \"*$collFSBoxExtension\"
              .collFSBox.pattern.pattern delete 0 end
              .collFSBox.pattern.pattern insert 0 \$collFSBox(pattern)
              collFSBoxFSShow \[.collFSBox.path.path get\] \$collFSBox(pattern)  \$collFSBox(all)"
        }
      }
    }
  }
  if {$collFSBox(extensions)} {
    .collFSBox.pattern.patterns.patterns.menu add separator
  }
  if {$collFSBox(extensions) || 
      "[.collFSBox.pattern.patterns.patterns.menu index last]" == "none"} {
    .collFSBox.pattern.patterns.patterns.menu add checkbutton  -label "Scan extensions"  -variable "collFSBox(extensions)"  -command {
        global collFSBox
        collFSBoxFSShow [.collFSBox.path.path get]  [.collFSBox.pattern.pattern get] $collFSBox(all)}
  }
}


# Internal procedures



# startup source
proc StartupSrc {args} {

}
global env
global collVect
set collVect(source_dir) $env(ASCENDDIST)/models/examples/flexible_design

source $collVect(source_dir)/base_procedures.tcl
source $collVect(source_dir)/colors.tcl
source $collVect(source_dir)/design.tcl
source $collVect(source_dir)/flexible.tcl
source $collVect(source_dir)/procedures.tcl


# prepare auto loading
global auto_path
global tk_library
global xfLoadPath
foreach xfElement [eval list [split $xfLoadPath :] $auto_path] {
  if {[file exists $xfElement/tclIndex]} {
    lappend auto_path $xfElement
  }
}
catch "unset auto_index"

catch "unset auto_oldpath"

catch "unset auto_execs"


# startup source
StartupSrc

# initialize global variables
proc InitGlobals {} {

  # please don't modify the following
  # variables. They are needed by xf.
  global {autoLoadList}
  set {autoLoadList(base_procedures.tcl)} {0}
  set {autoLoadList(colors.tcl)} {0}
  set {autoLoadList(design.tcl)} {0}
  set {autoLoadList(flexible.tcl)} {0}
  set {autoLoadList(main.tcl)} {0}
  set {autoLoadList(procedures.tcl)} {0}
  global {internalAliasList}
  set {internalAliasList} {}
  global {moduleList}
  global {preloadList}
  set {preloadList(xfInternal)} {}
  global {symbolicName}
  set {symbolicName(root)} {.}
  global {xfWmSetPosition}
  set {xfWmSetPosition} {}
  global {xfWmSetSize}
  set {xfWmSetSize} {}
  global {xfAppDefToplevels}
  set {xfAppDefToplevels} {}
}

# initialize global variables
InitGlobals

# display/remove toplevel windows.
ShowWindow.

global xfShowWindow.designtop
set xfShowWindow.designtop 1
ShowWindow.designtop

# load default bindings.
if {[info exists env(XF_BIND_FILE)] &&
    "[info procs XFShowHelp]" == ""} {
  source $env(XF_BIND_FILE)
}

# parse and apply application defaults.
XFLocalLoadAppDefs Main
XFLocalSetAppDefs

# eof
#


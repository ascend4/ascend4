#  main.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.9 $
#  Last modified on: $Date: 1998/11/22 16:04:52 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: main.tcl,v $
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

# Program: main
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module inclusion
global env
global xfLoadPath
global xfLoadInfo
set xfLoadInfo 0
global tcl_platform
if {[string compare tcl_platform(platform) "unix"]==0} {
if {[info exists env(XF_LOAD_PATH)]} {
  if {[string first $env(XF_LOAD_PATH) "/usr1/ballan/local/XF/bin:/usr1/ballan/local/xf2.2:.:$env(ASCENDTK)"] == -1} {
    set xfLoadPath "$env(XF_LOAD_PATH):/usr1/ballan/local/XF/bin:/usr1/ballan/local/xf2.2:.:$env(ASCENDTK)"
  } else {
    set xfLoadPath "/usr1/ballan/local/XF/bin:/usr1/ballan/local/xf2.2:.:$env(ASCENDTK)"
  }
} else {
  set xfLoadPath "/usr1/ballan/local/XF/bin:/usr1/ballan/local/xf2.2:.:$env(ASCENDTK)"
}
} else {
  set xfLoadPath $env(ASCENDTK)
}

global argc
global argv
set tmpArgv ""
for {set counter 0} {$counter < $argc} {incr counter 1} {
  switch [string tolower [lindex $argv $counter]] {
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
      if {$tk_version >= 3.0} {
        tk colormodel . monochrome
      }
    }
    {-xfmodelcolor} {
      if {$tk_version >= 3.0} {
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


# procedure to show window . taken over by Glob_do_GNU
# proc ShowWindow. {args}
proc ShowWindow. {args} {# xf ignore me 7

StartupSrc.

  # Window manager configurations
# wm positionfrom . user
# wm sizefrom . user
  update idletask
# wm iconify .
# wm maxsize . 85 92
# wm minsize . 0 0
# wm title . {ASCEND IV}


EndSrc.

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree ."
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc StartupSrc. {args}
proc StartupSrc. {args} {
# root startup entrance
}

# proc EndSrc. {}
proc EndSrc. {} {
# root startup exit
}


# User defined procedures


# Internal procedures



# sources the rest of ascend window modules
# module load procedure
proc XFLocalIncludeModule {{moduleName ""}} {
  global env
  global xfLoadInfo
  global xfLoadPath
  global xfStatus
  global tcl_platform
  set pathsep ":"
  if {![string compare $tcl_platform(platform) "windows"]} {
    set pathsep ";"
  }
  foreach p [split $xfLoadPath $pathsep] {
    if {[file exists "$p/$moduleName"]} {
      if {![file readable "$p/$moduleName"]} {
        puts stderr "Cannot read $p/$moduleName (permission denied)"
        continue
      }
      if {$xfLoadInfo} {
        puts stdout "Loading $p/$moduleName..."
      }
      source "$p/$moduleName"
      return 1
    }
    # first see if we have a load command
    if {[info exists env(XF_VERSION_SHOW)]} {
      set xfCommand $env(XF_VERSION_SHOW)
      regsub -all {\$xfFileName} $xfCommand $p/$moduleName xfCommand
      if {$xfLoadInfo} {
        puts stdout "Loading $p/$moduleName...($xfCommand)"
      }
      if {[catch "$xfCommand" contents]} {
        continue
      } else {
        eval $contents
        return 1
      }
    }
#    # are we able to load versions from wish ?
#    if {[catch "afbind $p/$moduleName" aso]} {
#      # try to use xf version load command
#      global xfVersion
#      if {[info exists xfVersion(showDefault)]} {
#        set xfCommand $xfVersion(showDefault)
#      } else {
#	# our last hope
#        set xfCommand "vcat -q $p/$moduleName"
#      }
#      regsub -all {\$xfFileName} $xfCommand $p/$moduleName xfCommand
#      if {$xfLoadInfo} {
#        puts stdout "Loading $p/$moduleName...($xfCommand)"
#      }
#      if {[catch "$xfCommand" contents]} {
#        continue
#      } else {
#        eval $contents
#        return 1
#      }
#    } else {
#      # yes we can load versions directly
#      if {[catch "$aso open r" inFile]} {
#        puts stderr "Cannot open $p/[$aso attr af_bound] (permission denied)"
#        continue
#      }
#      if {$xfLoadInfo} {
#        puts stdout "Loading $p/[$aso attr af_bound]..."
#      }
#      if {[catch "read \{$inFile\}" contents]} {
#        puts stderr "Cannot read $p/[$aso attr af_bound] (permission denied)"
#        close $inFile
#        continue
#      }
#      close $inFile
#      eval $contents
#      return 1
#    }
#  }
  puts stderr "Cannot load module $moduleName -- check your xf load path"
  puts stderr "Specify a xf load path with the environment variable:"
  puts stderr "  XF_LOAD_PATH (e.g \"export XF_LOAD_PATH=.\")"
  puts stderr "to quit, type 'exit'."
#  catch "destroy ."
#  catch "exit 0"
}

# application parsing procedure
proc XFLocalParseAppDefs {xfAppDefFile} {
  global xfAppDefaults

  # basically from: Michael Moore
  if {[file exists $xfAppDefFile] &&
      [file readable $xfAppDefFile] &&
      "[file type $xfAppDefFile]" == "link"} {
    catch "file type $xfAppDefFile" xfType
    while {"$xfType" == "link"} {
      if {[catch "file readlink $xfAppDefFile" xfAppDefFile]} {
        return
      }
      catch "file type $xfAppDefFile" xfType
    }
  }
  if {!("$xfAppDefFile" != "" &&
        [file exists $xfAppDefFile] &&
        [file readable $xfAppDefFile] &&
        "[file type $xfAppDefFile]" == "file")} {
    return
  }
  if {![catch "open $xfAppDefFile r" xfResult]} {
    set xfAppFileContents [read $xfResult]
    close $xfResult
    foreach line [split $xfAppFileContents "\n"] {
      # backup indicates how far to backup.  It applies to the
      # situation where a resource name ends in . and when it
      # ends in *.  In the second Case you want to keep the *
      # in the widget name for pattern matching, but you want
      # to get rid of the . if it is the end of the name.
      set backup -2
      set line [string trim $line]
      if {[string index $line 0] == "#" || "$line" == ""} {
        # skip comments and empty lines
        continue
      }
      set list [split $line ":"]
      set resource [string trim [lindex $list 0]]
      set i [string last "." $resource]
      set j [string last "*" $resource]
      if {$j > $i} {
        set i $j
        set backup -1
      }
      incr i
      set name [string range $resource $i end]
      incr i $backup
      set widname [string range $resource 0 $i]
      set value [string trim [lindex $list 1]]
      if {"$widname" != "" && "$widname" != "*"} {
        # insert the widget and resourcename to the application
        # defaults list.
        if {![info exists xfAppDefaults]} {
          set xfAppDefaults ""
        }
        lappend xfAppDefaults [list $widname [string tolower $name] $value]
      }
    }
  }
}

# application loading procedure
proc XFLocalLoadAppDefs {{xfClasses ""} {xfPriority "startupFile"} {xfAppDefFile ""}} {
  global env

  if {"$xfAppDefFile" == ""} {
    set xfFileList ""
    if {[info exists env(XUSERFILESEARCHPATH)]} {
      append xfFileList [split $env(XUSERFILESEARCHPATH) :]
    }
    if {[info exists env(XAPPLRESDIR)]} {
      append xfFileList [split $env(XAPPLRESDIR) :]
    }
    if {[info exists env(XFILESEARCHPATH)]} {
      append xfFileList [split $env(XFILESEARCHPATH) :]
    }
    append xfFileList " /usr/lib/X11/app-defaults"
    append xfFileList " /usr/X11/lib/X11/app-defaults"

    foreach xfCounter1 $xfClasses {
      foreach xfCounter2 $xfFileList {
        set xfPathName $xfCounter2
        if {[regsub -all "%N" "$xfPathName" "$xfCounter1" xfResult]} {
          set xfPathName $xfResult
        }
        if {[regsub -all "%T" "$xfPathName" "app-defaults" xfResult]} {
          set xfPathName $xfResult
        }
        if {[regsub -all "%S" "$xfPathName" "" xfResult]} {
          set xfPathName $xfResult
        }
        if {[regsub -all "%C" "$xfPathName" "" xfResult]} {
          set xfPathName $xfResult
        }
        if {[file exists $xfPathName] &&
            [file readable $xfPathName] &&
            ("[file type $xfPathName]" == "file" ||
             "[file type $xfPathName]" == "link")} {
          catch "option readfile $xfPathName $xfPriority"
          if {"[info commands XFParseAppDefs]" != ""} {
            XFParseAppDefs $xfPathName
          } else {
            if {"[info commands XFLocalParseAppDefs]" != ""} {
              XFLocalParseAppDefs $xfPathName
            }
          }
        } else {
          if {[file exists $xfCounter2/$xfCounter1] &&
              [file readable $xfCounter2/$xfCounter1] &&
              ("[file type $xfCounter2/$xfCounter1]" == "file" ||
               "[file type $xfCounter2/$xfCounter1]" == "link")} {
            catch "option readfile $xfCounter2/$xfCounter1 $xfPriority"
            if {"[info commands XFParseAppDefs]" != ""} {
              XFParseAppDefs $xfCounter2/$xfCounter1
            } else {
              if {"[info commands XFLocalParseAppDefs]" != ""} {
                XFLocalParseAppDefs $xfCounter2/$xfCounter1
              }
            }
          }
        }
      }
    }
  } else {
    # load a specific application defaults file
    if {[file exists $xfAppDefFile] &&
        [file readable $xfAppDefFile] &&
        ("[file type $xfAppDefFile]" == "file" ||
         "[file type $xfAppDefFile]" == "link")} {
      catch "option readfile $xfAppDefFile $xfPriority"
      if {"[info commands XFParseAppDefs]" != ""} {
        XFParseAppDefs $xfAppDefFile
      } else {
        if {"[info commands XFLocalParseAppDefs]" != ""} {
          XFLocalParseAppDefs $xfAppDefFile
        }
      }
    }
  }
}

# application setting procedure
proc XFLocalSetAppDefs {{xfWidgetPath "."}} {
  global xfAppDefaults

  if {![info exists xfAppDefaults]} {
    return
  }
  foreach xfCounter $xfAppDefaults {
    if {"$xfCounter" == ""} {
      break
    }
    set widname [lindex $xfCounter 0]
    if {[string match $widname ${xfWidgetPath}] ||
        [string match "${xfWidgetPath}*" $widname]} {
      set name [string tolower [lindex $xfCounter 1]]
      set value [lindex $xfCounter 2]
      # Now lets see how many tcl commands match the name
      # pattern specified.
      set widlist [info command $widname]
      if {"$widlist" != ""} {
        foreach widget $widlist {
          # make sure this command is a widget.
          if {![catch "winfo id $widget"] &&
              [string match "${xfWidgetPath}*" $widget]} {
            catch "$widget configure -$name $value"
          }
        }
      }
    }
  }
}

XFLocalIncludeModule browser.tcl
XFLocalIncludeModule display.tcl
XFLocalIncludeModule generalk.tcl
XFLocalIncludeModule pane.tcl
XFLocalIncludeModule library.tcl
XFLocalIncludeModule typetree.tcl
XFLocalIncludeModule probe.tcl
XFLocalIncludeModule script.tcl
XFLocalIncludeModule solver.tcl
XFLocalIncludeModule debug.tcl
XFLocalIncludeModule mtx.tcl
XFLocalIncludeModule toolbox.tcl
XFLocalIncludeModule util.tcl
XFLocalIncludeModule units.tcl

# prepare auto loading
global auto_path
global xfLoadPath
foreach xfElement [eval list [split $xfLoadPath :] $auto_path] {
  if {[file exists $xfElement/tclIndex]} {
    lappend auto_path $xfElement
  }
}
catch "unset auto_index"

catch "unset auto_oldpath"

#catch "unset auto_execs"


# initialize global variables
proc InitGlobals {} {
  global {alertBox}
  set {alertBox(activeBackground)} {}
  set {alertBox(activeForeground)} {}
  set {alertBox(after)} {0}
  set {alertBox(anchor)} {nw}
  set {alertBox(background)} {}
  set {alertBox(button)} {0}
  set {alertBox(font)} {}
  set {alertBox(foreground)} {}
  set {alertBox(justify)} {center}
  set {alertBox(toplevelName)} {.alertBox}

  # please don't modify the following
  # variables. They are needed by xf.
  global {autoLoadList}
  set {autoLoadList(browser.tcl)} {0}
  set {autoLoadList(display.tcl)} {0}
  set {autoLoadList(generalk.tcl)} {0}
  set {autoLoadList(pane.tcl)} {0}
  set {autoLoadList(library.tcl)} {0}
  set {autoLoadList(typetree.tcl)} {0}
  set {autoLoadList(main.tcl)} {0}
  set {autoLoadList(probe.tcl)} {0}
  set {autoLoadList(script.tcl)} {0}
  set {autoLoadList(solver.tcl)} {0}
  set {autoLoadList(debug.tcl)} {0}
  set {autoLoadList(mtx.tcl)} {0}
  set {autoLoadList(toolbox.tcl)} {0}
  set {autoLoadList(util.tcl)} {0}
  set {autoLoadList(units.tcl)} {0}
  global {internalAliasList}
  set {internalAliasList} {}
  global {moduleList}
  set {moduleList(browser.tcl)} { .browser}
  set {moduleList(display.tcl)} { .display}
  set {moduleList(generalk.tcl)} { Alias GetSelection MenuPopupAdd MenuPopupMotion MenuPopupPost MenuPopupRelease NoFunction OptionButtonGet OptionButtonSet SN SymbolicName Unalias cls ascclearlist ascfileread readdir updatelist ascGetSelection}
  set {moduleList(library.tcl)} { .library}
  set {moduleList(typetree.tcl)} { .typetree}
  set {moduleList(main.tcl)} { .}
  set {moduleList(probe.tcl)} { .probe}
  set {moduleList(script.tcl)} { .script}
  set {moduleList(solver.tcl)} { .solver}
  set {moduleList(debug.tcl)} { .debug}
  set {moduleList(mtx.tcl)} { .mtx}
  set {moduleList(toolbox.tcl)} { .toolbox do_raise do_raise_lower}
  set {moduleList(util.tcl)} { .util}
  set {moduleList(units.tcl)} { .units}
  global {preloadList}
  set {preloadList(xfInternal)} {}
  global {symbolicName}
  set {symbolicName(root)} {.}
  global {xfWmSetPosition}
  set {xfWmSetPosition} {.toolbox}
  global {xfWmSetSize}
  set {xfWmSetSize} {.toolbox .sims .script .display .library .probe .units}
  global {xfAppDefToplevels}
  set {xfAppDefToplevels} {}
}

# initialize global variables
# InitGlobals

# display/remove toplevel windows.
ShowWindow.

global xfShowWindow.browser
set xfShowWindow.browser 0

global xfShowWindow.display
set xfShowWindow.display 0

global xfShowWindow.library
set xfShowWindow.library 0

global xfShowWindow.probe
set xfShowWindow.probe 0

global xfShowWindow.script
set xfShowWindow.script 0

global xfShowWindow.sims
set xfShowWindow.sims 0

global xfShowWindow.solver
set xfShowWindow.solver 0

global xfShowWindow.toolbox
set xfShowWindow.toolbox 0

global xfShowWindow.util
set xfShowWindow.util 0

global xfShowWindow.units
set xfShowWindow.units 0

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


#  UtilProc.tcl: ascend path/unix utility box procedures
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.38 $
#  Last modified on: $Date: 1998/06/18 15:55:07 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: UtilProc.tcl,v $
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

#------------------------------------------------------------------------
# each system default not configured by
# Xresources or ascend.ad should have an entry in the ascUtilVect
# The list of subscripts is keyed to the list of labels through
# the util_get_label procedure so we don't actually have to
# add new widget definitions every time we get a new utility:
# just update the  procedures
# util_init
# util_get_label
# util_get_var
#
# display is alpha-by-utilvect-subscript order
# any sanity checking desired must be done at button  press time
#
# util init:creates a utilvect entry for each option we want to appear
# on the unix page. the values in sub must be appropriate as array
# subscripts, duh, and widget names.
#

# Work around for tcl's braindead file volume command which
# demands media in every drive before returning.
# This simply gets the list of lettered devices c-z: that exist
proc windoze_file_volume {} {
  set result {}
  foreach i {c d e f g h i j k l m n o p q r s t u v w x y z} {
    if {[file exists $i:]} {
      lappend result $i:
    }
  }
  if {![llength $result]} {
    lappend result a: b:
  }
  return $result
}

#
# This function tries to guess windoze tmp directory, since
# the idiots didn't standardize it.
proc set_Windoze_tmp {} {
  global ascUtilVect env ascGlobalVect
  if {[info exists ascUtilVect(asctmp)] && \
      [file writable $ascUtilVect(asctmp)] && \
      [file isdirectory $ascUtilVect(asctmp)]} {
    if {![info exists env(TMPDIR)]} {
      set env(TMPDIR) $ascUtilVect(asctmp)
    }
    return
  }
  set drive [windoze_file_volume]
  set suffix [list \
    {/Tmp} \
    {/Temp} \
    {/ASCEND IV/Tmp} \
    {/WinNT/Temp} \
    {/Win95/Temp} \
    {/Windows/Temp} \
    {/Program Files/Temp} \
    {/Program Files/ASCEND IV/Tmp} \
  ]
  set dl {}
  foreach dr $drive {
    if {[string compare [string tolower $dr] "a:"] && \
        [string compare [string tolower $dr] "b:"]} {
      foreach su $suffix {
        lappend dl $dr$su
      }
    }
  }
  if {[info exists ascGlobalVect(userhome)]} {
    lappend dl $ascGlobalVect(userhome)/../tmp
  }
  if {[info exists env(TMPDIR)]} {
    linsert dl 0 $env(TMPDIR)
  }
  if {[info exists env(TMP)]} {
    linsert dl 0 $env(TMP)
  }
  if {[info exists env(TEMP)]} {
    linsert dl 0 $env(TEMP)
  }
  foreach d $dl {
    set ascUtilVect(asctmp) [file rootname $d ] 
    if {[file isdirectory $ascUtilVect(asctmp)] && \
        [file writable $ascUtilVect(asctmp)]} {
      if {![info exists env(TMPDIR)]} {
        set env(TMPDIR) $ascUtilVect(asctmp)
      }
      return
    }
  }
  set ascUtilVect(asctmp) c:/temp
  if {![info exists env(TMPDIR)]} {
    set env(TMPDIR) $ascUtilVect(asctmp)
  }
  set str "Could not find a writable scratch directory. Tried:"
  append str " " $dl
  append str " Open the ASCEND System Utilities tool and set one."
  append str " Otherwise, some functions like Display Code do not work."
  Script_Raise_Alert $str
  return
}
#
# proc set_Util_Defaults {}
#------------------------------------------------------------------------
# set utility defaults here. note: any values not stored in
# utilvect must be initialized elsewhere, eg. set_Toolbox_Defaults
#------------------------------------------------------------------------
proc set_Util_Defaults {} {
  global ascUtilVect tcl_platform env
  if {[string compare $tcl_platform(platform) windows]==0} {
    set ascUtilVect(edit) "runemacs"
    set ascUtilVect(ghostview) "ghostview"
    set_Windoze_tmp
    set ascUtilVect(plot_type) "xgraph"
    set ascUtilVect(plot_command) \{$env(ASCENDDIST)
    append ascUtilVect(plot_command) "/../Tcl/bin/tkxgraph.exe\} -- -f"
  } else {
    set ascUtilVect(edit) "emacs"
    set ascUtilVect(ghostview) "ghostview"
    set ascUtilVect(asctmp) "/tmp"
    set ascUtilVect(plot_type) "xgraph"
    set ascUtilVect(plot_command) "xgraph"
  }
  set_Print_Defaults
}

#
# proc Util_Init {}
#------------------------------------------------------------------------
# Utilvect valu check here to guarantee widget creation.
# All variables must ultimately resolve to vector entries.
# We will _not_ have single variables running around loose in Util
#------------------------------------------------------------------------
proc Util_Init {} {
  global ascUtilSubs ascUtilVect
  set subs "dir"
  lappend subs asclib ascdist tklib tcllib printer
  lappend subs print edit ghostview asctmp xspread webstart
  lappend subs webkick webroot plot_type plot_command
 # if not set, set dummy so it gets on the util page
  set alist [array names ascUtilVect]
  foreach s $subs {
    if {[lsearch $alist $s]=="-1"} {
      set ascUtilVect($s) "unset"
    }
  }
  set ascUtilSubs $subs
}

#
# proc Util_Get_Label {defsub}
#------------------------------------------------------------------------
# take a util default subscript and return its label string
# for the utility box
#------------------------------------------------------------------------
proc Util_Get_Label {defsub} {
  switch $defsub {
    {asclib}      {return "ASCENDLIBRARY path"}
    {webstart}    {return "WWW startup"}
    {webroot}     {return "WWW root URL"}
    {ascdist}     {return "ASCENDDIST dir"}
    {asctmp}      {return "Scratch directory"}
    {dir}         {return "Working Directory"}
    {edit}        {return "Text edit command"}
    {ghostview}   {return "Postscript viewer"}
    {plot_command} {return "Plot program name"}
    {plot_type}   {return "Plot file type"}
    {print}       {return "Text print command"}
    {printer}     {return "PRINTER"}
    {webkick}     {return "WWW restart"}
    {tcllib}      {return "TCL_LIBRARY"}
    {tklib}       {return "TK_LIBRARY dir"}
    {xspread}     {return "Spreadsheet command"}
    default       {error "util_get_label called with bad option $defsub"}
  }
}

#
# proc Util_Get_Var {defsub}
#------------------------------------------------------------------------
# take a util default subscript and return its variable
# for the utility box. you better have set these vars
# elsewhere or trap those not ever set
#------------------------------------------------------------------------
proc Util_Get_Var {defsub} {
  switch $defsub {
    {dir}         {return "ascToolVect(dirinput)"}
    {asclib}      {return "env(ASCENDLIBRARY)"}
    {ascdist}     {return "env(ASCENDDIST)"}
    {webstart}    {return "ascHelpVect(start_command)"}
    {webroot}     {return "ascHelpVect(rootURL)"}
    {asctmp}      {return "ascUtilVect(asctmp)"}
    {tklib}       {return "env(TK_LIBRARY)"}
    {tcllib}      {return "env(TCL_LIBRARY)"}
    {plot_type}   {return "ascUtilVect(plot_type)"}
    {plot_command} {return "ascUtilVect(plot_command)"}
    {printer}     {return "env(PRINTER)"}
    {print}       {return "ascToolVect(printargs)"}
    {webkick}     {return "ascHelpVect(restart_command)"}
    {ghostview}   {return "ascUtilVect(ghostview)"}
    {edit}        {return "ascUtilVect(edit)"}
    {xspread}     {return "ascUtilVect(xspread)"}
    default       {error "util_get_var called with bad option $defsub"}
  }
}

#
# proc Util_do_OK {}
#------------------------------------------------------------------------
# any return from here before the Destroy leaves the utilbox up.
# do sanity checks here where those failing cause a return
#------------------------------------------------------------------------
proc Util_do_OK {} {
  global ascUtilSubs ascUtilVect
  foreach i $ascUtilSubs {
    set s [Util_Get_Var $i]
    set aname [lindex [split $s (] 0]
    global $aname
    set $s [string trim [set $s]]
  }
  if {[catch {Tool_set_dir} ]} {return}
  global ascUtilVect
  if {[catch { set ascUtilVect(asctmp) \
               [file rootname $ascUtilVect(asctmp) ] } ] } {
    return
  }
  if {![file isdirectory $ascUtilVect(asctmp)] || \
      ![file writable $ascUtilVect(asctmp)]} {
    set str $ascUtilVect(asctmp)
    append str " is not a writable Scratch directory"
    Script_Raise_Alert $str
    return
  }
  Tool_printinstr
  DestroyWindow.util
}

#
# proc Util_do_Read {}
#------------------------------------------------------------------------
# Reads in settings from ~/ascdata/ascend-config if that is available.
#------------------------------------------------------------------------
proc Util_do_Read {} {
  global env 
  set configread 0

  if {!$configread && [file exists ~/ascdata/ascend-config] &&
      [file readable ~/ascdata/ascend-config] &&
      ("[file type ~/ascdata/ascend-config]" == "file" ||
       "[file type ~/ascdata/ascend-config]" == "link")} {
    if {[catch "uplevel {source [glob ~/ascdata/ascend-config]}" ]} {
      puts stderr "error in config file: [glob ~/ascdata/ascend-config]"
    } else {
      set configread 1
    }
  }

}

#
# proc Util_do_Save {}
#------------------------------------------------------------------------
# save ~/ascdata/ascend-config. overwrite whatever there.
#------------------------------------------------------------------------
proc Util_do_Save {} {
  global ascUtilVect ascGlobalVect

  if {$ascGlobalVect(saveoptions) == 0} {
    puts stdout "cannot write ascend configure file"
    return;
  }

  if {[catch {set conffile [open ~/ascdata/ascend-config w]} ]} {
    Script_Raise_Alert "Error writing ~/ascdata/ascend-config."
    return
  }

  set ostr \
    "\# Parameters in this file may be changed, saved, and read at any time."
  puts $conffile $ostr
  puts $conffile "\# Actions in the .ascendrc are only done at startup time."
  set ostr \
    "\# This file is overwritten when toolbox.utilities.save is pressed."
  puts $conffile $ostr
  puts $conffile "\# Environment variables are not saved and set here."
  set subs [lsort [array names ascUtilVect]]
  foreach s $subs {
    set varname [Util_Get_Var $s]
    set aname [lindex [split $varname (] 0]
    global $varname
# we don't save environment vars
    if {[string range $varname 0 2] !="env"} {
      puts $conffile "  global $aname"
      puts $conffile "  set $varname {[.util.box.main_frm.val_frm.$s get]}"
    }
  }
  close $conffile
  puts stdout "wrote config file."
}

#
# proc Util_do_Help {}
#------------------------------------------------------------------------
# Util Help button
#------------------------------------------------------------------------
proc Util_do_Help {} {
  Help_button utilities
}

#
# proc ascFindFile {filename code args}
#------------------------------------------------------------------------
# File find procedure. baa 3-94
# given a filename, a control code, and zero or more searchpaths,
# will return the first, last or all matches to a filename in the 
# search paths.
# paths are searched in the order given and may be compound paths
# separated by colons.
# Each match is returned as a full path name.
# filename is first warped to the nativename format
# If file starts with / or ~ file will be globbed and the first match
# returned rather than searched for.
# under windoze, \ or ~ will be globbed instead of / and ~.
#
# If no path is given, . will be assumed.
# If some path is given, . will NOT be assumed
# If redundant paths are given, redundant entries will be returned
# on the list.
#
# code must be one of:  first last all
# for all, return is a list of name elements
# for first or last return is a single full path name
#------------------------------------------------------------------------
proc ascFindFile {filename code args} {
  global tcl_platform
  set filename [file nativename $filename]
  switch $code {
    {first} -
    {last} -
    {all} {}
    default {error "bad call to ascFindFile: want first last or all"}
  }
  if {[file exists $filename]} {
    if {[file readable $filename]} {
      return $filename
    }
  }
  set fc [string range $filename 0 0]
  if {($fc == "/" ) ||
      ($fc == "\\" && $tcl_platform(platform)=="windows") } {
    return $filename
  }
  if {$fc == "~"} {
    if {[catch {set tfile [lindex [glob $filename] 0]} foo ]} {
      puts stderr $foo
      return ""
    } {
      return $tfile
    }
  }
  if {$args == ""} {set searchlist "."} {set searchlist ""}
  foreach p $args {
    set dlist ""
    if {$tcl_platform(platform)=="unix"} {
      set dlist [split $p :]
    }
    if {$tcl_platform(platform)=="windows"} {
      set dlist [split $p \;]
    }
    foreach d $dlist {
      lappend searchlist $d ;# redundant?
    }
  }
  set foundlist ""
  set SLASH "/"
  if {$tcl_platform(platform)=="windows"} {
    set SLASH "\\"
  }
  foreach d $searchlist {
    if {[file exists "${d}${SLASH}$filename"]} {
      if {![file readable "${d}${SLASH}$filename"]} {
        puts stderr "Cannot read $d$SLASH$filename (permission denied)"
      } else {
        lappend foundlist ${d}${SLASH}$filename
        if {$code == "first"} {
          return [file nativename [lindex $foundlist 0]]
        }
      }
    }
  }
  if {$code == "last"} {
    return [file nativename [lindex $foundlist end]]
  }
  return [file nativename $foundlist]
}

#
# proc Util_Label_Width {}
#------------------------------------------------------------------------
# pick out the maximum width of a Util window label
#------------------------------------------------------------------------
proc Util_Label_Width {} {
  global ascUtilVect
  set items [array names ascUtilVect]
  set wid 0
  foreach i $items {
    set sl  [string length "[Util_Get_Label $i]"]
    if {[expr $sl > $wid]} {set wid $sl}
  }
  return $wid
}

proc set_Print_Defaults {} {
  global tcl_platform ascPrintVect env
  set ascPrintVect(grab) 1
  set ascPrintVect(cancellable) 1
  set ascPrintVect(entrywidth) 20
  set ascPrintVect(npages) 1
  set ascPrintVect(toplevel) .printconfig
  set ascPrintVect(title) "Printer setup"
  set ascPrintVect(helpcommand) {Help_button print}
  set ascPrintVect(usercheckcommand) Print_checkinput
  set ascPrintVect(whenokcommand) ""
  set ascPrintVect(namelist) [list \
    destination \
    printername \
    filename \
    enscriptflags \
    customcommand \
  ]
  if {$tcl_platform(platform) == "unix"} {
    set ascPrintVect(destination.choices) [list \
      {Print} \
      {Write to file} \
      {Append to file} \
      {Enscript} \
      {Custom}
    ]
  } else {
    # until we can cope with PC idiocy
    set ascPrintVect(destination.choices) Print
  }
  set ascPrintVect(destination) {Print}
  set ascPrintVect(destination.type) string
  set ascPrintVect(destination.label) Destination
  set ascPrintVect(filename) Noname
  set ascPrintVect(filename.type) string
  set ascPrintVect(filename.label) "Name of file"

  set ascPrintVect(enscriptflags) {-2rG}
  set ascPrintVect(enscriptflags.type) string
  set ascPrintVect(enscriptflags.label) "Enscript flags"

  set ascPrintVect(customcommand) {>> /dev/null cat}
  set ascPrintVect(customcommand.type) string
  set ascPrintVect(customcommand.label) "User print command"

  # set the default printer name to mirage, then overwrite it with the
  # value in the PRINTER or LPDEST environment variable.  Finally, set
  # the PRINTER environment variable so the Utilities window will
  # display the current printer
  set ascPrintVect(printername) mirage
  if {[catch {set ascPrintVect(printername) $env(PRINTER)}]} {
    catch {set ascPrintVect(printername) $env(LPDEST)}
  }
  set env(PRINTER) $ascPrintVect(printername)

  set ascPrintVect(printername.type) string
  set ascPrintVect(printername.label) "Printer"

  if {$tcl_platform(platform) == "unix"} {
    # Use SysV-style `lp' command if on HP-UX, IRIX, or Solaris
    # Use BSD-style `lpr' command everywhere else
    switch -regexp $tcl_platform(os)$tcl_platform(osVersion) {
      HP-UX*  -
      IRIX*   -
      SunOS5* {
	set ascPrintVect(Print) {lp -d%Printer%}
	set ascPrintVect(Enscript) {enscript %Enscript% -d%Printer%}
      }
      default {
	set ascPrintVect(Print) {lpr -P%Printer%}
	set ascPrintVect(Enscript) {enscript %Enscript% -P%Printer%}
      }
    } ;# endsw
    set ascPrintVect(Write) {> %File% cat}
    set ascPrintVect(Append) {>> %File% cat}
  } else {
    set ascPrintVect(Print) {notepad /p}
    # 3 NOT accessed since only Print is offered under windows.
    set ascPrintVect(Write) {> %File% type}
    set ascPrintVect(Append) {>> %File% type}
    set ascPrintVect(Enscript) {enscript %Enscript%P%Printer%}
  }
}

proc Print_checkinput {args} {
  global ascToolVect env ascPrintVect
  set option [lindex $ascPrintVect(destination) 0]
  switch $option {
  Print {
    if {"[stringcompact $ascPrintVect(printername)]" == ""} {
      error "You must specify a printer"
    }
    regsub %Printer% $ascPrintVect(Print) \
           $ascPrintVect(printername) ascToolVect(printargs)
    return
  }
  Write {
    if {"[stringcompact $ascPrintVect(filename)]" == ""} {
      error "You must specify a name for the file written"
    }
    set fname [file nativename $ascPrintVect(filename)]
    if {[file exists $fname]} {
      if {[file isfile $fname] && [file writable $fname]} {
        set not [Script_Raise_Alert "File exists. Ok to replace?" \
          "Print to file question"]
        if {$not} {error "Please specify a new file name"}
      } else {
        error "Please specify a writable file name"
      }
    }
    regsub %File% $ascPrintVect(Write) $fname ascToolVect(printargs)
    return
  }
  Append {
    if {"[stringcompact $ascPrintVect(filename)]" == ""} {
      error "You must specify a name for the file to append"
    }
    set fname [file nativename $ascPrintVect(filename)]
    if {![file exists $fname]} {
      set not [Script_Raise_Alert "File does not exist. Ok to create?" \
        "Print to file question"]
      if {$not} {error "Please specify a new file name"}
    } else {
      if {![file isfile $fname] || ![file writable $fname]} {
        error "File not appendable. Please specify a new file name"
      }
    }
    regsub %File% $ascPrintVect(Append) $fname ascToolVect(printargs)
    return
  }
  Enscript {
    if {"[stringcompact $ascPrintVect(printername)]" == ""} {
      error "You must specify a printer"
    }
    regsub %Printer% $ascPrintVect(Enscript) \
           $ascPrintVect(printername) tmp1
    regsub %Enscript% $tmp1 \
           $ascPrintVect(enscriptflags) ascToolVect(printargs)
    return
  }
  Custom {
    if {"[stringcompact $ascPrintVect(customcommand)]" == ""} {
      error "You must specify a custom printing command"
    }
    set ascToolVect(printargs) $ascPrintVect(customcommand)
    return
  }
  default {error "Unknown destination specified to print dialog"}
  } ;# endswitch 
}

proc Print_cancelcheck {args} {
  global ascPrintVect
  return $ascPrintVect(__,cancelled)
}

#
# call this and Print_cancelcheck in sequence always
proc Print_configure {window {destination ""}} {
  global ascToolVect env ascPrintVect
  set g +10+10
  catch {
    set w [winfo toplevel $window]
    set g "+[winfo x $w]+[winfo y $w]"
  }
  if {$destination !=""} {
    set ascPrintVect(destination) $destination
  }
  ascParPage ascPrintVect $g 1
  set env(PRINTER) $ascPrintVect(printername)
  set env(LPDEST)  $ascPrintVect(printername)
}

#  methods.tcl: METHOD suggestion generation
#  By Benjamin A Allan
#  Created May 14, 1998
#  Part of ASCEND
#  Revision: $Revision: 1.3 $
#  Last modified on: $Date: 1998/06/18 15:55:31 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: methods.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1998 Carnegie Mellon University
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

# Goal is to suggest method bodies for the user to
# improve upon and thus feel useful.
# Since method generation is a messy business and a separate
# concept from window management, these functions are in
# a separate file.

# To be called from LibraryProc.tcl defaults function.
proc set_MethodsDefaults {} {
  global ascLibrVect
  set ascLibrVect(dofmethods) seqmod
  set ascLibrVect(standardmethods) [list \
    default_self \
    check_self \
    scale_self \
    bound_self \
    default_all \
    check_all \
    bound_all \
    scale_all \
    specify \
  ]
  set ascLibrVect(varmethods) [list bound scale default]
  # check, specify are not concerned with vars mostly.
  set ascLibrVect(generate_ADDMETHOD) 0
  set metainfo [libr_query -childinfo]
  set infoindex 0
  foreach i $metainfo {
    set ikey [lindex [split $i -] 0]
    set ascLibrVect(cinfo.$ikey) $infoindex
    incr infoindex
  }
}

# args: opened file fid, loaded type type
# Main entry point for clients.
# The goal of this method, besides generating methods semi-intelligently,
# is to do it without being dependent on hardcoded numeric positions
# of elements in the list of type childinfo. We manage this by using
# the syntactic information -childinfo returns when called with no args.
# We have given up on true genericity of method generation for this attempt.
proc asc_suggest_methods {fid type} {
  global ascLibrVect gmvect ascDispVect
  catch {unset gmvect}
  #gmvect is a global that does no persist across calls to be examined after
  
  set gmvect(comment) $ascDispVect(ShowComments)
  set gmvect(firstdone) 0
  set newmethods [libr_query -methods -type $type]
  set gmvect(newisavar) {}    ;# list of newly declared local variables
  set gmvect(newwillbevar) {} ;# list of new passed variables
  set gmvect(newisa) {}       ;# list of newly declared local parts
  set gmvect(newwillbe) {}    ;# list of newly received by pointer parts
  set newchildren [libr_query -childnames -type $type] ;# all children
  set ancestorlist [libr_query -ancestors -type $type] ;# all ancestors
  set gmvect(oldisavar) {}    ;# list of local variables in ancestor
  set gmvect(oldwillbevar) {} ;# list of passed variables in ancestor
  set gmvect(oldisa) {}       ;# list of local parts in ancestor
  set gmvect(oldwillbe) {}    ;# list of passed parts in ancestor
  set gmvect(ancestor) {}     ;# last ancestor of type
  set oldmethods {}           ;# list of methods in ancestor
  set oldchildren {}          ;# list of all children in ancestor
  if {[llength $ancestorlist]} {
    set ancestor [lindex $ancestorlist 0]
    set gmvect(ancestor) $ancestor
    set oldchildren [libr_query -childnames -type $ancestor]
    set oldmethods [libr_query -methods -type $ancestor]
  }
  foreach c $oldchildren {
    set cinfo [libr_query -childinfo -type $ancestor -child $c]
    set willbe [lindex $cinfo $ascLibrVect(cinfo.willbe)]
    set isa [lindex $cinfo $ascLibrVect(cinfo.isa)]
    set kind [lindex $cinfo $ascLibrVect(cinfo.basetype)] ;# array base
    switch $kind {
    MODEL {
        if {$isa} { 
          lappend gmvect(oldisa) $c
        }
        if {$willbe} {
          lappend gmvect(oldwillbe) $c
        }
      }
    when -
    relation -
    logrelation -
    real_constant -
    integer_constant -
    symbol_constant -
    boolean_constant -
    set {
       # do nothing for constants
      }
    default {
        # variable type
        if {$isa} { 
          lappend gmvect(oldisavar) $c
        }
        if {$willbe} {
          lappend gmvect(oldwillbevar) $c
        }
      }
    }
  }
  foreach c $newchildren {
    if {[lsearch $oldchildren $c] != -1} { # old part
      continue;
    }
    set cinfo [libr_query -childinfo -type $type -child $c]
    set willbe [lindex $cinfo $ascLibrVect(cinfo.willbe)]
    set isa [lindex $cinfo $ascLibrVect(cinfo.isa)]
    set kind [lindex $cinfo $ascLibrVect(cinfo.basetype)] 
    switch $kind {
    MODEL {
        if {$isa} { 
          lappend gmvect(newisa) $c
        }
        if {$willbe} {
          lappend gmvect(newwillbe) $c
        }
      }
    when -
    relation -
    logrelation -
    real_constant -
    integer_constant -
    symbol_constant -
    boolean_constant -
    set {
       # do nothing for constants
      }
    default {
        # variable type
        if {$isa} { 
          lappend gmvect(newisavar) $c
        }
        if {$willbe} {
          lappend gmvect(newwillbevar) $c
        }
      }
    }
  }
  foreach m $ascLibrVect(standardmethods) {
    set needed 0
    set suffix [lindex [split $m _] end]
    set prefix [lindex [split $m _] 0]
    if {[lsearch $newmethods $m] == -1} { 
      # not inherited
      set needed 1
    }
    switch $suffix {
    all {
        if {[llength $gmvect(newwillbe)] || \
            [llength $gmvect(newwillbevar)] || \
            $needed} {
          # call the particular method generation
          generate_$m $fid $type 
        } else {
          generate_inheritance $fid $m
        }
      }
    self {
        if {[llength $gmvect(newisa)] || \
            [llength $gmvect(newisavar)] || \
            $needed} {
          generate_$m $fid $type
        } else {
          generate_inheritance $fid $m
        }
      }
    default {
        generate_$m $fid $type
      }
    }
  }
  if {$ascLibrVect(generate_ADDMETHOD)} {
    puts $fid "END METHODS;"  
  }
}

proc generate_inheritance {fid m} {
  puts $fid "(* METHOD $m is already written or is inherited properly. *)"
}

# This method should account for arrayness from childinfo.
# at present it is stupid. User may need to put FOR loops
# and indices on array children. Probably should at least
# echo the array children named.
# sep should be "" . or :: as needed.
proc generate_run {fid type child sep m} {
  if {[string compare $sep "::"] != 0} {
    set child [generate_fullname $type $child]
  }
  puts $fid "\tRUN $child$sep$m;"
}

# generate method opening
proc generate_header {fid type m} {
  global ascLibrVect gmvect

  if {!$gmvect(firstdone)} {
    if {$ascLibrVect(generate_ADDMETHOD)} {
      puts $fid "ADD METHODS IN $type;"
    } else {
      puts $fid "METHODS"
    }
    puts $fid \
      "(* generated code for $type to be customized or corrected. *)"
    set gmvect(firstdone) 1
  }
  puts $fid "METHOD $m;"
}

# generate method closing
proc generate_footer {fid type m} {
  global ascLibrVect gmvect
  puts $fid "END $m;"
  puts $fid ""
}

# generate scaling update (child.nominal) heuristically
proc generate_NominaL {fid type child} {
  if {![generate_is_solver_var $type $child]} {
    return
  }
  set child [generate_fullname $type $child]
  puts $fid "\t$child.nominal := abs($child) * 1.001 + 1.0e-4{?};"
}

# generate_is_solver_var type child returns 1 if child is a solvervar
proc generate_is_solver_var {type child} {
  global ascLibrVect
  set cinfo [libr_query -childinfo -type $type -child $child]
  set type [lindex $cinfo $ascLibrVect(cinfo.guesstype)]
  set ancestors [libr_query -ancestors -type $type]
  if {[lsearch -exact $ancestors solver_var] == -1} {
    return 0
  }
  set fullname [lindex $cinfo $ascLibrVect(cinfo.fullname)]
  return 1
}

# lookup the indexed name of an array child
proc generate_fullname {type child} {
  global ascLibrVect
  if {[string compare $child ""]==0} {return ""}
  set cinfo [libr_query -childinfo -type $type -child $child]
  set fullname [lindex $cinfo $ascLibrVect(cinfo.fullname)]
  return $fullname
}

# generate bounds update (child.*_bound) heuristically
proc generate_BoundS {fid type child} {
  if {![generate_is_solver_var $type $child]} {
    return
  }
  set child [generate_fullname $type $child]
  puts $fid "\t$child.lower_bound := $child - boundwidth*$child.nominal;"
  puts $fid "\t$child.upper_bound := $child + boundwidth*$child.nominal;"
}

# generate default values stupidly
proc generate_DefaulT {fid type child} {
  set child [generate_fullname $type $child]
  puts $fid "\t$child\t:=  ;"
}

# Returns the dof name first found in methods of child from
# those in doflist. If doflist empty, will return reset.
# In any case, if child is not a MODEL or array thereof,
# returns the empty string. Also returns empty string if
# child is 'passed down'.
proc generate_dofname {type child doflist} {
  global ascLibrVect gmvect
  set cinfo [libr_query -childinfo -type $type -child $child]
  if {[string compare [lindex $cinfo $ascLibrVect(cinfo.basetype)] "MODEL"]} {
    return ""; # not a MODEL means no reset, duh.
  }
  if {[lindex $cinfo $ascLibrVect(cinfo.passed)]} {
    return ""
  }
  set mlist [libr_query -methods \
               -type [lindex $cinfo $ascLibrVect(cinfo.guesstype)]]
  foreach m $doflist {
    if {[lsearch -exact $mlist $m] >= 0 && $m != {}} {
      return $m
    }
  }
  return reset
}
# ---------------------------------------------------------------------
# The following procs mirror our conventions for writing methods.
# Unfortunately the conventions and generated comments are specific
# for each method. Each method m in $ascLibrVect(standardmethods) should
# have a generate_$m function here.
# ---------------------------------------------------------------------

# call check_all on passed in parts, call check_self.
proc generate_check_all {fid type} {
  global gmvect

  generate_header $fid $type check_all
  if {$gmvect(comment)} {
    puts $fid \
      "\t(* Array parts and variables need subscripts and FOR/DO loops. *)"
  }

  foreach child $gmvect(oldwillbe) {
    generate_run $fid $type $child . check_all
  }
  foreach child $gmvect(newwillbe) {
    generate_run $fid $type $child . check_all
  }

  generate_run $fid $type "" "" check_self
  generate_footer $fid $type check_all
}

# call check_all on passed in parts, call check_self.
proc generate_check_self {fid type} {
  global gmvect

  generate_header $fid $type check_self
  if {[llength $gmvect(newisa)]} {
    if {$gmvect(comment)} {
      puts $fid \
        "\t(* Put new high-level checks here before checking new parts. *)"
      puts $fid \
        "\t(* Array parts probably need subscripts and FOR/DO loops. *)"
    }
  } else {
    if {$gmvect(comment)} {
      puts $fid "\t(* Put new high-level checks here. *)"
    }
  }

  if {[llength $gmvect(oldisa)]} {
    generate_run $fid $type $gmvect(ancestor) :: check_self
  }
  foreach child $gmvect(newisa) {
    generate_run $fid $type $child . check_self
  }

  generate_footer $fid $type check_self
}

# call scale_all on passed in parts, rescale passed in vars, call scale_self.
proc generate_scale_all {fid type} {
  global gmvect

  generate_header $fid $type scale_all
  if {$gmvect(comment)} {
    puts $fid \
      "\t(* Array parts and variables need subscripts and FOR/DO loops. *)"
  }

  foreach child $gmvect(oldwillbe) {
    generate_run $fid $type $child . scale_all
  }
  foreach child $gmvect(newwillbe) {
    generate_run $fid $type $child . scale_all
  }

  if {[llength $gmvect(oldwillbevar)] || [llength $gmvect(newwillbevar)]} {
    if {$gmvect(comment)} {
      puts $fid \
        "\t(* .nominal assignments may need value/units corrected for 1.0e-4."
      puts $fid "\t * Some .nominal assignments may need to be deleted."
      puts $fid "\t *)"
    }
  }
  foreach child $gmvect(oldwillbevar) {
    generate_NominaL $fid $type $child
  }
  foreach child $gmvect(newwillbevar) {
    generate_NominaL $fid $type $child
  }  

  generate_run $fid $type "" "" scale_self
  generate_footer $fid $type scale_all
}

# call old scale method, call new parts, new vars scalings.
proc generate_scale_self {fid type} {
  global gmvect

  generate_header $fid $type scale_self
  if {[llength $gmvect(oldisavar)] || [llength $gmvect(oldisa)]} {
    generate_run $fid $type $gmvect(ancestor) :: scale_self
  }

  foreach child $gmvect(newisa) {
    generate_run $fid $type $child . scale_self
  }

  if {[llength $gmvect(newisavar)]} {
    if {$gmvect(comment)} {
      puts $fid \
        "\t(* .nominal assignments may need value/units corrected for 1.0e-4."
      puts $fid "\t * Some .nominal assignments may need to be deleted."
      puts $fid "\t *)"
    }
  }
  foreach child $gmvect(newisavar) {
    generate_NominaL $fid $type $child
  }

  generate_footer $fid $type scale_self
}

# call bound_all on passed in parts, rebound passed in vars, call bound_self.
# whither boundwidth?
proc generate_bound_all {fid type} {
  global gmvect

  generate_header $fid $type bound_all
  if {$gmvect(comment)} {
    puts $fid \
      "\t(* Array parts and variables need subscripts and FOR/DO loops."
    puts $fid \
      "\t * Generated code assumes boundwidth IS_A bound_width; in $type."
    puts $fid \
      "\t *)"
  }

  foreach child $gmvect(oldwillbe) {
    generate_run $fid $type $child . bound_all
  }
  foreach child $gmvect(newwillbe) {
    generate_run $fid $type $child . bound_all
  }


  if {[llength $gmvect(oldwillbevar)] || [llength $gmvect(newwillbevar)]} {
    if {$gmvect(comment)} {
      puts $fid "\t(* Bound assignments may need units corrected."
      puts $fid "\t * Some assignments may need to be deleted or checked for"
      puts $fid "\t * physical niceness. Replace with good physics if possible."
      puts $fid "\t *)"
    }
  }
  foreach child $gmvect(oldwillbevar) {
    generate_BoundS $fid $type $child
  }
  foreach child $gmvect(newwillbevar) {
    generate_BoundS $fid $type $child
  }

  generate_run $fid $type "" "" bound_self
  generate_footer $fid $type bound_all
}

# call bound_self on IS_A'd parts, rebound IS_A'd vars
# call old boundself.
# whither boundwidth?
proc generate_bound_self {fid type} {
  global gmvect

  generate_header $fid $type bound_self
  if {[llength $gmvect(oldisavar)] || [llength $gmvect(oldisa)]} {
    generate_run $fid $type $gmvect(ancestor) :: bound_self
  }
  if {$gmvect(comment)} {
    puts $fid \
      "\t(* Array parts and variables need subscripts and FOR/DO loops."
    puts $fid \
      "\t * Generated code assumes boundwidth IS_A bound_width; in $type."
    puts $fid \
      "\t *)"
  }

  foreach child $gmvect(newisa) {
    generate_run $fid $type $child . bound_self
  }

  if {[llength $gmvect(newisavar)] && $gmvect(comment)} {
    puts $fid "\t(* Bound assignments may need units corrected."
    puts $fid "\t * Some assignments may need to be deleted or checked for"
    puts $fid "\t * physical niceness. Replace with good physics if possible."
    puts $fid "\t *)"
  }
  foreach child $gmvect(newisavar) {
    generate_BoundS $fid $type $child
  }

  generate_footer $fid $type bound_self
}

# call scale_all on passed in parts, rescale passed in vars, call scale_self.
proc generate_default_all {fid type} {
  global gmvect

  generate_header $fid $type default_all
  if {$gmvect(comment)} {
    puts $fid \
      "\t(* Array parts and variables need subscripts, perhaps FOR/DO loops. *)"
  }

  foreach child $gmvect(oldwillbe) {
    generate_run $fid $type $child . default_all
  }
  foreach child $gmvect(newwillbe) {
    generate_run $fid $type $child . default_all
  }


  if {[llength $gmvect(oldwillbevar)] || [llength $gmvect(newwillbevar)]} {
    if {$gmvect(comment)} {
      puts $fid "\t(* Default assignments need to be corrected or deleted."
      puts $fid "\t * Assignments to variables are incomplete." 
      puts $fid "\t *)"
    }
  }
  foreach child $gmvect(oldwillbevar) {
    generate_DefaulT $fid $type $child
  }
  foreach child $gmvect(newwillbevar) {
    generate_DefaulT $fid $type $child
  }

  generate_run $fid $type "" "" default_self
  generate_footer $fid $type default_all
}

proc generate_default_self {fid type} {
  global gmvect

  generate_header $fid $type default_self
  if {[llength $gmvect(oldisavar)] || [llength $gmvect(oldisa)]} {
    generate_run $fid $type $gmvect(ancestor) :: default_self
  }

  if {$gmvect(comment)} {
    puts $fid \
     "\t(* Array parts and variables need subscripts, perhaps FOR/DO loops. *)"
  }
  foreach child $gmvect(newisa) {
    generate_run $fid $type $child . default_self
  }

  if {[llength $gmvect(newisavar)] && $gmvect(comment)} {
    puts $fid "\t(* Default assignments need to be corrected or deleted."
    puts $fid "\t * Assignments to variables are incomplete." 
    puts $fid "\t *)"
  }
  foreach child $gmvect(newisavar) {
    generate_DefaulT $fid $type $child
  }

  generate_footer $fid $type default_self
}

# This method is hell. Runs reset for locally defined,
# unpassed parts and first found member of
# ascLibrVect(dofmethods) for parameters. reset should
# not be among dofmethods as we assume it.
proc generate_specify {fid type} {
  global gmvect ascLibrVect

  generate_header $fid $type specify
  if {$gmvect(comment)} {
    puts $fid "\t(* Boolean, integer, symbol, and set variables"
    puts $fid "\t * controlling WHENs should be assigned here."
    puts $fid "\t * Some variables may need .fixed := TRUE assigned."
    puts $fid "\t *)"
  }
  set clist {}
  foreach child $gmvect(oldwillbe) {
    lappend clist $child
  }
  foreach child $gmvect(newwillbe) {
    lappend clist $child
  }
  foreach child $clist {
    set m [generate_dofname $type $child $ascLibrVect(dofmethods)]
    if {[string compare $m ""]} {
      generate_run $fid $type $child . $m
    }
  }
  set clist {}
  foreach child $gmvect(oldisa) {
    lappend clist $child
  }
  foreach child $gmvect(newisa) {
    lappend clist $child
  }
  foreach child $clist {
    set m [generate_dofname $type $child $ascLibrVect(dofmethods)]
    if {[string compare $m ""]} {
      generate_run $fid $type $child . $m
    }
  }
  generate_footer $fid $type specify
}


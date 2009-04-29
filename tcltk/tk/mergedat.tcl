#  mergedat.tcl: merge data file utilities for mashing integrator output
#  by Benjamin Allan
#  Created March 29, 1998
#  Part of ASCEND
#  Revision: $Revision: 1.6 $
#  Last modified on: $Date: 1998/06/18 15:55:30 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: mergedat.tcl,v $
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

# merge data file utility functions for mashing
# integrator output .dat files into a single file
# or into a matlab format file.
#
# Requires the ascend extension stringcompact

# Usage:
# asc_merge_data_files <format> <output file name> <infile1> [infile2...]
# Merges the infiles given (as many as you like) into a new outputfile
# formatted for matlab, excel, or ascend.
#
# If the output file named already exists, it is overwritten.
#
# format is one of "ascend", "excel", "matlab".
#
# For ascend format,
# any time a set of headers matches the previous set of headers,
# it is eliminated. A new header will start a new data set.
#
# For excel format,
# duplicate headers are eliminated, and ascend headers are converted
# to a format that spreadsheet users find more congenial for import.
#
# For matlab format,
# duplicate headers are eliminated, but if a new set of headers
# is encountered, the conversion stops. outfile name should be
# something ending in .m.
#
# With the exception of headers, input is read/written line at a time,
# so if an error is encountered, the output may be partial.


proc mdf_check_output {file} {
  # verifies that output is writable, or errs, and defines
  # the matlab data matrix name.
  global ascmdfVect
  if {[file isdirectory $file] || \
      ([file exists $file] && ![file writable $file]) } {
    error "asc_merge_data_files called with bad output $file"
  }
  set ascmdfVect(outname) $file
  set ascmdfVect(matname) [file rootname [file tail $file]]
}

proc mdf_check_inputs {flist} {
  # verifies that input is readable text, or errs
  foreach file $flist {
    if {![file readable $file] || ![file isfile $file]} {
      error "asc_merge_data_files called with bad input $file"
    } 
  }
}

proc asc_merge_data_files {format outfile args} {
  # merges the files named in args into outfile, formatted according
  # to $format. currently 'ascend' and 'matlab' are understood.
  # For ascend format,
  # any time a set of headers matches the previous set of headers,
  # it is eliminated. A new header will start a new data set.
  # For matlab format,
  # duplicate headers are eliminated, but if a new set of headers
  # is encountered, the conversion stops. outfile name should be
  # something .m.
  #
  # With the exception of headers, input is handled line at a time,
  # so if an error is encountered, the output may be partial.
  global ascmdfVect
  if {[llength $args]==0} {
    error "asc_merge_data_files called without input files"
  }
  catch {unset ascmdfVect}
  mdf_check_output $outfile
  mdf_check_inputs $args
  # preprocessing
  set format [string tolower $format]
  switch $format {
  ascend -
  excel { # do nothing
    }
  matlab {
      set ascmdfVect(matlabheaderdone) 0
    }
  default {
      error \
        "asc_merge_data_files format output_file input_file \[input_file...\] where format is 'ascend','excel', or 'matlab' and input files are ascend .dat output files."
    }
  }
  # process input files
  set ofid [open $outfile "w+"]
  foreach file $args {
    set ifid [open $file "r+"]
    switch $format {
    ascend {
        set error [catch {mdf_ascend2ascend $ifid $ofid} errmsg]
      }
    excel {
        set error [catch {mdf_ascend2excel $ifid $ofid} errmsg]
      }
    matlab {
        set error [catch {mdf_ascend2matlab $ifid $ofid} errmsg]
      }
    }
    close $ifid
    if {$error} {break}
  }
  if {!$error} {
    # post processing
    switch $format {
    ascend -
    excel {
        # do nothing
      }
    matlab {
        puts $ofid "\];"
        puts $ofid "echo on"
        set column 1
        foreach i $ascmdfVect(oldvars) {
          set aname [lindex $i 1]
          # Handle the multisubscript arrays, converting each join to a .
          # the combinations here manage mixed integer/symbol subscripts
          # order here matters, else get redundant _ in output
          regsub -all -- {'\]\['} $aname . m1
          regsub -all -- {\]\['} $m1 . m2
          regsub -all -- {'\]\[} $m2 . m3
          regsub -all -- {\['} $m3 . m4
          regsub -all -- {\[} $m4 . m5
          regsub -all -- {'\]\.} $m5 . m6
          regsub -all -- {\]\.} $m6 . m7
          regsub -all -- {'\]} $m7 {} m8
          regsub -all -- {\]} $m8 {} m9
          regsub -all -- {\.} $m9 _ m10
          puts $ofid "$m10 = $ascmdfVect(matname)var(:,$column);"
          incr column
        }
        puts $ofid "echo off"
      }
    }
  }
  close $ofid
  if {$error} {
    error $errmsg
  }
}

proc mdf_ascend2ascend {infid outfid} {
  # internal function to strip extra headers from ascend dat files
  global ascmdfVect
  set state error
  if {![info exists ascmdfVect(oldvars)]} {
    set ascmdfVect(oldvars) "{ERROR reading input} {}"
  }
  set ok 1
  while {$ok} {
    set cc [gets $infid data]
    if {$cc<0} {break}
    set line [stringcompact $data]
    if {[string length $line] <= 0} {
      continue;
    }
    set c [lindex $line 0]
    switch -exact -- $c {
    DATASET {
        set ascmdfVect(newline1) $data; # keep in case next set is new 
        set state header
      }
    Observations: -
    States: {
        set ascmdfVect(newline2) $data
        set ascmdfVect(newvars) {}
      }
    default {
        switch -- [string index $line 0] {
        \{ {
            lappend ascmdfVect(newvars) $data
          }
        default {
            switch $state {
            error {
                error "DATASET not seen as expected in digestion"
              }
            header {
                set state coltitle
                set ascmdfVect(newtitle) $data
              }
            coltitle {
                set state rowminus
                set ascmdfVect(newminus) $data
              }
            rowminus {
                set state more
                if {[string compare \
                      $ascmdfVect(newvars) $ascmdfVect(oldvars)] != 0} {
                  set ascmdfVect(oldvars) $ascmdfVect(newvars)
                  puts $outfid $ascmdfVect(newline1)
                  puts $outfid $ascmdfVect(newline2)
                  foreach vdef $ascmdfVect(newvars) {
                    puts $outfid $vdef
                  }
                  puts $outfid $ascmdfVect(newtitle)
                  puts $outfid $ascmdfVect(newminus)
                }
              }
            more {
                puts $outfid $data
              }
            default {
                error "Unexpected line:\n$data\n in input file."
              }
            }
          }
        }
      }
    }
  }
}

proc mdf_ascend2excel {infid outfid} {
  # internal function to strip extra headers from ascend dat files
  # turns the var definitions on their sides as column titles in a row
  # after putting out the row of units. drops the dataset headers.
  global ascmdfVect
  set state error
  if {![info exists ascmdfVect(oldvars)]} {
    set ascmdfVect(oldvars) "{ERROR reading input} {}"
  }
  set ok 1
  while {$ok} {
    set cc [gets $infid data]
    if {$cc<0} {break}
    set line [stringcompact $data]
    if {[string length $line] <= 0} {
      continue;
    }
    set c [lindex $line 0]
    switch -exact -- $c {
    DATASET {
        set ascmdfVect(newline1) $data; # keep in case next set is new 
        set state header
      }
    Observations: -
    States: {
        set ascmdfVect(newline2) $data
        set ascmdfVect(newvars) {}
      }
    default {
        switch -- [string index $line 0] {
        \{ {
            lappend ascmdfVect(newvars) $data
          }
        default {
            switch $state {
            error {
                error "DATASET not seen as expected in digestion"
              }
            header {
                set state coltitle
                set ascmdfVect(newtitle) $data
              }
            coltitle {
                set state rowminus
                set ascmdfVect(newminus) $data
              }
            rowminus {
                set state more
                if {[string compare \
                      $ascmdfVect(newvars) $ascmdfVect(oldvars)] != 0} {
                  set ascmdfVect(oldvars) $ascmdfVect(newvars)
                  # puts $outfid $ascmdfVect(newline1)
                  # puts $outfid $ascmdfVect(newline2)
                  # write a line of units
                  foreach vdef $ascmdfVect(newvars) {
                    puts -nonewline $outfid "\t[lindex $vdef 2]"
                  }
                  puts $outfid ""
                  # write a line of names
                  foreach vdef $ascmdfVect(newvars) {
                    puts -nonewline $outfid "\t[lindex $vdef 1]"
                  }
                  puts $outfid ""
                  # puts $outfid $ascmdfVect(newtitle)
                  # puts $outfid $ascmdfVect(newminus)
                }
              }
            more {
                puts $outfid $data
              }
            default {
                error "Unexpected line:\n$data\n in input file."
              }
            }
          }
        }
      }
    }
  }
}

proc mdf_ascend2matlab {infid outfid} {
  # internal function to strip extra headers from ascend dat files
  # and generate a single matlab matrix
  global ascmdfVect
  set state error
  if {![info exists ascmdfVect(oldvars)]} {
    set ascmdfVect(oldvars) "{ERROR reading input} {}"
  }
  set ok 1
  while {$ok} {
    set cc [gets $infid data]
    if {$cc<0} {break}
    set line [stringcompact $data]
    if {[string length $line] <= 0} {
      continue;
    }
    set c [lindex $line 0]
    switch -exact -- $c {
    DATASET {
        set ascmdfVect(newline1) $data; # keep in case next set is new 
        set state header
      }
    Observations: -
    States: {
        set ascmdfVect(newline2) $data
        set ascmdfVect(newvars) {}
      }
    default {
        switch -- [string index $line 0] {
        \{ {
            lappend ascmdfVect(newvars) $data
          }
        default {
            switch $state {
            error {
                error "DATASET not seen as expected in digestion"
              }
            header {
                set state coltitle
                set ascmdfVect(newtitle) $data
              }
            coltitle {
                set state rowminus
                set ascmdfVect(newminus) $data
              }
            rowminus {
                set state more
                if {[string compare \
                      $ascmdfVect(newvars) $ascmdfVect(oldvars)] != 0} {
                  if {$ascmdfVect(matlabheaderdone)} {
                    error "Cannot merge unlike data sets into a matlab file"
                  }
                  set ascmdfVect(oldvars) $ascmdfVect(newvars)
                  puts $outfid "%%$ascmdfVect(newline1)"
                  puts $outfid "%%$ascmdfVect(newline2)"
                  foreach vdef $ascmdfVect(newvars) {
                    puts $outfid "%%$vdef"
                  }
                  puts $outfid "%%$ascmdfVect(newtitle)"
                  puts $outfid "%%$ascmdfVect(newminus)"
                  puts $outfid "echo off"
                  puts $outfid $ascmdfVect(matname)var=\[
                  set ascmdfVect(matlabheaderdone) 1
                } else {
                  puts stderr "DUPLICATE HEADER"
                }
              }
            more {
                puts $outfid $data
              }
            default {
                error "Unexpected line:\n$data\n in input file."
              }
            }
          }
        }
      }
    }
  }
}

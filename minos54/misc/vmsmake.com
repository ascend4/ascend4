$! This is vmsmake.com, a "make" file for Fortran on DEC VAX/VMS systems.
$! Comments at end.
$
$	On control_Y then goto CONTRL_Y
$	Set noon
$       error   = 0
$
$! set fortran option by seeing if directory is of the form [...DBG]
$	defdir  = f$directory()
$	foption = ""
$	loption = """/nomap"""
$	if f$locate("DBG]",defdir) .ge. f$length(defdir) then goto NAMEIT
$	foption = "/debug /nooptimize /check=(bounds,overflow)"
$       loption = """/debug /nomap"""
$
$ NAMEIT:
$       if P1 .eqs. "" then goto NOARG
$       MAKENAME == P1
$       goto OPENIT
$
$ NOARG:
$       if "''MAKENAME'" .eqs. "" then goto NOMAKENAME
$       
$ OPENIT:
$       write sys$output " Making ''MAKENAME'"
$ 	Open /read makelist [-]'MAKENAME'.mak
$       if .not. $status then goto FINISH
$
$ LOOP:
$	Read /end_of_file=LINKIT makelist fortfilename
$	fullname= f$parse(fortfilename,"[-]*.for")
$	If fullname .eqs. "" then goto NOSUBR
$	name    = f$parse(fullname,,,"name")
$	fordate = F$File_Attributes("''fullname'", "CDT")
$	objdate = F$File_Attributes("''name'.obj", "CDT")
$	If .not. $status then objdate = "1-JAN-1970"
$       fordate = f$cvtime(fordate)  ! convert to time for comparison
$       objdate = f$cvtime(objdate)
$       If fordate .lts. objdate then goto LOOP
$
$ COMPILEIT:
$ 	Write Sys$output " Compiling ''fullname'"
$ 	Fortran 'foption' 'fullname'
$ 	If .not. $status then goto FAIL
$ 	Purge 'name'.obj
$	Goto LOOP
$
$ NOSUBR:
$	Write Sys$output " No ''fortfilename' to compile!"
$	error = error + 1
$	Goto LOOP
$
$ LINKIT:
$       If error .gt. 0 then goto FAIL
$ 	Set message /sev /text
$	Write Sys$output " Linking ..."
$	@[-]'MAKENAME'.lnk 'loption'
$	if .not. $status then goto LINKERROR
$	Write Sys$output " Link finished"
$       dir /size=all /date *.exe
$	Goto DONE
$
$ LINKERROR:
$       Write Sys$output " Link warning or error.  Check if a new .EXE file exists"
$       Goto DONE
$
$ NOMAKENAME:
$       Write Sys$output " make needs an argument the first time you use it."
$       Write Sys$output " Say                make prog"
$       Write Sys$output " and later just     make"
$       Write Sys$output " where prog is the program you want to make"
$       goto FINISH
$
$ CONTRL_Y:
$	Write Sys$output " Aborted by Control_Y"
$
$ FAIL:
$	Write Sys$output " Link not done"
$
$ DONE:
$	Close makelist
$	Purge *.exe
$ FINISH:
$ 	Set message /nosev /text
$ 	Set on
$       Exit


	VMS Makefile
	============

This impression of "make" keeps track of Fortran files such as

   [mike.minos]mimain.for, mi1.for, mi2.for            (etc.)

and corresponding object and excecutable files

   [mike.minos.dbg]mimain.obj, mi1.obj, mi2.obj,  minos.exe   (debug code)
   [mike.minos.opt]mimain.obj, mi1.obj, mi2.obj,  minos.exe   (optimized code)

as described below.



1. [mike.minos]minos.mak   should contain a list of the Fortran filenames,
   one name per line:
	mimain
	mi1
	mi2
	 .
         .
   The filenames may contain more info, e.g.
   [mike.minos]mimain   or   $disk2:[fortran.lib]blas.for
   or as much as you want.

2. [mike.minos]minos.lnk   should contain appropriate DCL commands
   to link the object files (and possibly other things).  For example,

   $ link  'P1'  /exe=minos   -
       mi00main, mi05funs,    -    ! put problem-dependent routines here
       mi10vms ,                               -
       mi15blas, mi20amat, mi25bfac,           -
       mi30spec, mi35inpt, mi40bfil, mi50lp  , -
       mi60srch, mi65rmod, mi70nobj, mi80ncon, -
       [-]vmsc/opt

   Note the presence of 'P1'  (see Note 3 below).


3. This "make" file should have a home, e.g. [mike.util]vmsmake.com,
   and the corresponding command   make :== @[mike.util]vmsmake.com
   should be issued (e.g. in login.com).



For debug runs
--------------
Suppose you are working on a program called minos
whose source code (mimain.for, mi1.for, etc) is stored in [mike.minos].
Suppose your screen has two windows.
(If you have only one, treat Windows 1 and 2 as the same.)

   In Window 1,
        set def [mike.minos]            (go into the minos directory)
        edit mimain.for                 (edit some of the routines
             or whatever                 and save them as new .for files)

   In Window 2,
        set def [mike.minos.dbg]        (go into the debug directory)
        make minos                      (compile modified routines and link)
        run  minos                      (run new minos.exe)
   or   @minos                          (define files and then run)

   In Window 1,
        look at results and do some more editing if desired.

   In Window 2,
        make minos                      (as before)
   or just
        make
   since the name minos is recorded     (in the global symbol MAKENAME).


For optimized runs
------------------
   In Window 2,
        set def [mike.minos.opt]
   and proceed as before.



Notes:
-----
1. minos.mak says which Fortran files are to be checked.
   These are recompiled if they are newer than their .obj files,
   or if the .obj file does not exist.  (Ignore warning messages
   in the latter case.)

2. If some additional routines are to be checked,
   minos.mak and/or minos.lnk should be modified.

3. If you use make in the ".dbg" directory,

   (a) the Fortran files are compiled with the options
        /debug /nooptimize /check=(bounds,overflow)  ;
   (b) the link is performed with 'P1' = /debug .

   For any other directory, the default compiler and link options
   are used.

History:
 11 Jun 1989: Original version (Mike Saunders, Stanford University).
 11 Aug 1989: Irv Lustig's mods incorporated.  'P1' allows
              the lnk file to be in the root directory.
 19 Nov 1991: make and link names changed to mak and lnk to allow for
              copying via DOS (the lowest common denominator!).
 30 Jan 1992  Global symbol MAKENAME used to remember the program name.
              make prog      sets MAKENAME == prog.
              make           is sufficient thereafter.

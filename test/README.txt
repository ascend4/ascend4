
Test routines for libascend
===========================

This directory contains 'driver' code for running the test routines for 'core' ASCEND
code, a.k.a. libascend, including the compiler, the analysis module, expression
evaulation, etc.

The test code itself if located in 'test' subdirectories of ascend/compiler, etc:
close to the code being tested.

Test routines are linked against the CUnit unit testing library for C, available in most
Linux distros as well as from http://cunit.sourceforge.net.

SCons run from the parentdirectory will build the test program if and only if CUnit is
detected on your system and if you call for the 'test' target to be built, e.g.:

  scons test

There are flags that you can send to SCons to tell it where CUnit is installed on your
system, if you have installed it somewhere unusual.

-- 
John Pye
Apr 2009


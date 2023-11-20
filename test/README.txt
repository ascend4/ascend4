
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



Test status
===========

The following tests currently pass, if MALLOC_DEBUG=1:

general_color general_dstring general_hashpjw general_list general_listio \
general_mem general_pool general_pretty general_stack general_table \
general_tm_time general_ospath general_env general_ltmatrix general_ascMalloc \
utilities_ascDynaLoad utilities_ascEnvVar utilities_ascPrint \
utilities_ascSignal utilities_readln utilities_set linear_qrrank linear_mtx \
compiler_basics  compiler_expr 	compiler_fixfree  compiler_fixassign \
packages_defaultall solver_slv_common solver_slvreq	solver_ipopt solver_qrslv \
solver_fprops solver_lrslv integrator_lsode

(run them all by typing test/test following by the above text)

Of those, the following result in memory leaks, but still pass:
(probably associated with 'dlopen' and no unloading loaded libraries)

solver_slv_common solver_slvreq	solver_ipopt solver_qrslv solver_fprops solver_lrslv
integrator_lsode

There are problems with the following tests:

compiler_autodiff
 - errors are not within tolerance, need to check why

compiler_bintok
 - PASSES, even though it has failed to run bintok. Bintok feature is failing
   to report errors when switched on.

compiler_blackbox
 - should be returning sim==NULL but doesn't

solver_conopt 
 - didn't re-test recently due to missing consub.so

integrator_ida
 - there is an error with SUNDIALS 2.5.0

-- 
John Pye
Apr 2009
Feb 2017



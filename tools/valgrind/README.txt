This directory contains a 'suppressions' file for use when testing ASCEND using Valgrind. In particular, it suppresses a number on non-fixable errors relating to IPOPT and possibly Python.

The following is a snapshot of content from our wiki, see http://ascendwiki.cheme.cmu.edu/Valgrind for the latest version

---------------------------------------------------------------------
 
[http://valgrind.org/ Valgrind] is a tool that can detect memory leaks and invalid attempts to access memory, or perform actions that depend on the contents of uninitialised memory.

When running ASCEND through valgrind, the following approach is suggested:

<source lang=sh>
scons -j2 test
export ASCENDLIBRARY=~/ascend/models
export LD_LIBRARY_PATH=~/ascend:/usr/lib/coin:/usr/lib/coin/ThirdParty
valgrind --suppressions=tools/valgrind/suppressions --tool=memcheck --leak-check=full --show-reachable=yes test/test TESTNAME
</source>

where in the above, TESTNAME is replaced by a valid test case or test suite, use <tt>test/test -l</tt> and <tt>test/test -tSUITENAME</tt> to list available options.

The above approach requires that you have [http://cunit.sourceforge.net/ CUnit] installed on your system, and that you have configured the 'scons' flags correctly so that it is detected on your system.

[[Category:Development]]


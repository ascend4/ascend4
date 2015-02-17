#!/usr/bin/sh
#	ASCEND modelling environment
#	Copyright (C) 2006, 2007 Carnegie Mellon University
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2, or (at your option)
#	any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place - Suite 330,
#	Boston, MA 02111-1307, USA.

# This script is assumed to run with $PWD = ascend/compiler/test
# Authored by: Mahesh Narayanamurthi, August 2009

# Checking if the PWD is ascend/compiler/test
if echo $PWD | grep -q 'ascend/compiler/test'; then
 	echo "Guessed that we are under ASCEND ROOT Directory";
else 
	echo "NOTE: Please execute this command under ascend/compiler/test directory";
	exit 1;
fi;
# Going to ASCEND Root directory
cd ../../../
# Going to Models Directory
cd models/test/ipopt
# Checking if there are any new changes or additions to the Models list
if md5sum *.a4c | md5sum -c MD5Hash | grep -q 'OK'; then 
	echo "Guessed that there are no changes to the Model list. Continuing with compilation"
	# Unsetting ASC_YACAS_GEN if its already set, as those files have already been generated	
	unset ASC_YACAS_GEN
elif which yacas; then
	echo "Looks like there are changes to the Models list. Regenerating super-model file."
	md5sum *.a4c | md5sum > MD5Hash
	cd ../reverse_ad
	python modelgen.py
	sed -i '/^IMPORT /d' allmodels.a4c
	echo "Regenerating YACAS Files"
	export ASC_YACAS_GEN=1
fi;
# Moving upto ASCEND Root
cd ../../../
# Compiling
scons DEBUG=1 test
# Running compiler_autodiff test suite
echo "running test binary"
#strace -s 128 -f -F  -v -o ltout.txt ./test/test compiler_autodiff 
#ltrace -s 128 -f  -o ltout.txt ./test/test compiler_autodiff 
#valgrind -v --log-file=/tmp/vgout.txt ./test/test compiler_autodiff 
./test/test compiler_autodiff 2> LogTape.txt
echo "done running test binary"
if env|grep -q ASC_YACAS_GEN; then
	cd ascend/compiler/test
	#Assuming Yacas is installed
	# Generating all the symbolic derivatives
	if which yacas; then
		yacas Yacas1st.txt > YacasSymbolic1st.txt
		yacas Yacas2nd.txt > YacasSymbolic2nd.txt
	fi
	# plugging in the operating points and generating a yacas file
	python yacasgen.py
	# generating all the second derivative values
	if which yacas; then
		yacas YacasFirstInp.txt > YacasFirstDeriv.txt
		yacas YacasSecondInp.txt > YacasSecondDeriv.txt
	fi
	# Cleaning up the output
	sed '/^@ Relation/d' < YacasFirstDeriv.txt |sed '/^Quitting/d' > FirstDeriv.txt
	sed '/^@ Relation/d' < YacasSecondDeriv.txt |sed '/^Quitting/d' > SecondDeriv.txt
fi






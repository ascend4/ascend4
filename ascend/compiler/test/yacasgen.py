#!/usr/bin/env python
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
#	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

# This script generates a YACAS file that includes all the symbolic derivs
# and operating points which is later parsed by YACAS

try:
	print "Generating files for input to Yacas"
	varsf = open('Vars.txt','r')
	yacasprepf1st = open('yacas-prep-1st.txt','r')
	yacasoutf1st = open('yacas-input-1st.txt','w')
	yacasprepf2nd = open('yacas-prep-2nd.txt','r')
	yacasoutf2nd = open('yacas-input-2nd.txt','w')

#ToStdout() [Echo(N( Eval(D(%s) %s),17))]

# ToStdout() [Echo(N( Eval(D(%s) D(%s) %s),17))];

	i=0 # counter for number of variables in each relation
	for line in varsf:
		if line.strip().startswith('@ Relation'):
			print "i=%d: %s"%(i,line.strip())	
			j = (i)*(i) # i^2-- there will be this many second derivatives
			yacasoutf2nd.write('ToStdout() [Echo("' + line.strip() + '");];\n')
			while j>0:
				yacasoutf2nd.write("ToStdout() [Echo({N(Eval(%s),17)});];\n"%(yacasprepf2nd.readline().strip(),))
				j = j-1

			yacasoutf1st.write('ToStdout() [Echo("' + line.strip() + '");];\n')
			while i>0:
				yacasoutf1st.write("ToStdout() [Echo({N(Eval(%s),17)});];\n"%(yacasprepf1st.readline().strip(),))
				i = i-1
			# back to i=0 again
		else:
			# Vars.txt contains a variable value declaration
			i = i+1 # increase the number of variables
			yacasoutf2nd.write(line.strip()+';\n')
			yacasoutf1st.write(line.strip()+';\n')
	varsf.close()
	yacasprepf2nd.close()
	yacasoutf2nd.close()
	yacasprepf1st.close()
	yacasoutf1st.close()
except IOError, e:
    print "Error managing streams" ,e

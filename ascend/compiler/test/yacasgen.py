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
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place - Suite 330,
#	Boston, MA 02111-1307, USA.

# This script generates a YACAS file that includes all the symbolic derivs
# and operating points which is later parsed by YACAS

try:
    print "Generating files for input to Yacas"
    varsf = open('Vars.txt','r')
    yacasinf1st = open('YacasSymbolic1st.txt','r')
    yacasoutf1st = open('YacasFirstInp.txt','w')
    yacasinf2nd = open('YacasSymbolic2nd.txt','r')
    yacasoutf2nd = open('YacasSecondInp.txt','w')

    i=0
    for line in varsf:
        if line.strip().startswith('@ Relation'):
            j = (i)*(i)
            yacasoutf2nd.write('ToStdout() [Echo("' + line.strip() + '");];\n')
            while j>0:
                yacasoutf2nd.write('ToStdout() [Echo(N(' + yacasinf2nd.readline().strip() + ',17));];\n')
                j = j-1

            yacasoutf1st.write('ToStdout() [Echo("' + line.strip() + '");];\n')
            while i>0:
                yacasoutf1st.write('ToStdout() [Echo(N(' + yacasinf1st.readline().strip() + ',17));];\n')
                i = i-1

        else:
	    i = i+1
            yacasoutf2nd.write(line.strip()+';\n')
            yacasoutf1st.write(line.strip()+';\n')
    varsf.close()
    yacasinf2nd.close()
    yacasoutf2nd.close()
    yacasinf1st.close()
    yacasoutf1st.close()
except IOError, e:
    print "Error managing streams" ,e

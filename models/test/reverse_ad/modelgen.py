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
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.


# This script generates a model file that includes all the models under
# models/test/ipopt which is later used for second derivative calculations

import os, sys, glob

dirname =  '../ipopt/'
files = glob.glob(dirname+'*.a4c')

try:

    #Clearing already existing file
    fout = open('allmodels.a4c','w')
    fout.close()
    #Creating new model file
    fout = open('allmodels.a4c','a')
    for modelfile in sorted(files):
        fin = open(modelfile,'r')
        contents = fin.read()
        fout.write(contents+'\r\n')
        fin.close()
    fout.write('MODEL allmodels;\r\n')
    i=1
    for modelfile in sorted(files):
        (filepath,filewithext) =  os.path.split(modelfile)
        (filename,ext) = os.path.splitext(filewithext)
        fout.write('\tm'+str(i)+' IS_A '+ filename + ';\r\n')
        i=i+1
    fout.write('METHODS\r\n')
    fout.write('METHOD on_load;\r\n')
    i=1
    for modelfile in sorted(files):
        fout.write('\tRUN m'+str(i)+'.on_load;\r\n')
        i=i+1
    fout.write('END on_load;\r\nEND allmodels;\r\n')    
    fout.close()
except IOError:
    print "Error Creating Super-Model file"
    

    

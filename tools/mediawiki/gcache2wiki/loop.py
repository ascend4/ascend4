#!/usr/bin/env python
# loop through files in the nominated folder 

from striphf import *
import os,sys

excl = ["MISSING","README","README.txt","wiki",".svn",".cvs"]
sourcedir = os.path.expanduser("~/wikiscrape")
targetdir = os.path.expanduser("~/wikiscrape/wiki")
wikiname = "ASCEND"

if not os.path.exists(targetdir):
	print "Target directory %s does not exist"%targetdir
	sys.exit(1)

res = {}
for f in os.listdir(sourcedir):
	sys.stderr.write("Processing %s...\n"%f)
	if f in excl:
		continue
	c = open(os.path.join(sourcedir,f)).read()
	s,pagename = html2wiki(c,wikiname)
	t = os.path.join(targetdir,f + ".txt")
	sys.stderr.write("Writing to %s\n"%t)
	res[pagename] = (t,"NEW")
	open(t,"w").write(s)

import pickle
pickle.dump(res,open("reslist.pickle","w"))



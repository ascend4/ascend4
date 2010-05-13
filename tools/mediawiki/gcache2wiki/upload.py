#!/usr/bin/env python
# This script will do the hard work of uploading the content to our wiki.
import sys
import urllib2
import os.path
import pickle

res = pickle.load(open("reslist.pickle"))

wiki = "http://ascend.cheme.cmu.edu/"

# first create a big huge textfile containing the pages

if 0:
	pagestart = "\nPAGESTARTxkljhdakljfdfsljhkjhdsgnsdgnweisdwgwrehjgsjfs\n"
	pageend =   "\nPAGEENDslkfjsldkfjskldfjskljfsghwuewrweorswnmbrwewemwr\n"
	titlestart = "TITLESTART>>>"
	titleend =   "<<<TITLEEND\n"
	print "WRITING PAGES TO BIG TEXT FILE"
	f = open('bigpage.txt','w')
	for name in res:
		filename,status = res[name]
		f.write(pagestart)
		f.write(titlestart + name + titleend)
		f.write(open(filename).read())
		f.write(pageend)
	f.close()

	print "PAGES WRITTEN"

sys.path.append(os.path.expanduser("~/pywikipedia"))
import pagefromfile
import codecs

print "UPLOADING TO WIKI"

errorfile = None
# TODO: make config variables for these.
filename = "bigpage.txt"

include = False
force = True
append = None
notitle = True
summary = "Restored page from Google Cache, uploaded by John Pye"
minor = False
autosummary = False
dry = False

bot = pagefromfile.PageFromFileRobot(None, force, append, summary, minor, autosummary, dry)

try:
	for name in res:
		filename,status = res[name]
		if status !="SAVED" and status !="ERROR":
			content = codecs.open(filename,'r','utf-8').read()
			errrorfile = name
			try:
				bot.put(name,content)
				errorfile = None
				res[name] = (filename,"SAVED")
			except Exception,e:
				print "ERROR in uploading:",e
				if errorfile:
					filename,status=res[errorfile]
					res[errorfile]=(filename,"ERROR")

except Exception,e:
	print "ERROR:",str(e)
except KeyboardInterrupt:
	print "INTERRUPTED"

pickle.dump(res,open("reslist.pickle","w"))
print "STATUS FILE UPDATED"


#!/usr/bin/env python
# This script will do the hard work of uploading the content to our wiki.
import sys

user = "Jpye"
password = sys.argv[1]

import urllib2
from zope.testbrowser.browser import Browser
import pickle

res = pickle.load(open("reslist.pickle"))

wiki = "http://ascend.cheme.cmu.edu/"

print "LOGGING IN TO WIKI"
b = Browser(wiki)
login = b.getLink("Log in / create account")
login.click()
print "AT LOGIN PAGE:",b.url
userinput = b.getControl(name="wpName")
userinput.value = user
passwordinput = b.getControl(name="wpPassword")
passwordinput.value = password
submitbutton = b.getControl(name="wpLoginattempt")
submitbutton.click()
print "LOGGED IN TO:",b.url
b.handleErrors = True

editpage = wiki + '/index.php?title=%s&action=edit'

try:
	for name in res:
		filename,status = res[name]
		if status!="SAVED":
			print "Creating page '%s'..."%name
			try:
				url = editpage%name
				print "OPENING:",url
				b.open(url)
			except urllib2.HTTPError,e:
				print "ERROR CODE:",e.code
				print "ERROR:",str(e)
				print "ERROR OPENING",wiki+name
				print "CONTENT =",b.content
				raise RuntimeError("Quitting, failed to load page")
			print 
			if "There is currently no text in this page." in b.content:
				print "CREATING PAGE AT",b.url
			else:
				raise RuntimeError("Page '%s' already exists"%name)
			#res[name] = filename,"SAVED"
		else:
			print "Skipping already-saved '%s'" % name
except Exception,e:
	print "ERROR:",str(e)
	print "Saving updated status to pickle..."
	pickle.dump(res,open("reslist.pickle","w"))

print "DONE"



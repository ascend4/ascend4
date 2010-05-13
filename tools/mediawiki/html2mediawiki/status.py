#!/usr/bin/env python
print "LOADING PICKLE"
import pickle
res = pickle.load(open("reslist.pickle"))

print "STATUS CONTENT:"

for name in res:
	filename,status = res[name]
	if status == "SAVED":
		print "SAVED:",name

for name in res:
	filename,status = res[name]
	if status != "SAVED":
		print "%s: %s" % (status,name)


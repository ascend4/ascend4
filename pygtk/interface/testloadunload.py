import os, os.path, re
import threading, heapq
import time, platform

import sys, dl
# This sets the flags for dlopen used by python so that the symbols in the
# ascend library are made available to libraries dlopened within ASCEND:
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)
import ascend

if os.getenv('ASCENDLIBRARY')==None:
	path = '~/src/ascend/trunk/models'
	print "Setting ASCEND path to",path
	os.putenv('ASCENDLIBRARY',path)

L = ascend.Library();
L.load(filepath);
for M in L.getModules():
	print "Looking at module '"+M.getName()+"'"
	for t in L.getModuleTypes(M):
		#print "Looking at type '"+str(t.getName())+"'"
		for m in t.getMethods():
			print "Looking at method '"+str(m.getName())+"'"
			if m.getName()==TEST_METHOD_NAME:
				jobslock.acquire()
				heapq.heappush(jobs, (0, AscendTestModel( self.filepath,M.getName())) )
				jobslock.release()
del(L)
ascendlock.release()		


import os, os.path, mmap, re
import threading, heapq
import time, platform

import sys, dl
# This sets the flags for dlopen used by python so that the symbols in the
# ascend library are made available to libraries dlopened within ASCEND:
sys.setdlopenflags(dl.RTLD_GLOBAL|dl.RTLD_NOW)
import ascend

# Our input will be the ASCENDLIBRARY environment variable
# plus perhaps some commandline options relating to exclusions and inclusions.
# This program needs to search files in the given directory and hunt for
# A4C and A4L files that contain MODELs that contain 'METHOD self_test'
# methods. We then create instances of any such models and run these
# methods. The methods should include commands which we can trap via
# some kind of callback, reporting failure or success.

TEST_METHOD_NAME='self_test';
METHOD_RE = '\\s*METHOD\\s+'+TEST_METHOD_NAME+'\\s*;';
METHOD_REGEXP = re.compile(METHOD_RE);

#---------------------------------------------------------------------
# TASK CLASSES

# These classes represent tasks that will be added to the queue. They
# just need to have __init__ and run methods, they will be queued and
# run by the thread manager.


# Scan a directory, store any a4c or a4l files in the queue,
# also store and subdirectories in the queue, they will be
# recursed into.
class AscendScanDirectory:
	global jobs, jobslock
	def __init__(self,path):
		self.path=path
	
	def run(self):
		print "Scanning",self.path,"..."		
		for f in os.listdir(self.path):
			if f==".svn" or f=="CVS" or f=="westerberg":
				continue
			if os.path.isdir( os.path.join(self.path,f) ):
				jobslock.acquire()
				heapq.heappush( jobs,( 30, AscendScanDirectory(os.path.join(self.path,f)) ) )
				jobslock.release()
			else:
				stem,ext = os.path.splitext(f)
				if ext==".a4c" or ext==".a4l":
					jobslock.acquire()
					heapq.heappush( jobs,( 10, AscendInspectFile(os.path.join(self.path,f)) ) )
					jobslock.release()

# Ascend file inspection task: load the file and check for 'METHOD self_test'
# somewhere in the file.


class AscendInspectFile:
	global jobs,jobslock;
	global inspectlock;
	
	def __init__(self,filepath):
		self.filepath = filepath;
	
	def run(self):
		inspectlock.acquire()
		try:
			(path,file) = os.path.split(self.filepath)
			#print "Checking",self.filepath,"for '"+TEST_METHOD_NAME+"' method"
			size = os.path.getsize(self.filepath)
			#print "File size is",size
			if size > 0:
				f = open(self.filepath,'r')
				s = mmap.mmap(f.fileno(),size,mmap.MAP_SHARED,mmap.PROT_READ)
				
				if METHOD_REGEXP.search(s):
					print "Found 'METHOD "+TEST_METHOD_NAME+"' in",file
					jobslock.acquire()
					heapq.heappush(jobs, (5, AscendTestFile(self.filepath)) )
					jobslock.release()
				else:
					pass #print "File",file," is not self-testing"
				f.close()
		except IOError:
			print "IOError"
		inspectlock.release()


# This 'task class' will use ASCEND to load the specified model
# then some sub-task will read off the list of models and methods
# and for each model that includes self_test, will run that
# method and check the result.
class AscendTestFile:
	global jobs, jobslock, ascendlock, library;
	
	def __init__(self,filepath):
		self.filepath = filepath
	
	def run(self):
		ascendlock.acquire()
		L = ascend.Library()
		L.clear()
		L.load(self.filepath)
		for M in L.getModules():
			print "Looking at module '"+M.getName()+"'"
			for t in L.getModuleTypes(M):
				#print "Looking at type '"+str(t.getName())+"'"
				for m in t.getMethods():
					#print "Looking at method '"+str(m.getName())+"'"
					if m.getName()==TEST_METHOD_NAME:
						jobslock.acquire()
						heapq.heappush(jobs, (0, AscendTestModel( self.filepath,t.getName().toString())) )
						jobslock.release()
		ascendlock.release()		

# Run self_test on a given model
class AscendTestModel:
	global jobs, jobslock;
	
	def __init__(self,filepath,modelname):
		self.filepath=filepath
		self.modelname=modelname

	def run(self):
		ascendlock.acquire()
		try:
			L = ascend.Library()
			L.clear()
			L.load(self.filepath)
			t = L.findType(self.modelname)
			testmethod = None
			for m in t.getMethods():
				if m.getName()==TEST_METHOD_NAME:
					testmethod = m;
			if not testmethod:
				raise RuntimeError("No method '"+TEST_METHOD_NAME+"' found")

			s = t.getSimulation('testsim');
			s.check()
			print "LAUNCHING SOLVER...\n\n"
			s.solve(ascend.Solver(0))
			s.run(testmethod)
		except RuntimeError, e:
			print e

		ascendlock.release()


#---------------------------------------------------------------
# Thread runner	
		
class AscendModelLibraryTester:
	def __init__(self):
		global jobs, jobslock, inspectlock, ascendlock

		jobs = []
		
		self.runlock = threading.Lock()
		jobslock = threading.Lock()
		inspectlock = threading.Lock()
		ascendlock = threading.Lock()
		
		self.runlock.acquire()
		self.path = os.environ['ASCENDLIBRARY']
		self.threads = []
		
		if platform.system()=='Windows':
			self.separator=";"
		else:
			self.separator=":"

		print 'Search path:',self.path
		for p in self.path.split(self.separator):
			j = AscendScanDirectory(p) 
			jobslock.acquire()
			heapq.heappush(jobs, (20,j) )
			jobslock.release()

	def run(self,num_threads):
		for i in range(num_threads):
			t = AscendModelLibraryTester.DoWork(i) # new thread
			self.threads.append(t)
			t.start()
		
		for i in range(num_threads):
			self.threads[i].join()
			
		self.runlock.release()
	
	class DoWork(threading.Thread): # thread number tn
		global jobs, jobslock, max_threads
		moreThreads = threading.Event()
		lock = threading.Lock()

		def __init__(self,tn):
			threading.Thread.__init__(self)
			self.threadnum = tn

		def run(self):
			while 1:
				
				jobslock.acquire()
				try:
					index, job = heapq.heappop(jobs)
				except IndexError:
					jobslock.release()
					print "No jobs left for thread",self.threadnum
					return					
				jobslock.release()

				job.run()

# The following little extra bit means that this file can be automatically
# be used as part of a larger unittesting process. So if there are
# CUnit tests, and they can be executed somehow via Python, then we
# test everything in ASCEND at once.

import unittest

class TestAscendModelLibrary(unittest.TestCase):
		
	def testAllModelsInPath(self):
		t = AscendModelLibraryTester();
		t.run(5); # five threads
		print 'Waiting for ASCEND Model Library Tester to complete...'
		t.runlock.acquire()

if __name__=='__main__':
	unittest.main()

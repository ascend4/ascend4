'''Import the SWIG wrapper'''
import os
DEFAULT_CANVAS_MODEL_LIBRARY_FOLDER = os.path.join('..','..','models','test','canvas')

try:
	import ascpy
except ImportError as e:
	print "Error: Could not load ASCEND Library. Please check the paths \
	ASECNDLIBRARY and LD_LIBRARY_PATH\n",e

from blocktype import BlockType
from blockstream import BlockStream

class ascPy(object):
	'''
	The ASCEND Library class. Everything that talks to ASCEND should be here.
	'''
	def __init__(self):
		self.library = None
		self.annodb = None
		self.modules = None	
		self.types = None
		self.canvas_blocks = []
		self.streams = []
		self.reporter = ascpy.getReporter()
		self.defaultlibraryfolder = DEFAULT_CANVAS_MODEL_LIBRARY_FOLDER
	
	def load_library(self,lib_name = None):
		if lib_name == None:
			return
		
		lib_path = os.path.join('test','canvas',lib_name)
		try:
			self.library.clear()
			self.library.load(lib_path)
		except Exception as e:
			self.library = ascpy.Library()
			self.library.load(lib_path)
			
		self.annodb = self.library.getAnnotationDatabase()
		self.modules = self.library.getModules()
		
		try:
			self.blocktypes = set()
			self.streamtypes = set()
			for m in self.modules:
				self.types = self.library.getModuleTypes(m)
				for t in self.types:
					#if t.hasParameters():
					#	continue
					self.parse_types(t)
					self.parse_streams(t)
		except Exception as e:
			print 'Error: ASCEND Blocks Could not be loaded \n',e
			exit()
		
		try:
			del self.canvas_blocks[:]
			for t in self.blocktypes:
				b = BlockType(t,self.annodb)
				self.canvas_blocks +=[b]
		except Exception as e:
			print 'Error: Could not load blocktypes \n',e
			exit()
		try:
			for stream in self.streamtypes:
				s = BlockStream(stream,self.annodb)
				self.streams +=[s]
				
		except Exception as e:
			print 'Error: Could not load streams \n',e
			exit()
			
		
		'''
		try:
			for stream in streamtypes:
				notes = self.annodb.getTypeRefinedNotesLang(stream,
				                                            ascpy.SymChar("inline"))
				for n in notes:
					types = str(n.getText()).split(',')
					self.streams.append((str(n.getId()),types))
		except Exception as e:
			print 'Error: Could not load streamtypes \n',e
			exit()
		'''
	def parse_types(self,t):
		x = self.annodb.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
		if x:
			self.blocktypes.add(t)
	
	def parse_streams(self,t):
		x = self.annodb.getNotes(t,ascpy.SymChar("stream"),ascpy.SymChar("SELF"))
		if x:
			self.streamtypes.add(t)

# vim: set ts=4 noet:

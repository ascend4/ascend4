'''Import the SWIG wrapper'''
import os
DEFAULT_CANVAS_MODEL_LIBRARY_FOLDER = os.path.join('..','..','models','test','canvas')

try:
	import ascpy
except ImportError as e:
	print "Error: Could not load ASCEND Library. Please check the paths ASECNDLIBRARY and LD_LIBRARY_PATH\n",e

from blocktype import BlockType

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
			blocktypes = set()
			for m in self.modules:
				self.types = self.library.getModuleTypes(m)
				for t in self.types:
					if t.hasParameters():
						continue
					x = self.annodb.getNotes(t,ascpy.SymChar("block"),ascpy.SymChar("SELF"))
					if x:
						blocktypes.add(t)
		except Exception as e:
			print 'Error: ASCEND Blocks Could not be loaded \n',e
		try:
			del self.canvas_blocks[:]
			for t in blocktypes:
				b = BlockType(t,self.annodb)
				self.canvas_blocks +=[b]
		except Exception as e:
			print 'Error: Could not load blocktypes \n',e
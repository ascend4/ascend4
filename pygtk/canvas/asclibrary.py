'''Import the SWIG wrapper'''
import os
import traceback
DEFAULT_CANVAS_MODEL_LIBRARY_FOLDER = os.path.join('..','..','models','test','canvas')

try:
	import ascpy
except ImportError as e:
	print("Error: Could not load ASCEND Library. Please check the paths \
	ASECNDLIBRARY and LD_LIBRARY_PATH\n",e)

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
			print("[canvas] load_library called with no library name")
			return
		
		lib_path = os.path.join('test','canvas',lib_name)
		print("[canvas] loading library %r via ASCEND path %r" % (lib_name, lib_path))
		print("[canvas] ASCENDLIBRARY=%r" % os.environ.get("ASCENDLIBRARY"))
		try:
			if self.library is not None:
				self.library.clear()
			self.library.load(lib_path)
		except Exception as e:
			print("[canvas] existing library load failed: %s: %s" % (type(e).__name__, e))
			print("[canvas] retrying with a fresh ASCEND Library")
			self.library = ascpy.Library()
			self.library.load(lib_path)
			
		self.annodb = self.library.getAnnotationDatabase()
		self.modules = self.library.getModules()
		module_list = list(self.modules)
		print("[canvas] loaded %d module(s)" % len(module_list))
		
		try:
			self.blocktypes = set()
			self.streamtypes = set()
			for m in module_list:
				self.types = self.library.getModuleTypes(m)
				for t in self.types:
					#if t.hasParameters():
					#	continue
					self.parse_types(t)
					self.parse_streams(t)
			print("[canvas] found %d canvas block type(s), %d stream type(s)" % (len(self.blocktypes), len(self.streamtypes)))
		except Exception as e:
			print('Error: ASCEND Blocks Could not be loaded \n',e)
			traceback.print_exc()
			exit()
		
		try:
			del self.canvas_blocks[:]
			for t in self.blocktypes:
				b = BlockType(t,self.annodb)
				self.canvas_blocks +=[b]
				print("[canvas] block type %s: inputs=%d outputs=%d params=%d icon=%r" % (
					b.type.getName(), len(b.inputs), len(b.outputs), len(b.params), b.iconfile
				))
			print("[canvas] constructed %d palette block object(s)" % len(self.canvas_blocks))
		except Exception as e:
			print('Error: Could not load blocktypes \n',e)
			traceback.print_exc()
			exit()
		try:
			del self.streams[:]
			for stream in self.streamtypes:
				s = BlockStream(stream,self.annodb)
				self.streams +=[s]
			print("[canvas] constructed %d stream object(s)" % len(self.streams))
				
		except Exception as e:
			print('Error: Could not load streams \n',e)
			traceback.print_exc()
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

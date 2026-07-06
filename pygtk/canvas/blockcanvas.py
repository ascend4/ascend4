from gaphas import Canvas
import re
from blockitem import DefaultBlockItem, GraphicalBlockItem
from blockline import BlockLine
from blockstream import BlockStream
from functools import reduce

UNITS_RE = re.compile(r"([-+]?(\d+(\.\d*)?|\d*\.\d+)([eE][-+]?\d+)?)\s*(.*)");

saved_model = None

class BlockCanvas(Canvas):
	
	def __init__(self):
		super(BlockCanvas, self).__init__()
		self.filestate = 0
		self.canvasmodelstate = 'Unsolved'
		self.model_library = ''
		self.saved_data = None
		self.filename = None
		self.user_code = ''

	def add(self, item, parent=None, index=None):
		super(BlockCanvas, self).add(item, parent, index)
		setup_constraints = getattr(item, "setup_canvas_constraints", None)
		if setup_constraints is not None:
			setup_constraints(self.connections)
	
	def update_constraints(self, items):
		"""
		Update constraints. Also variables may be marked as dirty before the
		constraint solver kicks in.
		"""
		# request solving of external constraints associated with dirty items
		request_resolve = getattr(self.solver, "request_resolve", None)
		if request_resolve is None:
			return self.connections.solve()
		for item in items:
			if hasattr(item,'ports'):
				for p in item._ports:
					if hasattr(p,"point"):
						request_resolve(p.point.x)
						request_resolve(p.point.y)

		self.connections.solve()

	def update_now(self, dirty_items=None):
		"""
		Perform a Gaphas model update.

		Gaphas 3 calls this method with the set of dirty items from GtkView.
		Legacy canvas code also calls it without arguments after loading a
		canvas, so keep that path as a full-canvas update.
		"""
		if dirty_items is None:
			dirty_items = tuple(self.get_all_items())
		super(BlockCanvas,self).update_now(dirty_items)
		normalized_items = []
		for item in dirty_items:
			normalize_origin = getattr(item, "normalize_origin", None)
			if normalize_origin is not None and normalize_origin():
				normalized_items.append(item)
		if normalized_items:
			super(BlockCanvas,self).update_now(normalized_items)

	def _obtain_cairo_context(self):
		import cairo
		surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 0, 0)
		return cairo.Context(surface)

	def get_connection(self, handle):
		return self.connections.get_connection(handle)

	def reattach_ascend(self, ascwrap, notesdb):
		"""
		After unpickling a canvas, this method gives a way of reattaching
		the model to an ASCEND library, by connecting the types named in the
		pickle with the typenames present in the Library.
		"""
		# FIXME need to set ASCEND to *load* the missing types if they're not
		# already in the Library.
		items = []
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			bi = item.blockinstance
			bi.reattach_ascend(ascwrap,notesdb)
			
			#if bi.blocktype.type is None:
			#	bi.blocktype.reattach_ascend(ascwrap, notesdb)
	'''		
	def set_stream(self,stream):
		items = self.get_all_items()
		for item in items:
			if type(item)==DefaultBlockItem:
				bi = item.blockinstance
				bi.stream=stream
	'''
	
	def __str__(self):
		"""
		Create ASCEND code corresponding to canvas model, return the
		code as a string.
		Note: This function uses Python's advanced string formatting capability.
		"""

		# FIXME it should be possible to perform this function at an
		# application layer, without using routines from Gaphas...
			
		string = '''
(* automatically generated model*)
REQUIRE "{lib_name}";

MODEL canvasmodel;
    {is_a}
    {streams}
    {are_the_same}
    {canvas_user_code} 
METHODS
METHOD canvas_user_code;
END canvas_user_code;
METHOD parameter_code;
    {parameter_code} 
END parameter_code;
METHOD on_load;
    RUN canvas_user_code;
    RUN parameter_code;
END on_load;
END canvasmodel;
'''
		replacement_fields = {'lib_name':str(self.model_library),'is_a':'','are_the_same':'',
		                      'canvas_user_code':'','parameter_code':'','block_user_code':''
		                      ,'streams':''}
	
		items = self.get_all_items()
	
		def parse(item):
			if type(item)==DefaultBlockItem or type(item)==GraphicalBlockItem:
				bi = item.blockinstance
				replacement_fields['is_a']+=str(bi)
				specify = [param for param in bi.params if bi.params[param].value != None]
				fix = [param for param in bi.params if bi.params[param].fix == True]
				specify = [x for x in specify if not (x in fix)]
				specify = ['\t{0}.{1}:={2};\n'.
				              format(bi.name,param,bi.params[param].value) for param in specify]
				fix = ['\tFIX {0}.{1};\n\t{0}.{1}:={2}{4}{3}{5};\n'.
				          format(bi.name,param,bi.params[param].value,
				                 bi.params[param].units,'{','}') for param in fix]
				try:
					replacement_fields['parameter_code']+=\
					                  reduce(lambda x,y:x+y,specify)
				except TypeError:
					pass
				try:
					replacement_fields['parameter_code']+=\
					                  reduce(lambda x,y:x+y,fix)
				except TypeError:
					pass

			if type(item)==BlockLine:
				replacement_fields['are_the_same']+=str(item.lineinstance)

		list(map(parse,items))
		
		replacement_fields['canvas_user_code'] = self.user_code
		return string.format(**replacement_fields)

	def __getstate__(self):
		"""
		Placeholder for any special pickling stuff that we want to do with
		our canvas.
		"""
		return super(BlockCanvas,self).__getstate__()
	
	def __setstate__(self, state):
		"""
		Placeholder for any special pickling stuff that we want to do with
		our canvas.
		"""
		super(BlockCanvas,self).__setstate__(state)

# vim: set ts=4 noet:

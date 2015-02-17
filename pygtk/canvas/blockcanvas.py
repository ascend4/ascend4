from gaphas import Canvas
from gaphas.item import Line
import re
from blockitem import DefaultBlockItem
from blockline import BlockLine
from blockstream import BlockStream

UNITS_RE = re.compile("([-+]?(\d+(\.\d*)?|\d*\.d+)([eE][-+]?\d+)?)\s*(.*)");

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
	
	def update_constraints(self, items):
		"""
		Update constraints. Also variables may be marked as dirty before the
		constraint solver kicks in.
		"""
		# request solving of external constraints associated with dirty items
		request_resolve = self._solver.request_resolve
		for item in items:
			if hasattr(item,'ports'):
				for p in item._ports:
					if hasattr(p,"point"):
						request_resolve(p.point.x)
						request_resolve(p.point.y)

		super(BlockCanvas,self).update_constraints(items)

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
			if type(item)==DefaultBlockItem:
				bi = item.blockinstance
				replacement_fields['is_a']+=str(bi)
				specify = filter(lambda param:bi.params[param].value != None,bi.params)
				fix = filter(lambda param:bi.params[param].fix == True,bi.params)
				specify = filter(lambda x:not (x in fix),specify)
				specify = map(lambda param:'\t{0}.{1}:={2};\n'.
				              format(bi.name,param,bi.params[param].value),specify)
				fix = map(lambda param:'\tFIX {0}.{1};\n\t{0}.{1}:={2}{4}{3}{5};\n'.
				          format(bi.name,param,bi.params[param].value,
				                 bi.params[param].units,'{','}'),fix)
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
		
			
		map(parse,items)
		
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
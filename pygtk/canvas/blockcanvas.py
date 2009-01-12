from gaphas import Canvas
from gaphas.item import Line

class BlockCanvas(Canvas):
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

	def reattach_ascend(self, library, notesdb):
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
			if bi.blocktype.type is None:
				bi.blocktype.reattach_ascend(library, notesdb)

	def __str__(self):
		"""
		Create ASCEND code corresponding to canvas model, return the
		code as a string.
		"""
		s = "\n(* automatically generated model from blocklist.py *)\n";
		s += "MODEL canvasmodel;\n"
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			s += str(item.blockinstance)

		for line in self.get_all_items():
			if not isinstance(line, Line):
				continue
			s += str(line.lineinstance)
		s += "END canvasmodel;\n";
		return s

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


from gaphas import Canvas

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
		        for p in item.ports:
		            request_resolve(p.x)
		            request_resolve(p.y)

		super(BlockCanvas,self).update_constraints(items)

	def _normalize(self, items):
		"""
		Correct offset of ports due to movement of left-side handles.
		"""
		dirty_matrix_items = set()
		for item in items:
		    if not hasattr(item, 'ports'):
		        continue
		    handles = item.handles()
		    ports = item.ports
		    if not handles or not ports:
		        continue
		    x, y = map(float, handles[0].pos)
		    # Dirty marking is done by the superclass' method
		    if x:
		        for p in ports:
		            p.x._value -= x
		    if y:
		        for p in ports:
		            p.y._value -= y
		dirty_matrix_items.update(super(BlockCanvas, self)._normalize(items))
		return dirty_matrix_items

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
			bi = item.blockinstance
			s += "\t%s IS_A %s;\n" % (bi.name, bi.blocktype.type.getName())

		for item in self.get_all_items():
			if not hasattr(item, 'lineinstance'):
				continue
			li = item.lineinstance
			s += ("\t%s, %s ARE_THE_SAME;\n" % (li.fromblock.name, li.toblock.name))
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


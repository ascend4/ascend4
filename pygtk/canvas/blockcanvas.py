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
		# FIXME still need a way of iterating through these items
		items = []
		for item in self.get_all_items():
			if not hasattr(item, 'blockinstance'):
				continue
			bi = item.blockinstance
			if bi.blocktype.type is None:
				bi.blocktype.reattach_ascend(library, notesdb)

	def __getstate__(self):
		return super(BlockCanvas,self).__getstate__()


	def __setstate__(self, state):
		super(BlockCanvas,self).__setstate__(state)


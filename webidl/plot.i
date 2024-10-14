/*

SWIG interface for plotting from ASCEND

*/

%include "plot.h"

%extend Plot{
	%pythoncode %{
		def show(self,mainloop=True):
			import loading
			loading.load_matplotlib(throw=True)
			import pylab
			import platform
			pylab.ioff()
			pylab.figure()
			pylab.title(self.getTitle())
			#print("XLabel:",self.getXLabel())
			pylab.xlabel(self.getXLabel())
			#print("YLabel:",self.getYLabel())
			pylab.ylabel(self.getYLabel())
			_l = []
			_have_legends = False
			for _c in self.curves:
				_f = _c.getFormat()
				_args = [_c.x, _c.y]
				if len(_f):
					_args += [_f]
				if self.isXLog() and self.isYLog():
					pylab.loglog(*_args)
				elif self.isXLog():
					pylab.semilogx(*_args)
				elif self.isYLog():
					pylab.semilogy(*_args)
				else:
					pylab.plot(*_args)
				_l1 = _c.getLegend()
				if _l1:
					_have_legends = True
				_l.append(_l1)
			# only show legends if their text is non-empty:
			if _have_legends:
				pylab.legend(_l,self.getLegendPosition())

			#print("Mainloop:",mainloop)
			pylab.ion()
			if platform.system()=="Windows":
				pylab.show()
			else:
				pylab.show(mainloop)
				
			
			# /*
			# if self.getXLow() and self.getXHigh():
			#	matplotlib.gca().set_xlim([self.getXLow(), self.getXHigh()])
			# */
	%}
}


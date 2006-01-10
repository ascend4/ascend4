/*

SWIG interface for plotting from ASCEND

*/


class Plot : public Instanc{
	
public:
	const std::string getTitle() const;
	const std::string getXLabel() const;
	const std::string getYLabel() const;
	const bool isXLog() const;
	const bool isYLog() const;
	const double getXLow() const;
	const double getXHigh() const;
	const double getYLow() const;
	const double getYHigh() const;
	
	std::vector<Curve> curves;
};


%extend Plot{
	%pythoncode{
		def show(self):
			import pylab;
			pylab.figure()
			pylab.title(self.getTitle())
			pylab.xlabel(self.getXLabel())
			pylab.ylabel(self.getYLabel())
			_l = []
			for _c in self.curves:
				print "ADDING CURVE..."
				if self.isXLog() and self.isYLog():
					pylab.loglog(_c.x, _c.y)
				elif self.isXLog():
					pylab.semilogx(_c.x, _c.y)
				elif self.isYLog():
					pylab.semilogy(_c.x, _c.y)
				else:
					print "LINEAR..."
					for x in _c.x:
						print x
					pylab.plot(_c.x, _c.y)
				_l.append(_c.getLegend())
			pylab.legend(_l)
			pylab.show()
			# /*
			# if self.getXLow() and self.getXHigh():
			#	matplotlib.gca().set_xlim([self.getXLow(), self.getXHigh()])
			# */
	}
}


#ifndef ASCXX_PLOT_H
#define ASCXX_PLOT_H

#include <vector>
#include <string>

#include "instance.h"

#ifndef ASCXX_CURVE_H
#include "curve.h"
#else
class Curve;
#endif

#define PLOT_TITLE "title"
#define PLOT_XLABEL "XLabel"
#define PLOT_YLABEL "YLabel"
#define PLOT_XLOG "Xlog"
#define PLOT_YLOG "Ylog"
#define PLOT_XLO "Xlow"
#define PLOT_XHI "Xhigh"
#define PLOT_YLO "Ylow"
#define PLOT_YHI "Yhigh"
#define PLOT_CURVE "curve"
#define PLOT_LEGEND "legend"
#define PLOT_FORMAT "format"
#define PLOT_LEGENDPOSITION "legend_position"
#define PLOT_POINT "pnt"
#define PLOT_XPOINT "x"
#define PLOT_YPOINT "y"


/**
	This is interface for accessing plottable data from ASCEND. It's needed in order
	to cleanly access the MatPlotLib commands via Python, but it could also be used to
	abstract the xgraph plotting code somewhat as well.
*/
class Plot : public Instanc{

private:
	friend class Instanc;
	explicit Plot(const Instanc &);
	Plot();
public:
	Plot(const Plot &plot);	

	const std::string getTitle() const;
	const std::string getXLabel() const;
	const std::string getYLabel() const;

	/**
		Get 'legend position', an integer for use by matplotlib, see here for
		documentation:
		http://matplotlib.sourceforge.net/api/pyplot_api.html#matplotlib.pyplot.legend

		This value is not used by the Tcl/Tk GUI's plotting routine.
	*/
	const int getLegendPosition() const;

	const bool isXLog() const;
	const bool isYLog() const;
	const double getXLow() const;
	const double getXHigh() const;
	const double getYLow() const;
	const double getYHigh() const;
	
	std::vector<Curve> curves;
};

#endif

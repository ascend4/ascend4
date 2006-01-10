#include "plot.h"
#include "curve.h"

#include <compiler/plot.h>

#include <iostream>
using namespace std;

Plot::Plot(const Instanc &i) : Instanc(i){
	cerr << "Creating plot..." << endl;
	// create Curve objects as required:
	
	Instanc curve_array = getChild(PLOT_CURVE);
	vector<Instanc> cc = curve_array.getChildren();
	vector<Instanc>::iterator cci;
	for(cci=cc.begin();cci<cc.end(); ++cci){
		curves.push_back( Curve(*cci) );
	}
}

const string
Plot::getTitle() const{
	return getChild(PLOT_TITLE).getValueAsString();
}

const string
Plot::getXLabel() const{
	return getChild(PLOT_XLABEL).getValueAsString();
}

const string
Plot::getYLabel() const{
	return getChild(PLOT_YLABEL).getValueAsString();
}

const bool
Plot::isXLog() const{
	return getChild(PLOT_XLOG).getBoolValue();
}

const bool
Plot::isYLog() const{
	return getChild(PLOT_YLOG).getBoolValue();
}

const double
Plot::getXLow() const{
	return getChild(PLOT_XLO).getRealValue();
}

const double
Plot::getXHigh() const{
	return getChild(PLOT_XHI).getRealValue();
}

const double
Plot::getYLow() const{
	return getChild(PLOT_YLO).getRealValue();
}

const double
Plot::getYHigh() const{
	return getChild(PLOT_YHI).getRealValue();
}


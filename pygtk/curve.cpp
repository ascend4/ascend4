#include "curve.h"

#include <compiler/plot.h>
#include <stdexcept>
using namespace std;

Curve::Curve(const Instanc &i) : Instanc(i){
	cerr << "Created curve";

	// may through 'child not found'...
	Instanc point_array = getChild(PLOT_POINT);

	vector<Instanc> pa = point_array.getChildren();
	vector<Instanc>::iterator pai;

	for(pai = pa.begin(); pai < pa.end() ; ++pai){
		Instanc xinst = pai->getChild(PLOT_XPOINT);
		Instanc yinst = pai->getChild(PLOT_YPOINT);
		if(xinst.isAssigned() && yinst.isAssigned()){
			x.push_back(xinst.getRealValue());
			y.push_back(yinst.getRealValue());
		}
	}
}

Curve::Curve(){
	throw runtime_error("not allowed");
}

Curve::Curve(const Curve &old) : Instanc(old.getInternalType()){
	x = old.x;
	y = old.y;
}

const string
Curve::getLegend() const{
	Instanc li = getChild(PLOT_LEGEND);
	if(li.isAssigned()){
		return li.getSymbolValue().toString();
	}
	return "";
}

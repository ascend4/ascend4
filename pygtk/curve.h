#ifndef ASCXX_CURVE_H
#define ASCXX_CURVE_H

#include <vector>
#include <string>

#include "plot.h"
#include "instance.h"

class Curve : public Instanc{

private:
	friend class Plot;
	Curve(const Instanc &);	
public:
	Curve();
	Curve(const Curve &);
	std::vector<double> x;
	std::vector<double> y;
	const std::string getLegend() const;

};

#endif

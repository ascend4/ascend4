#ifndef ASCXX_CURVE_H
#define ASCXX_CURVE_H

#include <vector>
#include <string>

#include "plot.h"
#include "instance.h"

class Curve : public Instanc{
	friend class std::vector<Curve>;
private:
	friend class Plot;
	explicit Curve(const Instanc &);	
	Curve();
public:
	Curve(const Curve &);
	std::vector<double> x;
	std::vector<double> y;
	const std::string getLegend() const;
};

#endif

#ifndef ASCXX_RELATION_H
#define ASCXX_RELATION_H

#include <string>
#include <vector>

#include "simulation.h"

struct Variable;

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <system/slv_types.h>
#include <system/rel.h>
}

class Relation : public Instance{

private:
	Simulation *sim;
	struct rel_relation *rel;

public:
	Relation();
	Relation(const Relation &old);
	Relation(Simulation *sim, rel_relation *rel);

	const std::string getName() const;
	const double getResidual() const;
	const std::vector<Variable> getIncidentVariables() const;
	const int getNumIncidentVariables() const;
	Instanc getInstance() const;
	std::string getRelationAsString() const;
};

#endif /* ASCXX_RELATION_H */

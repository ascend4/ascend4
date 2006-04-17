#ifndef ASCXX_RELATION_H
#define ASCXX_RELATION_H

#include <string>

#include "simulation.h"

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/rel.h>
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

};

#endif /* ASCXX_RELATION_H */

#include "library.h"
#include "type.h"
#include "units.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace std;

int main(void){
	try{
		Library L;
		const char *model = "\n\
			UNITS\n\
				MW = {1e6*kg*m^2/s^3};\n\
				MWh = {3.6e9*kg*m^2/s^2};\n\
			END UNITS;\n\
			ATOM atom_decl_units REFINES real DIMENSION M*L^2/T^3 DEFAULT 3 {MW};\n\
			END atom_decl_units;\n\
			ATOM atom_no_decl_units REFINES real DIMENSION M*L^2/T^3;\n\
			END atom_no_decl_units;\n\
			CONSTANT const_decl_units REFINES real_constant UNITS {MWh};";

		L.loadString(model, "declunits_cpp_test");

		Type t_atom = L.findType("atom_decl_units");
		const char *atom_units = t_atom.getDeclaredUnits();
		if(atom_units == NULL || 0 != strcmp(atom_units, "MW")){
			throw runtime_error("atom declared units token mismatch");
		}
		UnitsM atom_u(atom_units);
		if(atom_u.getName().toString() == NULL){
			throw runtime_error("atom declared units object invalid");
		}

		Type t_const = L.findType("const_decl_units");
		const char *const_units = t_const.getDeclaredUnits();
		if(const_units == NULL || 0 != strcmp(const_units, "MWh")){
			throw runtime_error("constant declared units token mismatch");
		}
		UnitsM const_u(const_units);
		if(const_u.getName().toString() == NULL){
			throw runtime_error("constant declared units object invalid");
		}

		Type t_none = L.findType("atom_no_decl_units");
		if(t_none.getDeclaredUnits() != NULL){
			throw runtime_error("expected NULL declared units for atom_no_decl_units");
		}
	}catch(const runtime_error &e){
		cerr << "testdeclunits: " << e.what() << endl;
		return 1;
	}

	return 0;
}

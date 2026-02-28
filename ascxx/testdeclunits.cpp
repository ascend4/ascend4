#include "library.h"
#include "type.h"
#include "simulation.h"
#include "units.h"

#include <cstring>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace std;

int main(void){
	try{
		Library L;
		const char *model = "\n\
			UNITS\n\
				MW = {1e6*kg*m^2/s^3};\n\
				W = {kg*m^2/s^3};\n\
				kW = {1e3*W};\n\
				GW = {1e9*W};\n\
				MWh = {3.6e9*kg*m^2/s^2};\n\
			END UNITS;\n\
			UNITS LADDER\n\
				W = {kg*m^2/s^3};\n\
				kW = {1e3*W};\n\
				MW = {1e6*W};\n\
				GW = {1e9*W};\n\
			END UNITS LADDER;\n\
			ATOM atom_decl_units REFINES real DIMENSION M*L^2/T^3 DEFAULT 9500 {MW};\n\
			END atom_decl_units;\n\
			ATOM atom_no_decl_units REFINES real DIMENSION M*L^2/T^3;\n\
			END atom_no_decl_units;\n\
			CONSTANT const_decl_units REFINES real_constant UNITS {MWh};\n\
			MODEL display_units_probe;\n\
				a IS_A atom_decl_units;\n\
			END display_units_probe;";

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
		if(!atom_u.hasLadder()){
			throw runtime_error("expected atom declared units to have ladder membership");
		}
		{
			UnitsM auto_u = atom_u.getAutoScaledUnits(9.5e6);
			if(0 != strcmp(auto_u.getName().toString(), "MW")){
				throw runtime_error("auto-scaled units mismatch for 9.5e6 SI");
			}
		}
		{
			Type t_probe = L.findType("display_units_probe");
			Simulation sim = t_probe.getSimulation("sim1", false);
			Instanc a = sim.getModel().getChild("a");
			UnitsM disp_u = a.getDisplayUnits();
			double value_in_disp = a.getRealValue() / disp_u.getConversion();
			if(0 != strcmp(disp_u.getName().toString(), "GW")){
				throw runtime_error("instance display units resolution mismatch");
			}
			if(fabs(value_in_disp - 9.5) > 1e-6){
				throw runtime_error("display units conversion mismatch");
			}
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

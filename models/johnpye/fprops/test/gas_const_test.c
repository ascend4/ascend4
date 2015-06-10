/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*/
/*
	by Jacob Shealy, June 9, 2015

	Check that the molar ideal-gas constant `R' is really the same for all fluids
 */

#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

#define GAS_NFLUIDS 39

int main(){
	char *fluids[]={
		"acetone",
		"ammonia",
		"butane",
		"butene",
		"carbondioxide",
		"carbonmonoxide",
		"carbonylsulfide",
		"cisbutene",
		"decane",
		"dimethylether",
		"ethane",
		"ethanol",
		"hydrogen",
		"hydrogensulfide",
		"isobutane",
		"isobutene",
		"isohexane",
		"isopentane",
		"krypton",
		"methane",
		"neopentane",
		"nitrogen",
		"nitrousoxide",
		"nonane",
		"oxygen",
		"parahydrogen",
		"propane",
		"r116",
		"r134a",
		"r141b",
		"r142b",
		"r218",
		"r245fa",
		"r41",
		"sulfurdioxide",
		"toluene",
		"transbutene",
		"water",
		"xenon"
	};
	PureFluid *helms[GAS_NFLUIDS];
	
	int i;
	printf("\n  The value of the gas constant is:");
	for(i=0;i<GAS_NFLUIDS;i++){
		helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		printf("\n\tFor %s \t%s-- %.6g J/kg/K\t-->\t%.6g J/kmol/K",
				fluids[i], (strlen(fluids[i])<11) ? "\t" : "",
				helms[i]->data->R, helms[i]->data->R * helms[i]->data->M);
	} puts("");

	return 0;
}

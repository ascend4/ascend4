/* SCons will create the file 'fluids_list.h' automatically using the file
'fluids_list.h.in as a basis. A listing of *.c files in the 'fluids' directory
is used to create that list, so in fact no special code is required to add a new
fluid to FPROPS other than just running SCons and recompiling FPROPS again. */

#include "_rpp.h"

#define FLUIDS(F,X)\
	F(acetone) X \
	F(ammonia) X \
	F(butane) X \
	F(butene) X \
	F(carbondioxide) X \
	F(carbonmonoxide) X \
	F(carbonylsulfide) X \
	F(cisbutene) X \
	F(decane) X \
	F(dimethylether) X \
	F(ethane) X \
	F(ethanol) X \
	F(hydrogen) X \
	F(hydrogensulfide) X \
	F(isobutane) X \
	F(isobutene) X \
	F(isohexane) X \
	F(isopentane) X \
	F(krypton) X \
	F(methane) X \
	F(neopentane) X \
	F(nitrogen) X \
	F(nitrousoxide) X \
	F(nonane) X \
	F(oxygen) X \
	F(parahydrogen) X \
	F(propane) X \
	F(r116) X \
	F(r134a) X \
	F(r141b) X \
	F(r142b) X \
	F(r218) X \
	F(r245fa) X \
	F(r41) X \
	F(sulfurdioxide) X \
	F(toluene) X \
	F(transbutene) X \
	F(water) X \
	F(xenon)

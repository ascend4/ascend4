$Revision: 1.5 $

Topics-

1) content description
2) file extensions
3) file names
4) subdirectories
5) File summaries


1) Description:

This directory tree contains the ASCEND IV code for library models and
example models.

All the files in this directory hierarchy are Copyright
Carnegie Mellon University and distributed under the GNU
License version 2 unless OTHERWISE noted in an individual
file.

All 'supported' models are in this top level directory.
Subdirectories are supported by their authors (if at all) and
not by the ASCEND Project at large.


2) Files have 5 extensions:

.a4s:  A Script (tcl) for loading in the ASCEND script window.
.a4l:  A library of reusable models. Each library contains enough
       test models to be self-validating and self-documenting.
.a4c:  Model code examples and demos.
.a4v:  A file of saved values from a simulation.
.tcl:  Auxillary TCL scripts for performing miscellaneous tasks
       related to modeling that the user may wish to copy and
       modify: for example, the user can create many permutations
       on the set_intervals.tcl functions which specify a
       time sampling schedule for the integrators.

and 1 prefix

z-      normally of no interest to users, z files are models that
        contain code testing 1 or more features of the system which
        are not ordinarily tested in the course of routine distillation
        modeling.


3) A note on file names:

We frequently develop alternative modeling strategies as part of
research. Since all our models live in one directory, several files
may have names starting with part of the author's name to distinguish
them from similar sets of models by another author. 
Groups of related models may be kept in subdirectories.


4) Subdirectories:

Most subdirectories only exist at Carnegie Mellon, and are generally
full of experimental goop designed to frustrate the casual user.
They also contain legacy goop. Old style extensions in use for
these files include .s (now .a4s), .lib (now .a4l), .asc (now .a4c).
.values (now .a4v), .units (now defunct).

ben:	This directory contains the models presented in Ben Allan's
	thesis. These models are somewhat less flexible than the
	main models directory, but are generally less consuming of
	computer CPU tim/memory resources.

libraries:  This directory contains *.lib files, containing model
	    libraries.  There is a link between H_G_thermodynamics.lib
	    and thermo.lib.

examples:   This directory contains example files.  Each example has a
	    *.asc and a *.s file.  The *.s is a script for running the
	    models in the *.asc file.

pending:    This directory contains possible additions or changes to the
	    libraries and examples directories.  The models in these
	    directories fall under the GNU license, but there are no
	    guarantees on how well they will work.  A generic GNU
	    header is provided in each of the sub-directories to be
	    added to any models placed in pendings.

westerberg  This directory contains models that Art Westerberg is 
            developing.  

            Of interest are the simpleUnits.AWW.a4s/c script and
            models.  These models allow one to develop simple
            flowsheet models for stream mixing, splitting, pressure
            changing (compressor, valve) while using rigorous physical
            properties.  The H2Process models use these models to
            model a desktop hydrogen plant.  

            The models in SiirolaJohn are also for modeling a desktop
            hydrogen plant.  

	    The CelayaDemo models mimic the simple_FS models replaced
            with rigorous units and physical property calculations.

	    The ivp models are for solving initial value DAE models.
	    See the ASCEND Wiki description for this work at

        https://pse.cheme.cmu.edu/wiki/view/Ascend/InitialValueModeling


5) Models are primarily chemical engineering library application
except as OTHERWISE noted.

guthriecosts.a4l
	Cost correlation modeling, chemical engineering.
KenPendings.a4l
	Experimental code.
abc_flowsheet.a4l
	Experimental code.
atoms.a4l
	Basic variable definitions for all engineering and physics.
	Additional ATOM contributions welcomed.
basemodel.a4l
	Base system library.
bvp.a4l
	Generic mathematical boundary value problem framework for
	differential algebraic equations.
casestudy.a4s
	Simple example of case studies for chemical engineering flash MODEL.
collocation.a4l
	Reduced order distillation modeling by collocation.
	Variable number of trays and tray sections supported.
collocation.a4s
	Demo script for collocation.a4l.
collocation_tests.a4s
	Obsolete file.
column.a4l
	Rigorous tray-by-tray distillation models for chemical engineering.
	Fixed number of trays user configurable. Mass or energy balance
	options.
components.a4l
	Physical properties database from Reid Prausnitz and Poling
	(McGraw-Hill) for chemical engineering. Will be expanded greatly
	shortly with permission from McGraw-Hill.
cost_column.a4l
	Economic evaluation of collocation modeled distillation.
distance_calc.a4c
	Simple physics example.
dyn_column.a4l
	Dynamic simulation of vapor-liquid distillation in
	chemical engineering. Mass or energy balance options.
dyn_flash.a4l
	Dynamic simulation of vapor-liquid flash, condensers, reboilers etc
	for chemical engineering.
dyn_separation_demos.a4s
	Demo script for dynamic simulation of flash and distillation models.
dyn_tank.a4c
dyn_tank.a4s
	Dynamic simulation of water level in a tank.
flash.a4l
	Steady-state flash separation models for chemical engineering.
	Condenser, reboiler, tray, etc. Mass or energy balance options.
force1d.a4c
force1d.a4s
	Mass-spring-force models for basic physics or mechanical engineering.
heatex.a4c
heatex.a4s
	Heat exchange MODEL with condensation boundary determination.
ivpsystem.a4l
	Basic variable definitions for algebraic and initial value problem
	solvers.
kinetics.a4l
kinetics.a4s
	Models for computing chemical reactions.
linear_balance.a4c
linear_balance.a4s
	Trivial mass-balance flowsheet superstructures for chemical
	engineering solved with conditional modeling. Requires CONOPT
measures.a4l
	Units of measure definitions based on MKS system.
mix.a4l
mix.a4s
	Mixer unit for process streams in chemical engineering.
old_separation_demos.a4s
	Steady-state flash and distillation models from Ben Allan's thesis.
phaseq.a4c
phaseq.a4s
phaseq_comp.a4c
phaseq_comp.a4s
phases.a4l
pipeline.a4c
pipeline.a4s
	Conditional models of multiphase thermodynamic equilibrium
	and pipe networks.
plot.a4l
	Antiquated but sometimes useful plot package.
plotbvp.a4c
plotbvp.a4s
	Plot package application example for differential equations.
plotcol.a4c
	Plot package application example for distillation.
rachford.a4c
rachford.a4s
	Simple Rachford-Rice flash calculation.
ratelaws.a4c
	Chemical kinetics examples.
reactor.a4l
reactor.a4s
	Chemical reactor modeling examples, chemical engineering.
roots_of_poly.a4c
	Simple equation solving example. Mathematics.
separation_demos.a4s
	Steady-state flash and distillation models.
set_intervals.tcl
	Support script for initial value solver interface.
simple_fs.a4c
simple_fs.a4s
	Obsolete flowsheet structuring examples a la ASCEND III.
simple_fs_cost.a4c
simple_fs_cost.a4s
simple_fs_ext.a4c
	Obsolete flowsheet optimization examples a la ASCEND III.
	Requires CONOPT.
sonic.a4c
sonic.a4s
	Calculation of sonic flow transition in a pipe. Chemical/mechanical
	engineering.
splitter.a4l
splitter.a4s
	Process stream splitter models for chemical engineering.
stream_holdup.a4l
	Basic process stream definitions.
system.a4l
	Basic variable definition for algebraic solvers.
ternary_plot.a4l
	Plot library for 3 component mixtures in distillation columns from
	chemical engineering.
thermodynamics.a4l
	Vapor, liquid, and equilibrium thermodynamic calculation library.
vessel.a4c
vessel.a4s
vesselMethods.a4c
vesselNotes.a4c
vesselParams.a4c
vesselPlain.a4c
vesselPlot.a4c
vesselPlot.a4s
vesselStudy.a4s
vesselTabulated.a4c
vesselTabulated.a4s
	Simple models accumulated from the HowTo's for ASCEND.
when_demo.a4c
when_demo.a4s
	Simple example of conditional modeling.
z-addmethod.a4c
z-align.a4c
z-alike.a4c
z-anontype.a4c
z-arsubs.a4c
z-context.a4c
z-emptyarg.a4c
z-emptyfor.a4c
z-iflogic.a4c
z-indirect.a4c
z-isawhere.a4c
z-relname.a4c
z-suite.a4s
	Software Quality Assurance tests and bug documentation.
	Not for routine use.

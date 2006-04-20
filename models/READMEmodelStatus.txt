This file contains the status of the models in the ASCEND model
directory.

--------- 2006/04/10  AWW

The following model (.a4c) and script (.a4s) files were tested on the
dates indicated and work as they should.  The compiler issues a number
of "NULL children" warnings (null children are legal in ASCEND
modeling).  The solver issues a number of NULL parameter error
messages. 

simpleflowsheet.a4s
simpleflowsheet01.a4c
simpleflowsheet01cost.a4c
simpleflowsheet01cost.a4s
simpleflowsheet01mass.a4c
simpleflowsheet02.a4c
simpleflowsheetrigorous.a4c*
simpleflowsheetrigorous.a4s*
simpleunits.a4c*
simpleunits.a4s*
simpleunitsatoms.a4c*

--------- 2006/04/11  AWW

I ran the following scripts.  They functioned as their comments stated
they should.

casestudy.a4s
collocation.a4s
dyn_separation_demos.a4s  placed a comment about the "source" statement
                          requiring a full path definition. Runs
                          correctly.


The following scripts did not work.

collocation_tests.a4s  attempts to compile mw_demo_column, a type that
                       no longer is in collocation.a4l.  I am removing
                       this file from the model library.  You can
                       retrieve it in versions earlier than 531.


--------- 2006/04/20  AWW

dyn_tank.a4s           placed a comment about the "source" statement
                       requiring a full path definition.  Runs correctly.
forceld.a4s            runs correctly
heatex.a4s             requires solver CMSlv - not available under linux 
                       at this time.  Runs okay using solver QRSlv.
kinetics.a4s           runs correctly.
linear_balance.a4s     requires solver CMSlv - not available under linux
                       at this time.  Runs okay using solver QRSlv.
mix.a4s                runs correctly.
old_separation
  _demos.a4s           runs correctly
phaseq_comp.a4s        runs correctly.
plotbvp.a4s            runs correctly.
rachford.a4s           requires solver CMSlv - not available under linux
                       at this time.  Runs okay using solver QRSlv.
reactor.a4s            placed a comment about the "source" statement
                       requiring a full path definition.  Runs correctly.
separation_demos.a4s   runs correctly.
sonic.a4s              requires solver CMSlv - not available under linux
                       at this time.  Exceeds iteration limit when
		       solving with QRSlv.
splitter.a4s           runs correctly.
vesselPlot.a4s         runs correctly.
vesselStudy.a4s        runs correctly.
vesselTabulated.a4s    runs correctly.
when_demo.a4s          runs correctly.


The following scripts did not work in part, as noted

phaseq.a4s             requires solver CMSlv - not available under linux
                       at this time.  Exceeds iteration limit when
		       solving with QRSlv.
pipeline.a4s           requires solver CMSlv - not available under linux
                       at this time.  Exceeds iteration limit when
		       solving with QRSlv.
vessel.a4s             runs up to solving with first QRSlv statement.
                       Then it requires CONOPT - not available under
                       linux.  Then it requires gams - not attempted.
z-suite.a4s            generates many error messages.


--------- 
--------- 
--------- 
--------- 
--------- 


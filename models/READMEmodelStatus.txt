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
dyn_separation_demos.a4s  Placed a comment about the "source" statement
                          requiring a full path definition.



The following scripts did not work.

collocation_tests.a4s  attempts to compile mw_demo_column, a type that
                       no longer is in collocation.a4l.  I am removing
                       this file from the model library.  You can
                       retrieve it in versions earlier than 531.






--------- 
--------- 
--------- 
--------- 
--------- 


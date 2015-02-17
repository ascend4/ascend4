To ensure ASCEND builds the FPROPS python wrappers correctly and without error,
it may be necessary to set the path to the FPROPS header files directly.  This
can be done on line 37 of the Sconstruct file in the FPROPS python folder.  An
example follows:

,CPPPATH=['#',distutils.sysconfig.get_python_inc(),'/home/sean/ascend/models/johnpye/fprops']

Change the last path to point to the FPROPS header files within your own ASCEND
installation.  Another option is to add this path to your c compiler, whichever
you like.  There must be a better way to handle this, but right now try this if
you get a compiler error from the ASCEND install script.

Also, it may be necessary to set some ASCEND environment variables to be able
to import the FPROPS code.  The path to the compiled libraries must be
available to ASCEND through the ASCENDLIBRARY environment variable.  This can
be set by adding the following code to the bottom of your .bashrc file (linux):

export ASCENDLIBRARY=${ASCENDLIBRARY}:/home/sean/ascend/models;

Again, change the path to fit your own ASCEND install.

This branch will compile ASCEND and FPROPS with the Peng Robinson equation of
state.  Functions available include pressure, enthalpy, entropy, internal
energy, gibbs energy, helmholtz energy, cp, cv, and fluid sonic velocity.
Check the ASCEND FPROPS page for details and capabilities.  Some example ASCEND
models are available in the FPROPS directory which show the syntax necessary to
use these functions.

-- 
Sean Muratet
12 August 2012

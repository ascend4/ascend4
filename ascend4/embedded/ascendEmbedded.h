/**
   Ascend Environment Object (C version)
*/

struct AscendInterface {

  int g_compiler_timing;  /* RENAME these everywhere without g_ */
  int g_interface_simplify_relations;
  int g_interfacever;
  int g_tty;
  int relns_flag;

 };
typedef struct AscendInterface * A4ptr;


/**
   Create an Ascend Environment
*/

extern A4ptr createAscendEnvironment( );


/**
   Destroy an Ascend Environment
*/

extern void destroyAscendEnvironment(A4ptr environment);


/**
   Load and parse an ascend model file into Ascend Enviroment.  All
   other required files are also loaded.

   @param environment     Self pointer to environment
   @param filename        Name of the file to be loaded and parsed
   @return                0: all is well

 */

extern int readAscendFile(A4ptr environment, 
		   char * fileName);


/**
   Compile an instance of the named model within the loaded file

   @param environment       Self pointer to environment
   @param rootInstanceName  Name to be given to the compiled "root" instance
   @param modelName         Name of model to be compiled 
   @return                  0: all is well

*/

extern int compileAscendInstance(A4ptr environment,
			  char * simulationName,
			  char * modelName);

/**
   Run a method on an instance defined within the enviroment
   @param environment       Self pointer to environment
   @param methodName        Name of the method (including path name within 
                            environment
   @return                  0: all is well
*/

extern int rundAscendMethod(A4ptr environment,
		     char * methodName);

typedef struct AscendSlvSystem * SlvSystemPtr;

/**
   Create/Derive a solve system for an instance (which may be a part
   of the root instance) in the Ascend Environment
   @param environment       Self pointer to environment
   @param solverName        Name of the solver to be used when solving
   @param instanceName      Name of instance for which solve system is built
   @return                  0: all is well
*/

extern SlvSystemPtr createAscendSolverSystem(A4ptr environment,
				  char * solverName,
				  char * instanceName);


/**
   Destroy a solve system for an instance in the Ascend Environment
   @param environment       Self pointer to environment
   @system                  Name of system to destroy
*/

extern void destroyAscendSolverSystem(A4ptr environment,
			       SlvSystemPtr system);


/**
   Solve an Ascend instance
   @SlvSystemPtr            Pointer to the Solve System created for the instance
   @return                  0: all is well
*/

extern int solveAscendSlvSystemPtr(SlvSystemPtr system);


extern intgetSolverBooleanResult(SlvSystemPtr system,
			   char * property);

enum IntegrationMethod{
  AM,
  BDF
};

extern intsetRealValue(A4ptr environment,
		 char * variableName,
		 double value,
		 char * units);

extern intgetRealValue(A4ptr environment,
		 char * variableName,
		 double * valuePtr,
		 char * units);

extern intsetIntValue(A4ptr environment,
		char * variableName,
		int value);

extern intgetIntValue(A4ptr environment,
		char * variableName,
		int * valuePtr);

extern intsetBooleanValue(A4ptr environment,
		    char * variableName,
		    int value);

extern intgetBooleanValue(A4ptr environment,
		    char * variableName,
		    int * valuePtr);

extern intsetSymbolValue(A4ptr environment,
		   char * variableName,
		   int value);

extern intgetSymbolValue(A4ptr environment,
		   char * variableName,
		   int * valuePtr);




			 

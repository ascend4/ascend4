/**
 * <p>
 * SystemCpp is the fundamental mathematical INTERFACE we export
 * and import with ASCEND. It is an abstract interface that any
 * modeling system can support, and it is made of a collection of
 * objects represented as other abstract interfaces.
 * </p>
 * <p>Design by Benjamin Andrew Allan, 9/2000</p>
 * <p>
 * Design notes:
 * <ul>
 * <li>We are assuming that ASCEND has in fact captured a covering
 *     set of information for describing mathematical problems, or
 *     that anything ASCEND fails to capture is simply a future addition
 *     which will not break any code at the source level.
 *     To back this assumption, we are making the system objects 
 *     keyed by strings for generic access as well as direct query.</li>
 * <li>We are not positing an inheritance hierarchy for describing
 *     <em>solvers</em> as solvers will be plugged into ASCEND
 *     as black box components that operate on Systems any way
 *     they want.</li>
 * <li>Solver component implementers may find it most convenient to 
 *     steal some of our partial implementations.</li>
 * <li>There is more than one System in existence at any given time.</li>
 * <li>Systems must be composable from other systems, accounting for
 *     possible overlaps.</li>
 * <li>Systems may derive subsystems for solving subproblems.</li>
 * <li>Given the above, a relation, variable, etc will exist in any
 *     number of systems simultaneously.</li>
 * <li>Thread safety is not an immediate issue, but we want to
 *     manage data sensibly so as not be future-thread-hostile.</li>
 * <li>Subclasses of SystemCpp may be derived which enforce tighter
 *     semantics, such as "LinearProgramSystem", but in general
 *     properties of variables and relations are not
 *     be reflected at the generic system-level, as new properties
 *     are always being discovered and incorporated into math 
 *     programming algorithms. 
 * <li>SystemCpp implementations not supporting a certain class
 *     of variable or relation can indicate it by error codes.
 * <li>A SystemCpp implementation is not obliged to store its
 *     variables, etc, in arrays, but it must hand them out that way.
 * </li>
 * </ul>
 * </p>
 */
class SystemCpp {

  /** Get the array of object pointers matching the type specified and
      the size of the array.
      @return 0 if ok, -1 if the type is not present in the system.
      @param type The name corresponding to an array of isomorphic objects.
             Well-known type names include:
           Variables Equalities Inequalities Objectives Boundaries
*/
  virtual int getObjectPointerArray(const char * type, void * & pArray, int &np) = 0;

  /** Get the list of floating-point variables.
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support FP
       variables.
  */
  virtual int getVariables(VarCpp * & vArray, int &nV) = 0; 

  /** Get the list of floating-point equality constraints
      among VarCpp. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support FP
       equalities.
  */
  virtual int getEqualities(RelCpp * & eArray, int &nE) = 0; 

  /** Get the list of floating-point inequality constraints
      among VarCpp. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support FP
       inequalities.
  */
  virtual int getInequalities(RelCpp * & iArray, int &nIE) = 0; 

  /** Get the list of floating-point objectives
      on VarCpp. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support FP
       objectives.
  */
  virtual int getObjectives(RelCpp * & oArray, int &nO) = 0; 

  /** Get the list of floating-point region boundary definitions
      on VarCpp. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support FP
       region boundaries.
  */
  virtual int getBoundaries(BndCpp * & bArray, int &nB) = 0; 

  /** Get the list of discrete variables. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support
       discrete variables.
  */
  virtual int getDiscretes(DisCpp * & dArray, int &nD) = 0; 

  /** Get the list of logical relations among DisCpp,
      and possibly among discrete variables and the residuals of
      BndCpp. 
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support
       logical relations.
  */
  virtual int getLogicalRelations(LogRelCpp * & lrArray, int &nLR) = 0; 

  /** Get the list of When/Conditional constructs in the system.
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support
       When constructs.
  */
  virtual int getWhens(WhenCpp * & wArray, int &nW) = 0; 

  /** Get the list of models in the system.
      @return 0 if array returned in arguments (which may be
       in fact a null list) and -1 if the system doesn't support
       model graph information.
  */
  virtual int getModels(ModCpp * & dArray, int &nM) = 0; 

};

/** 
 * <p>
 * SystemFactory is an INTERFACE for building SystemCpps.
 * It can be implemented differently for every particular solver.
 * In particular, we want it to be useful for building a system
 * either from scratch or from other systems.
 * Adding a variable, equality, etc, more than once is not a
 * cause for error nor should it result in a SystemCpp instance
 * with redundant information.
 * </p>
 * <p>Design by Benjamin Andrew Allan, 9/2000</p>
 */
class SystemFactory {
  /** Add variable(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addVariables(VarCpp * vArray, int nV, char * & errorMessage) = 0; 

  /** Add equality(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addEqualities(RelCpp * eArray, int nE, char * & errorMessage) = 0; 
  /** Add inequality(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addInequalities(RelCpp * iArray, int nIE, char * & errorMessage) = 0; 
  /** Add objective(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addObjectives(RelCpp * oArray, int nO, char * & errorMessage) = 0; 
  /** Add boundary(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addBoundaries(BndCpp * bArray, int nB, char * & errorMessage) = 0; 
  /** Add discrete variable(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addDiscretes(DisCpp * dArray, int nD, char * & errorMessage) = 0; 

  /** Add logical relation(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addLogicalRelations(LogRelCpp * lrArray, int nLR, char * & errorMessage) = 0; 

  /** Add When construct(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addWhens(WhenCpp * wArray, int nW, char * & errorMessage) = 0; 

  /** Add Model construct(s). 
      @return 0 if ok or -1 if not possible. */
  virtual int addModels(ModCpp * mArray, int nM, char * & errorMessage) = 0; 

  /** Perform final assembly and return a system.
   *  @param errorMessage print to see message of failure, if any.
   *  @param sys the system desired, or NULL if failed call.
   *  @return 0 if ok, or -1 if system is incomplete,
   *   or -2 if system cannot be built for any other reason.
   */
  virtual int extractSystem(SystemCpp * & sys, char * & errorMessage) = 0;
};

/** 
 * <p>
 *  INTERFACE for composing systems from other systems.
 *  We follow the rule of composition that all the input
 *  must be valid for the system implementation being built.
 * </p>
 * <p>Design by Benjamin Andrew Allan, 9/2000</p>
 */
class SystemComposer : SystemFactory {

  /** Get all the stuff from sys and add it to the system
      being built. 
      @returns 0 if ok, or -1 if not feasible because the
       system given is missing a key part, or -2 if not
       feasible because the system given has an indigestible
       part. 
   */
  virtual int addSystem(SystemCpp *sys, char * & errorMessage) = 0;

  /**@SystemFactory Interface
  */
  //@{
  virtual int addVariables(VarCpp * vArray, int nV) = 0; 
  virtual int addEqualities(RelCpp * eArray, int nE) = 0; 
  virtual int addInequalities(RelCpp * iArray, int nIE) = 0; 
  virtual int addObjectives(RelCpp * oArray, int nO) = 0; 
  virtual int addBoundaries(BndCpp * bArray, int nB) = 0; 
  virtual int addDiscretes(DisCpp * dArray, int nD) = 0; 
  virtual int addLogicalRelations(LogRelCpp * lrArray, int nLR) = 0; 
  virtual int addWhens(WhenCpp * wArray, int nW) = 0; 
  virtual int addModels(ModCpp * mArray, int nM) = 0; 
  //@}

};

#ifndef Attributes_h_seen
#define Attributes_h_seen

/** 
 * An extensible scalar attributes INTERFACE. The simplest implementation
 *  is some sort of hash table, but more efficient ones are expected.
 *  Attributes are mono-valued (closed-hash) and keyed by name.
 *  Typical usage:
 * <pre>
 * real64 lb;
 * if (a->get("lower_bound",lb)) {
 *    lb = -1.0e20; // default used if undefined.
 * }
 * </pre>
 *  We don't use c++ bool in this interface due to malingering
 *  C++ compiler writers.
 *  This is simply an interface.
 *  We don't ever return or free the name, so it's a good idea
 *  that the name string be guaranteed by the caller to live
 *  longer than the Attributes.
 */
class Attributes {

public:

  virtual ~Attributes() {}

  /** Sets (or defines and sets) an object attribute.
      Yeeow! Watch out using this one. We don't free
      ovalue when destroying Attributes.
  */
  virtual void set(const char *name, void * ovalue) = 0;

  /** Sets (or defines and sets) an integer attribute. */
  virtual void set(const char *name, int32 ivalue) = 0;

  /** Sets (or defines and sets) a long attribute. */
  virtual void set(const char *name, int64 lvalue) = 0;

  /** Sets (or defines and sets) a double attribute. */
  virtual void set(const char *name, real64 rvalue) = 0;

  /** Sets (or defines and sets) a float attribute. */
  virtual void set(const char *name, real32 rvalue) = 0;

  /** Sets (or defines and sets) a string attribute.
   *  svalue is kept, not copied, so it must be longer lived
   *  than the object implementing properties. static strings
   *  are a good choice.
   */
  virtual void set(const char *name, const char *svalue) = 0;

  /** Sets (or defines and sets) a boolean attribute.
   *  The order of arguments is deliberately different
   *  so that it is distinguishable from the integer case.
   *  @param boolValue 0 is FALSE, 1 is TRUE.
   */
  virtual void set(int boolValue, const char *name) = 0;


  /** Get a object attribute and load it into ovalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, void *&ovalue) = 0;

  /** Get an integer attribute and load it into ivalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, int32 & ivalue) = 0;

  /** Get an long attribute and load it into lvalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, int64 & lvalue) = 0;

  /** Get a double attribute and load it into rvalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, real64 & rvalue) = 0;

  /** Get a float attribute and load it into fvalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, real32 & fvalue) = 0;

  /** Get a string attribute and load it into svalue.
   * @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, const char *& svalue) = 0;

  /** Gets a bool attribute and loads it into boolValue.
   * @return 0 if ok, -1 if attribute is undefined. 
   *  The order of arguments is deliberately different
   *  so that it is distinguishable from the integer case.
   *  @param boolValue 0 is FALSE, 1 is TRUE.
   */
  virtual int get(int & boolValue, const char *name) = 0;

  /** Returns the list of attributes and semantics.
   * <p>
   *  This is a rather clunky interface since it is not
   *  generally used for other than UI purposes. 
   *  List is 3*n_Attributes long.
   *  Each triple consists of TYPE,name,pointer to value.
   *  in 3 consecutive entries of the argv.
   * </p>
   *  Type is one of:
   *    VOIDPTR, INT4, LONG8, FLOAT4, DOUBLE8, BOOL4, STRING.
   *  Thus, only the first character of type is significant.
   *  So each tuple must be treated as:
   * <pre>
   *    { char *type; char *name; void *data; } 
   * </pre>
   *  where data is can be interpretted by casting it
   *  as a pointer to the appropriate type, e.g.
   * <pre>
   *  void **argv = attr->getList();
   *  if ( ((char *)argv[0])[0] == 'D') { 
   *    double d = *((double *)argv[2]);
   *  }
   * </pre>
   * <p>
   * Note that for strings and void * objects, data
   * will be the string or object pointer itself, not
   * a reference to the string or object.
   * </p>
   * <p>The list returned is a valid pointer until
   *    the next call to getList. The data in the list
   *    is valid only until another set call is made on the
   *    attributes.
   * </p>
   * @param nAttributes will be
   */
  virtual void **getList(int32 & nAttributes) = 0;

  /** Removes the attribute from among the attributes, if it is
      known. If the name is known with more than one type, both
      are unset.
   */
  virtual void unset(const char *name) = 0;
};
#endif // Attributes_h_seen

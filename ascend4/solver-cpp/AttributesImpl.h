#ifndef AttributesImpl_h_seen
#define AttributesImpl_h_seen

/** private implementation detail. */
struct AttributeData;

/** Ascend default implementation of Attributes interface.
 *  Not bright about computing anything or saving memory.
 *  Testing:
 *  $(CXX) -DMain_AttributesImpl -I.. AttributesImpl.cpp ../archive/libutils.a
 *  ./a.out
 */
class AttributesImpl : Attributes {

public:

  static const int kindCount = 7;

  /** Create an attributes structure for about
      initCount items.
  */
  AttributesImpl();

  /** Destroy an attributes implementation and all the stored values therein.
   */
  virtual ~AttributesImpl();

  /** Sets (or defines and sets) an object attribute.
      Yeeow! Watch out using this one. We don't free it when
      destroying the Attributes.
  */
  virtual void set(const char *name, void * ovalue);

  /** Sets (or defines and sets) an integer attribute. */
  virtual void set(const char *name, int32 ivalue);

  /** Sets (or defines and sets) a long attribute. */
  virtual void set(const char *name, int64 lvalue);

  /** Sets (or defines and sets) a double attribute. */
  virtual void set(const char *name, real64 rvalue);

  /** Sets (or defines and sets) a float attribute. */
  virtual void set(const char *name, real32 rvalue);

  /** Sets (or defines and sets) a string attribute.
      svalue is kept, not copied, so it must be longer lived
      than the object implementing properties. static strings
      are a good choice.  */
  virtual void set(const char *name, const char *svalue);

  /** Sets (or defines and sets) a boolean attribute.
      The order of arguments is deliberately different
      so that it is distinguishable from the integer case.
      @param boolValue 0 is FALSE, 1 is TRUE.
    */
  virtual void set(int boolValue, const char *name);


  /** Get a object attribute and load it into ovalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, void *&ovalue);

  /** Get an integer attribute and load it into ivalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, int32 & ivalue);

  /** Get an long attribute and load it into lvalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, int64 & lvalue);

  /** Get a double attribute and load it into rvalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, real64 & rvalue);

  /** Get a float attribute and load it into fvalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, real32 & fvalue);

  /** Get a string attribute and load it into svalue.
     @return 0 if ok, -1 if attribute is undefined. 
   */
  virtual int get(const char *name, const char *& svalue);

  /** Gets a bool attribute and loads it into boolValue.
     @return 0 if ok, -1 if attribute is undefined. 
      The order of arguments is deliberately different
      so that it is distinguishable from the integer case.
      @param boolValue 0 is FALSE, 1 is TRUE.
   */
  virtual int get(int & boolValue, const char *name);

  /** Returns the list of attributes and semantics.
   * <p>
   *  This is a rather clunky interface since it is not
   *  generally used for other than UI purposes. List
   *  is 3*n_Attributes long.
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
   * @return the list or NULL if malloc fail.
   */
  virtual void **getList(int32 & nAttr);

  /** Removes the attribute from among the attributes, if it is
      known.
   */
  virtual void unset(const char *name);

private:

  /** an array of attribute lists, segregated by type. */
  struct AttributeData *dl[kindCount];

  /** Search an AttributeData list for one matching the name. */
  struct AttributeData *findName(AttributeData *head, const char *name);

  /** Where we keep the last answer to getList. */
  void **list;

};
#endif // AttributesImpl_h_seen

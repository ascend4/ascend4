/*
 *  Hash function
 *  by Tom Epperly
 *  10/24/89
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: hashpjw.h,v $
 *  Date last modified: $Date: 1997/07/18 11:38:39 $
 *  Last modified by: $Author: mthomas $
 */

/** @file
 *  Hash function.
 *  <pre>
 *  When #including hashpjw.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __hashpjw_h_seen__
#define __hashpjw_h_seen__

extern unsigned long hashpjw(CONST char *str, unsigned long size);
/**<
 *  Return a hash value base on str.  The value will be >= 0 and < size.
 *  The specified string may not be NULL, and size must be greater than 0 
 *  (checked by assertion).
 *
 *  @param str  String to use as base for hash (non-NULL).
 *  @param size Maximum value for hash (0 <= hash < size).
 *  @return Returns the hash value.
 */

extern unsigned long hashpjw_int(int id, unsigned long size);
/**<
 *  Return a hash value base on id.  The value will be >= 0 and < size.
 *  The requested size must be greater than 0 (checked by assertion).
 *  This function at the moment does a conversion of the integer to a string
 *  and so is relatively expensive. This needs to be cleaned up, with a
 *  dedicated integer hashing function.
 *
 *  @param id   Integer to use as base for hash.
 *  @param size Maximum value for hash (0 <= hash < size).
 *  @return Returns the hash value.
 *
 *  @todo general/hashpjw.h - Provide integer hashing function (or
 *        possibly remove function as hashpjw_int is not used in ASCEND).
 */

#endif /* __hashpjw_h_seen__ */


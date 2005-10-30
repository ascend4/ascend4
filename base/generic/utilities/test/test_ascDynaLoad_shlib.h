#ifndef TEST_ASCDYNALOAD_SHLIB_H_SEEN
#define TEST_ASCDYNALOAD_SHLIB_H_SEEN

typedef int valuetype;
typedef int (*initFunc)(void);
typedef int (*isInitializedFunc)(void);
typedef void (*cleanupFunc)(void);

/** A public datum. */
extern int DLEXPORT value;             

/** Initializes the library. Returns -5. */
int DLEXPORT init(void);
/** Returns TRUE if library has been initialized, FALSE otherwise. */
int DLEXPORT isInitialized(void);
/** Cleans up the library. */
void DLEXPORT cleanup(void);

#endif  /* TEST_ASCDYNALOAD_SHLIB_H_SEEN */

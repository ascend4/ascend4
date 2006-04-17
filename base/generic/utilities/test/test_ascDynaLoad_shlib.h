#ifndef TEST_ASCDYNALOAD_SHLIB_H_SEEN
#define TEST_ASCDYNALOAD_SHLIB_H_SEEN

typedef int valuetype;
typedef int (*initFunc)(void);
typedef int (*isInitializedFunc)(void);
typedef void (*cleanupFunc)(void);

#ifndef __WIN32__
#define ASC_DLLSPEC
#endif

/** A public datum. */
extern int ASC_DLLSPEC value;             

/** Initializes the library. Returns -5. */
int ASC_DLLSPEC init(void);
/** Returns TRUE if library has been initialized, FALSE otherwise. */
int ASC_DLLSPEC isInitialized(void);
/** Cleans up the library. */
void ASC_DLLSPEC cleanup(void);

#endif  /* TEST_ASCDYNALOAD_SHLIB_H_SEEN */

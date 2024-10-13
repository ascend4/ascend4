#include <ascend/general/platform.h>

typedef int (*initFunc)(void);
typedef int (*isInitializedFunc)(void);
typedef void (*cleanupFunc)(void);

/* ASC_DLLSPEC */ int valuex = FALSE;
                          
ASC_DLLSPEC int init(void){
  valuex = TRUE;
  return -3;
}

ASC_DLLSPEC int isInitialized3(void){
  return valuex;
}

ASC_DLLSPEC void cleanup3(void){
  valuex = FALSE;
}


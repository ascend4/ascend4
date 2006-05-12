#include <utilities/ascConfig.h>
#include "test_ascDynaLoad_shlib.h"

ASC_DLLSPEC(int) value = FALSE;
                          
int init(void)
{
  value = TRUE;
  return -5;
}

int isInitialized(void)
{
  return value;
}

void cleanup(void)
{
  value = FALSE;
}


#include <utilities/ascConfig.h>
#include "test_ascDynaLoad_shlib.h"

int ASC_DLLSPEC value = FALSE;
                          
int ASC_DLLSPEC init(void)
{
  value = TRUE;
  return -5;
}

int ASC_DLLSPEC isInitialized(void)
{
  return value;
}

void ASC_DLLSPEC cleanup(void)
{
  value = FALSE;
}


#include <utilities/ascConfig.h>
#include "test_ascDynaLoad_shlib.h"

int value = FALSE;
                          
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


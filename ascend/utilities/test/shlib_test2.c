#include <ascend/general/platform.h>
#include "shlib_test.h"

int valuex = FALSE;
                          
int init(void)
{
  valuex = TRUE;
  return -4;
}

int isInitialized2(void)
{
  return valuex;
}

void cleanup2(void)
{
  valuex = FALSE;
}


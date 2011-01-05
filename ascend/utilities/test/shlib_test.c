#include <ascend/general/platform.h>
#include "shlib_test.h"

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


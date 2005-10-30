#include "utilities/ascConfig.h"
#include "test_ascDynaLoad_shlib.h"

int DLEXPORT value = FALSE;
                          
int DLEXPORT init(void)
{
  value = TRUE;
  return -5;
}

int DLEXPORT isInitialized(void)
{
  return value;
}

void DLEXPORT cleanup(void)
{
  value = FALSE;
}


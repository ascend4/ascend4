#include <stdio.h>
#include "compiler/redirectFile.h"
FILE *g_ascend_errors=NULL;
FILE *g_ascend_warnings=NULL;
FILE *g_ascend_information=NULL;
 
void Asc_RedirectCompilerDefault() {
  g_ascend_errors = stderr;         /* error file */
  g_ascend_warnings = stderr;       /* whine file */
  g_ascend_information = stderr;    /* note file */

}


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "compiler/compiler.h"
#include "compiler/ascmalloc.h"
#include "compiler/ascpanic.h"
#include "compiler/list.h"
#include "compiler/symtab.h"
#include "utilities/ascEnvVar.h"

/*
acc -vc -g -I.. -DTESTEV ../compiler/list.c ../compiler/pool.c \
../utilities/ascEnvVar.c ../compiler/ascpanic.c testEnvVar.c
or
purify -always-use-cache-dir -cache-dir=/usr1/ballan/tmp/purify -inuse-at-exit=yes acc -vc -g -I.. -DTESTEV ../compiler/list.c ../compiler/pool.c ../utilities/ascEnvVar.c ../compiler/ascpanic.c testEnvVar.c
*/
FILE * g_ascend_errors = stderr;
int main()
{
  char **argv, **argtop;
  char *result=NULL;
  int argc;

  gl_init();
  gl_init_pool();
  Asc_InitEnvironment(2);
  if (Asc_PutEnv(" P3 = foo/bar/baz:biff;boof/ ")) {
    fprintf(stderr," Asc_SetPathList err1\n");
  }
  if (Asc_PutEnv(" P4=foo/b ar/baz : :: biff;boof/ ")) {
    fprintf(stderr," Asc_SetPathList err2\n");
  }
  if (Asc_PutEnv(" P 5=foo/b ar/baz : biff;boof/ ")) {
    fprintf(stderr," Asc_SetPathList err3 expected\n");
  }
  Asc_AppendPath("P1","element1/part2");
  Asc_AppendPath("P1","element2/part3");

  result = Asc_GetEnv("P3");
  fprintf(stderr,"P3>%s<\n",result);
  ascfree(result);

  result = Asc_GetEnv("P4");
  fprintf(stderr,"P4>%s<\n",result);
  ascfree(result);

  result = Asc_GetEnv("P1");
  fprintf(stderr,">%s<\n",result);
  ascfree(result);

  result = Asc_GetEnv("P2");
  if (result != NULL) {
    fprintf(stderr,"Asc_GetEnv p2 broken\n");
  }
  result = Asc_GetEnv("P1");
  fprintf(stderr,">%s<\n",result);
  if (Asc_SetPathList("PR",result)!=0) {
    fprintf(stderr,"Asc_SetPathList broken");
  }
  ascfree(result);
  result = Asc_GetEnv("PR");
  fprintf(stderr,"PR>%s<\n",result);
  ascfree(result);
  argtop = argv  = Asc_GetPathList("P1", &argc);
  if (argv== NULL || argc !=2) {
    fprintf(stderr,"Asc_GetPathList broken\n");
  } else {
    while (*argv != NULL) {
      fprintf(stderr,"element:>%s<\n",argv[0]);
      argv++;
    }
    ascfree(argtop);
  }
  Asc_ImportPathList("PATH");
  result = Asc_GetEnv("PATH");
  fprintf(stderr,">%s<\n",result);
  ascfree(result);
  argtop = argv  = Asc_GetPathList("PATH", &argc);
  if (argv== NULL) {
    fprintf(stderr,"Asc_GetPathList broken 2\n");
  } else {
    while (*argv != NULL) {
      fprintf(stderr,"PATHelement:>%s<\n",argv[0]);
      argv++;
    }
    ascfree(argtop);
  }
  argtop = argv = Asc_EnvNames(&argc);
  while (*argv != NULL) {
    fprintf(stderr,"var:>%s<\n",argv[0]);
    argv++;
  }
  ascfree(argtop);
  Asc_DestroyEnvironment();
  gl_emptyrecycler();
  gl_destroy_pool();
  exit(0);
}

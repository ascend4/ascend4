/*
 *  Unit test functions for ASCEND: utilities/ascEnvVar.c
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include <stdio.h>
#include <utilities/ascConfig.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <utilities/ascEnvVar.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "CUnit/CUnit.h"
#include "test_ascEnvVar.h"

#ifdef __WIN32__
#define SLASH '\\'
#define PATHDIV ';'
#else /* ! __WIN32__ */
#define SLASH '/'
#define PATHDIV ':'
#endif

#define STR_LEN 250

/* compare function for strings used in this test function */
static
int compare_strings(CONST VOIDPTR p1, CONST VOIDPTR p2)
{
  return strcmp((char *)p1, (char *)p2);
}

static void test_ascEnvVar(void)
{
  char str_path[STR_LEN];
  char str_path_ss1[STR_LEN];
  char str_path_ss2[STR_LEN];
  char str_path_ss3[STR_LEN];

  char str_pathbig[MAX_ENV_VAR_LENGTH*3];
  char str_pathbig_ss1[MAX_ENV_VAR_LENGTH*3];
  char str_pathbig_ss2[MAX_ENV_VAR_LENGTH*3];

  int elem_count;
  char **paths;
  struct gl_list_t *name_list;
  int i;
  char test_ext_varname[] = "TEST_ASCENVVAR_EXTERNAL_ENV_VAR";
  char *str_env;
  int i_initialized_lists = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  /* make sure list system is initialized */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }

  /* test Asc_InitEnvironment() */
  CU_TEST(0 == Asc_InitEnvironment(0));       /* init with 0 */
  CU_TEST(1 == Asc_InitEnvironment(0));       /* should not be able to initialize again */
  Asc_DestroyEnvironment();

  CU_TEST(0 == Asc_InitEnvironment(10));      /* init with typical number */
  CU_TEST(1 == Asc_InitEnvironment(10));      /* should not be able to initialize again */
  Asc_DestroyEnvironment();

  /* test Asc_SetPathList() */

  CU_TEST(1 == Asc_SetPathList("envar1", str_path));    /* not initialized */

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  CU_TEST(1 == Asc_SetPathList(NULL, str_path));        /* NULL var */
  CU_TEST(1 == Asc_SetPathList("envar1", NULL));        /* NULL path */
  CU_TEST(1 == Asc_SetPathList("", str_path));          /* empty var */
  CU_TEST(1 == Asc_SetPathList("envar1", ""));          /* empty path */
  CU_TEST(1 == Asc_SetPathList("envar 1", str_path));   /* embedded space in var */

  memset(str_pathbig, '.', MAX_ENV_VAR_LENGTH);
  str_pathbig[MAX_ENV_VAR_LENGTH] = '\0';

  CU_TEST(1 == Asc_SetPathList(str_pathbig, str_path));  /* var too big */

  Asc_DestroyEnvironment();                             /* start clean in case failure cases left vars */
  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  snprintf(str_path, STR_LEN-1, "%s", "ThisStringHasNoSpaces!");

  CU_TEST(0 == Asc_SetPathList("envar1", str_path));    /* single path element */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%s%c%s", "ThisStringHasNoSpaces+",
                                           PATHDIV,
                                           "this one does, huh?");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "ThisStringHasNoSpaces+");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "this one does, huh?");

  CU_TEST(0 == Asc_SetPathList("envar2", str_path));    /* 2 path elements */
  paths = Asc_GetPathList("envar2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%c%s", PATHDIV, "   ThisStringHasNoSpaces!   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "ThisStringHasNoSpaces!");

  CU_TEST(0 == Asc_SetPathList("envar2e1", str_path));  /* 2 elements, 1 empty */
  paths = Asc_GetPathList("envar2e1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%s%c", "< I DO have spaces >      ", PATHDIV);
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< I DO have spaces >");

  CU_TEST(0 == Asc_SetPathList("envar2e2", str_path));  /* 2 elements, other empty */
  paths = Asc_GetPathList("envar2e2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%c", PATHDIV);

  CU_TEST(0 == Asc_SetPathList("envar2e3", str_path));  /* 2 elements, both empty */
  paths = Asc_GetPathList("envar2e3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  snprintf(str_path, STR_LEN-1, "  %c  ", PATHDIV);

  CU_TEST(0 == Asc_SetPathList("envar2e4", str_path));  /* 2 elements, both empty with ws */
  paths = Asc_GetPathList("envar2e4", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%s%c%s%c%s", "< spaces >",
                                              PATHDIV,
                                              "~NoSpaces~  ",
                                              PATHDIV,
                                              "  another one . ~ $   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< spaces >");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "~NoSpaces~");
  snprintf(str_path_ss3, STR_LEN-1, "%s", "another one . ~ $");

  CU_TEST(0 == Asc_SetPathList("envar3", str_path));    /* 3 path elements */
  paths = Asc_GetPathList("envar3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(3 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  CU_TEST(!strcmp(paths[2], str_path_ss3));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%s%c%c%s", "  < spaces >  ",
                                            PATHDIV,
                                            PATHDIV,
                                            "  another one . ~ $   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< spaces >");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "another one . ~ $");

  CU_TEST(0 == Asc_SetPathList("envar3e1", str_path));  /* 3 elements - middle empty */
  paths = Asc_GetPathList("envar3e1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%c%s%c", PATHDIV,
                                          " \t - =ASCEND rocks || \t",
                                          PATHDIV);
  snprintf(str_path_ss1, STR_LEN-1, "%s", "- =ASCEND rocks ||");

  CU_TEST(0 == Asc_SetPathList("envar3e2", str_path));  /* 3 elements - 1st & 3rd empty */
  paths = Asc_GetPathList("envar3e2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "%c   %c  ", PATHDIV, PATHDIV);

  CU_TEST(0 == Asc_SetPathList("envar3e3", str_path));  /* 3 elements - all empty */
  paths = Asc_GetPathList("envar3e3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  memset(str_pathbig, '.', MAX_ENV_VAR_LENGTH-2);
  str_pathbig[MAX_ENV_VAR_LENGTH-2] = '\0';

  CU_TEST(0 == Asc_SetPathList("envar4", str_pathbig)); /* large string at max length */
  paths = Asc_GetPathList("envar4", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_pathbig));
  ascfree(paths);

  memset(str_pathbig_ss1, '%', MAX_ENV_VAR_LENGTH-1);
  str_pathbig_ss1[MAX_ENV_VAR_LENGTH-1] = '\0';
  memset(str_pathbig_ss2, '@', MAX_ENV_VAR_LENGTH-1);
  str_pathbig_ss2[MAX_ENV_VAR_LENGTH-1] = '\0';

  snprintf(str_pathbig, MAX_ENV_VAR_LENGTH*3, "%s%c%s",
                                              str_pathbig_ss1,
                                              PATHDIV,
                                              str_pathbig_ss2);

  CU_TEST(0 == Asc_SetPathList("envar5", str_pathbig)); /* multiple large strings */
  paths = Asc_GetPathList("envar5", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_pathbig_ss1));
  CU_TEST(!strcmp(paths[1], str_pathbig_ss2));
  ascfree(paths);

  memset(str_pathbig, '`', MAX_ENV_VAR_LENGTH*2);
  str_pathbig[MAX_ENV_VAR_LENGTH*2] = '\0';
  memset(str_pathbig_ss1, '`', MAX_ENV_VAR_LENGTH-1);
  str_pathbig_ss1[MAX_ENV_VAR_LENGTH-1] = '\0';

  CU_TEST(0 == Asc_SetPathList("envar6", str_pathbig)); /* large string over max length */
  paths = Asc_GetPathList("envar6", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_pathbig_ss1));
  ascfree(paths);

  memset(str_pathbig, 'a', MAX_ENV_VAR_LENGTH-2);
  str_pathbig[MAX_ENV_VAR_LENGTH-2] = '\0';

  CU_TEST(0 == Asc_SetPathList(str_pathbig, "\t\teasy\t\t")); /* var name at max length */
  paths = Asc_GetPathList(str_pathbig, &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], "easy"));
  ascfree(paths);

  name_list = gl_create(15);
  CU_TEST_FATAL(NULL != name_list);
  gl_append_ptr(name_list, "envar1");
  gl_append_ptr(name_list, "envar2");
  gl_append_ptr(name_list, "envar2e1");
  gl_append_ptr(name_list, "envar2e2");
  gl_append_ptr(name_list, "envar2e3");
  gl_append_ptr(name_list, "envar2e4");
  gl_append_ptr(name_list, "envar3");
  gl_append_ptr(name_list, "envar3e1");
  gl_append_ptr(name_list, "envar3e2");
  gl_append_ptr(name_list, "envar3e3");
  gl_append_ptr(name_list, "envar4");
  gl_append_ptr(name_list, "envar5");
  gl_append_ptr(name_list, "envar6");
  gl_append_ptr(name_list, str_pathbig);

  paths = Asc_EnvNames(&elem_count);                    /* check the registered names */
  CU_TEST(NULL != paths);
  CU_TEST(14 == elem_count);
  for (i=0 ; i<elem_count ; ++i) {
    if (0 == gl_search(name_list, paths[i], compare_strings)) {
      snprintf(str_pathbig, MAX_ENV_VAR_LENGTH*3, "Environment variable name not registered: %s",
                                                   paths[i]);
      CU_FAIL(str_pathbig);
    }
  }
  ascfree(paths);
  gl_destroy(name_list);

  Asc_DestroyEnvironment();

  /* test Asc_PutEnv() */

  snprintf(str_path, STR_LEN-1, "envar1=This is my string");

  CU_TEST(1 == Asc_PutEnv(str_path));                   /* not initialized */

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  CU_TEST(1 == Asc_PutEnv(NULL));                       /* NULL string */
  CU_TEST(1 == Asc_PutEnv(""));                         /* empty string */
  CU_TEST(1 == Asc_PutEnv("=my_path_is_silly"));        /* empty var name */
  CU_TEST(1 == Asc_PutEnv("envar 1=my_path_is_silly")); /* embedded space in var */
  CU_TEST(1 == Asc_PutEnv("my_path_is_silly"));         /* no '=' */

  memset(str_pathbig, '.', MAX_ENV_VAR_LENGTH-1);
  str_pathbig[MAX_ENV_VAR_LENGTH-1] = '=';
  sprintf(str_pathbig+MAX_ENV_VAR_LENGTH-1, "path");

  CU_TEST(1 == Asc_PutEnv(str_pathbig));                /* var name too big */

  Asc_DestroyEnvironment();                             /* start clean in case failure cases left vars */

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  snprintf(str_path, STR_LEN-1, "%s", "envar1=ThisStringHasNoSpaces!");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "ThisStringHasNoSpaces!");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* single path element */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar2  =\t %s%c%s", "ThisStringHasNoSpaces+",
                                                      PATHDIV,
                                                      "this one does, huh?");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "ThisStringHasNoSpaces+");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "this one does, huh?");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 2 path elements */
  paths = Asc_GetPathList("envar2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "  envar2e1  = %c%s", PATHDIV, "   ThisStringHasNoSpaces!   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "ThisStringHasNoSpaces!");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 2 elements, 1 empty */
  paths = Asc_GetPathList("envar2e1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar2e2= %s%c ", "< I DO have spaces >      ", PATHDIV);
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< I DO have spaces >");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 2 elements, other empty */
  paths = Asc_GetPathList("envar2e2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar2e3=%c", PATHDIV);

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 2 elements, both empty */
  paths = Asc_GetPathList("envar2e3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  snprintf(str_path, STR_LEN-1, "\t envar2e4 =   %c  ", PATHDIV);

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 2 elements, both empty with ws */
  paths = Asc_GetPathList("envar2e4", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar3=%s%c%s%c%s", "< spaces >",
                                                     PATHDIV,
                                                     "~NoSpaces~  ",
                                                     PATHDIV,
                                                     "  another one . ~ $   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< spaces >");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "~NoSpaces~");
  snprintf(str_path_ss3, STR_LEN-1, "%s", "another one . ~ $");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 3 path elements */
  paths = Asc_GetPathList("envar3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(3 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  CU_TEST(!strcmp(paths[2], str_path_ss3));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar3e1\t=%s%c%c%s", "  < spaces >  ",
                                                       PATHDIV,
                                                       PATHDIV,
                                                       "  another one . ~ $   ");
  snprintf(str_path_ss1, STR_LEN-1, "%s", "< spaces >");
  snprintf(str_path_ss2, STR_LEN-1, "%s", "another one . ~ $");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 3 elements - middle empty */
  paths = Asc_GetPathList("envar3e1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss2));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar3e2=%c%s%c", PATHDIV,
                                                   " \t - =ASCEND rocks || \t",
                                                   PATHDIV);
  snprintf(str_path_ss1, STR_LEN-1, "%s", "- =ASCEND rocks ||");

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 3 elements - 1st & 3rd empty */
  paths = Asc_GetPathList("envar3e2", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  snprintf(str_path, STR_LEN-1, "envar3e3=%c   %c  ", PATHDIV, PATHDIV);

  CU_TEST(0 == Asc_PutEnv(str_path));                   /* 3 elements - all empty */
  paths = Asc_GetPathList("envar3e3", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(paths[0] == '\0');
  CU_TEST(0 == elem_count);
  if (NULL != paths) ascfree(paths);

  memset(str_pathbig_ss1, '.', MAX_ENV_VAR_LENGTH-9);
  str_pathbig_ss1[MAX_ENV_VAR_LENGTH-9] = '\0';
  snprintf(str_pathbig, MAX_ENV_VAR_LENGTH-1, "envar4=%s", str_pathbig_ss1);

  CU_TEST(0 == Asc_PutEnv(str_pathbig));                /* large string at max length */
  paths = Asc_GetPathList("envar4", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_pathbig_ss1));
  ascfree(paths);

  memset(str_pathbig_ss1, 'a', MAX_ENV_VAR_LENGTH-3);
  str_pathbig_ss1[MAX_ENV_VAR_LENGTH-3] = '\0';
  snprintf(str_pathbig, MAX_ENV_VAR_LENGTH, "%s=1", str_pathbig_ss1);

  CU_TEST(0 == Asc_PutEnv(str_pathbig));                /* large string at max length */
  paths = Asc_GetPathList(str_pathbig_ss1, &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], "1"));
  ascfree(paths);

  name_list = gl_create(15);
  CU_TEST_FATAL(NULL != name_list);
  gl_append_ptr(name_list, "envar1");
  gl_append_ptr(name_list, "envar2");
  gl_append_ptr(name_list, "envar2e1");
  gl_append_ptr(name_list, "envar2e2");
  gl_append_ptr(name_list, "envar2e3");
  gl_append_ptr(name_list, "envar2e4");
  gl_append_ptr(name_list, "envar3");
  gl_append_ptr(name_list, "envar3e1");
  gl_append_ptr(name_list, "envar3e2");
  gl_append_ptr(name_list, "envar3e3");
  gl_append_ptr(name_list, "envar4");
  gl_append_ptr(name_list, str_pathbig_ss1);

  paths = Asc_EnvNames(&elem_count);                    /* check the registered names */
  CU_TEST(NULL != paths);
  CU_TEST(12 == elem_count);
  for (i=0 ; i<elem_count ; ++i) {
    if (0 == gl_search(name_list, paths[i], compare_strings)) {
      snprintf(str_pathbig, MAX_ENV_VAR_LENGTH*3, "Environment variable name not registered: %s",
                                                   paths[i]);
      CU_FAIL(str_pathbig);
    }
  }
  ascfree(paths);
  gl_destroy(name_list);

  Asc_DestroyEnvironment();                             /* clean up */

  /* test Asc_ImportPathList() */

  CU_TEST(1 == Asc_ImportPathList("envar1"));           /* not initialized */

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  CU_TEST(1 == Asc_ImportPathList(NULL));               /* NULL varname */
  CU_TEST(1 == Asc_ImportPathList(""));                 /* empty varname */

  Asc_DestroyEnvironment();                             /* start with a clean list */
  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  if (NULL != getenv(test_ext_varname)) {               /* make sure our external env var doesn't exist */
    snprintf(str_path, STR_LEN-1,
             "External environment variable '%s' already exists.  Aborting test of Asc_ImportPathList().",
             test_ext_varname);
    str_path[STR_LEN-1] = '\0';
    CU_FAIL(str_path);
  }
  else {

    CU_TEST(1 == Asc_ImportPathList(test_ext_varname)); /* non-existent external env var */

    snprintf(str_path_ss1, STR_LEN-1, "%s", "iHaveNoSpaces");
    snprintf(str_path, STR_LEN-1, "%s=%s", test_ext_varname, str_path_ss1);
    CU_TEST_FATAL(0 == putenv(str_path));

    CU_TEST(0 == Asc_ImportPathList(test_ext_varname)); /* import single path env var */
    paths = Asc_GetPathList(test_ext_varname, &elem_count);
    CU_TEST_FATAL(NULL != paths);
    CU_TEST_FATAL(1 == elem_count);
    CU_TEST(!strcmp(paths[0], str_path_ss1));
    if (NULL != paths) ascfree(paths);

    snprintf(str_path_ss1, STR_LEN-1, "%s", "~This one has spaces~");
    snprintf(str_path_ss2, STR_LEN-1, "%s", "<>NotMe<>");
    snprintf(str_path, STR_LEN-1, "%s=%s%c%s",
                                  test_ext_varname,
                                  str_path_ss1,
                                  PATHDIV,
                                  str_path_ss2);
    CU_TEST_FATAL(0 == putenv(str_path));

    CU_TEST(0 == Asc_ImportPathList(test_ext_varname)); /* import double path env var */
    paths = Asc_GetPathList(test_ext_varname, &elem_count);
    CU_TEST_FATAL(NULL != paths);
    CU_TEST_FATAL(2 == elem_count);
    CU_TEST(!strcmp(paths[0], str_path_ss1));
    CU_TEST(!strcmp(paths[1], str_path_ss2));
    if (NULL != paths) ascfree(paths);

    snprintf(str_path_ss1, STR_LEN-1, "%cusr%clocal%clib", SLASH, SLASH, SLASH);
    snprintf(str_path_ss2, STR_LEN-1, "%cc%cWindows%cTemp", SLASH, SLASH, SLASH);
    snprintf(str_path_ss3, STR_LEN-1, "server%c%caccount%csubfolder", SLASH, SLASH, SLASH);
    snprintf(str_path, STR_LEN-1, "%s=%s%c%s%c%s",
                                  test_ext_varname,
                                  str_path_ss1,
                                  PATHDIV,
                                  str_path_ss2,
                                  PATHDIV,
                                  str_path_ss3);
    CU_TEST_FATAL(0 == putenv(str_path));

    CU_TEST(0 == Asc_ImportPathList(test_ext_varname)); /* import double path env var */
    paths = Asc_GetPathList(test_ext_varname, &elem_count);
    CU_TEST_FATAL(NULL != paths);
    CU_TEST_FATAL(3 == elem_count);
    CU_TEST(!strcmp(paths[0], str_path_ss1));
    CU_TEST(!strcmp(paths[1], str_path_ss2));
    CU_TEST(!strcmp(paths[2], str_path_ss3));
    if (NULL != paths) ascfree(paths);
  }

  snprintf(str_path, STR_LEN-1, "%s=", test_ext_varname); /* clear the temporary external env var */
  CU_TEST(0 == putenv(str_path));
  Asc_DestroyEnvironment();                             /* clean up */

  /* test Asc_AppendPath() */

  CU_TEST(1 == Asc_AppendPath("envar1", str_path));     /* not initialized */

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  CU_TEST(1 == Asc_AppendPath(NULL, str_path));         /* NULL var */
  CU_TEST(1 == Asc_AppendPath("envar1", NULL));         /* NULL path */
  CU_TEST(1 == Asc_AppendPath("", str_path));           /* empty var */
  CU_TEST(1 == Asc_AppendPath("envar1", ""));           /* empty path */

  Asc_DestroyEnvironment();                             /* start clean in case failure cases left vars */
  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  snprintf(str_path_ss1, STR_LEN-1, "path/with\\many;unusual:chars");
  snprintf(str_path_ss2, STR_LEN-1, "  What should I test next?   ");
  snprintf(str_path_ss3, STR_LEN-1, "T");

  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss1)); /* append a path to a new variable */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  ascfree(paths);

  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss1)); /* append a path 2nd time */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss1));
  ascfree(paths);

  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss2)); /* append a new path */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(3 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss1));
  CU_TEST(!strcmp(paths[2], str_path_ss2));
  ascfree(paths);

  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss1)); /* append a path a 3rd time */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(4 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss1));
  CU_TEST(!strcmp(paths[2], str_path_ss2));
  CU_TEST(!strcmp(paths[3], str_path_ss1));
  ascfree(paths);

  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss3)); /* append a new path */
  paths = Asc_GetPathList("envar1", &elem_count);
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(5 == elem_count);
  CU_TEST(!strcmp(paths[0], str_path_ss1));
  CU_TEST(!strcmp(paths[1], str_path_ss1));
  CU_TEST(!strcmp(paths[2], str_path_ss2));
  CU_TEST(!strcmp(paths[3], str_path_ss1));
  CU_TEST(!strcmp(paths[4], str_path_ss3));
  ascfree(paths);

  Asc_DestroyEnvironment();                             /* clean up */

  /* test Asc_GetPathList() - test unusual cases - main use covered in other tests */

  paths = Asc_GetPathList("envar1", &elem_count);       /* not initialized */
  CU_TEST(-1 == elem_count);
  CU_TEST(NULL == paths);

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  paths = Asc_GetPathList(NULL, &elem_count);           /* NULL var */
  CU_TEST(-1 == elem_count);
  CU_TEST(NULL == paths);

  paths = Asc_GetPathList("envar1", NULL);              /* NULL argcPtr */
  CU_TEST(-1 == elem_count);
  CU_TEST(NULL == paths);

  paths = Asc_GetPathList("", &elem_count);             /* empty var */
  CU_TEST(0 == elem_count);
  CU_TEST(NULL == paths);

  paths = Asc_GetPathList("envar1", &elem_count);       /* non-existent var */
  CU_TEST(0 == elem_count);
  CU_TEST(NULL == paths);

  Asc_DestroyEnvironment();                             /* start clean in case failure cases left vars */

  /* test Asc_GetEnv() */

  str_env = Asc_GetEnv("envar1");                       /* not initialized */
  CU_TEST(NULL == str_env);

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  str_env = Asc_GetEnv(NULL);                           /* NULL var */
  CU_TEST(NULL == str_env);

  str_env = Asc_GetEnv("");                             /* empty var */
  CU_TEST(NULL == str_env);

  str_env = Asc_GetEnv("envar1");                       /* non-existent var */
  CU_TEST(NULL == str_env);

  snprintf(str_path_ss1, STR_LEN-1, "path/with\\many;unusual:chars");
  snprintf(str_path_ss2, STR_LEN-1, "  What should I test next?   ");
  snprintf(str_path_ss3, STR_LEN-1, "T");
  snprintf(str_path, STR_LEN-1, "%s%c%s%c%s", str_path_ss1, PATHDIV, str_path_ss2, PATHDIV, str_path_ss3);

  CU_TEST(0 == Asc_SetPathList("envar1", str_path_ss1));
  str_env = Asc_GetEnv("envar1");
  CU_TEST_FATAL(NULL != str_env);
  CU_TEST(!strcmp(str_env, str_path_ss1));
  ascfree(str_env);

  CU_TEST(0 == Asc_SetPathList("envar1", str_path_ss3));
  str_env = Asc_GetEnv("envar1");
  CU_TEST_FATAL(NULL != str_env);
  CU_TEST(!strcmp(str_env, str_path_ss3));
  ascfree(str_env);

  CU_TEST(0 == Asc_SetPathList("envar1", str_path_ss1));
  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss2));
  CU_TEST(0 == Asc_AppendPath("envar1", str_path_ss3));
  str_env = Asc_GetEnv("envar1");
  CU_TEST_FATAL(NULL != str_env);
  CU_TEST(!strcmp(str_env, str_path));
  ascfree(str_env);

  Asc_DestroyEnvironment();                             /* clean up */

  /* test Asc_EnvNames() */

  paths = Asc_EnvNames(&elem_count);                    /* not initialized */
  CU_TEST(NULL == paths);
  CU_TEST(-1 == elem_count);

  CU_TEST(0 == Asc_InitEnvironment(10));                /* init with typical number */

  paths = Asc_EnvNames(&elem_count);                    /* empty environment list */
  CU_TEST_FATAL(NULL != paths);
  CU_TEST(0 == elem_count);
  ascfree(paths);

  Asc_SetPathList("envar1", "my val 1");
  paths = Asc_EnvNames(&elem_count);                    /* 1 environment var */
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(1 == elem_count);
  CU_TEST(!strcmp(paths[0], "envar1"));
  ascfree(paths);

  Asc_SetPathList("envar2", "my val 2");
  paths = Asc_EnvNames(&elem_count);                    /* 2 environment var */
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(2 == elem_count);
  CU_TEST(!strcmp(paths[0], "envar1"));
  CU_TEST(!strcmp(paths[1], "envar2"));
  ascfree(paths);                                                                   

  Asc_SetPathList("envar3", "my val 3");
  paths = Asc_EnvNames(&elem_count);                    /* 2 environment var */
  CU_TEST_FATAL(NULL != paths);
  CU_TEST_FATAL(3 == elem_count);
  CU_TEST(!strcmp(paths[0], "envar1"));
  CU_TEST(!strcmp(paths[1], "envar2"));
  CU_TEST(!strcmp(paths[2], "envar3"));
  ascfree(paths);

  Asc_DestroyEnvironment();                             /* start with a clean list */

  if (TRUE == i_initialized_lists) {
    gl_destroy_pool();
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascEnvVar_test_list[] = {
  {"test_ascEnvVar", test_ascEnvVar},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_ascEnvVar", NULL, NULL, ascEnvVar_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascEnvVar(void)
{
  return CU_register_suites(suites);
}

/*
This program tests the signal reseting of compilers using ctrl-c.

Compile -DINTERACTIVE will ask the user for input and
expect the user to provide ^C. It should allow 3 and exit on the 4th.
But this is not useful for configure.

Compile with no flags yields a binary which returns 0 if
no special treatment of signal code is needed and nonzero
if -DRESETTRAPSIGNALS is required to build ASCEND.
If it returns nonzero, recompile -DRESETTRAPSIGNALS 
and retest just to be sure that signals will behave.

So far the following seem to need -DRESETTRAPSIGNALS
HPUX cc -Aa -D_HPUX_SOURCE 
Solaris cc
AIX xlc
IRIX cc

The following retain the last trap set with or without a call to longjmp
and so don't need the flag.
SunOS4 acc 
OSF32 cc
NetBSD gcc 2.4.5 -ansi

The symbol RESETTRAPSIGNALS might be better abbreviated to
something like RTSIGS.
*/

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

jmp_buf g_slv_int_env;

void (*fault_set_int_trap(void (*trapptr)(int)))(int)
{
  void (*lasttrap)(int);
  if ((lasttrap=signal(SIGINT,trapptr))==SIG_ERR) {
    fprintf(stderr,"Interrupt trap set failed!\n");
    fflush(stderr);
  }
  return lasttrap;
}

static void testctrlc(int signum)
{
  fprintf(stdout,"Ctrl-C caught\n");
  longjmp(g_slv_int_env,SIGINT);
}


#ifdef INTERACTIVE

static
int testdooley()
{
  char inline[200];
  int i;
  fprintf(stdout,"input");
  kill(getpid(),SIGINT);
  fscanf(stdin,"%s",inline);
  return strlen(inline);
}

int main() {
  static int c=0;

  fault_set_int_trap(testctrlc);
  for (;c<4;) {
    if (setjmp(g_slv_int_env)==0) {
      testdooley();
    } else {
#if ( defined(_HPUX_SOURCE) || \
      defined(_AIX) || \
      (defined(__SVR4) && defined(sun)))
      fault_set_int_trap(testctrlc);
      /* this is the tricky part that needs consideration */
#endif
      c++;
    }
  }
  return 0;
}

#else /* interactive */

static
int testdooley2()
{
  int i;
  fprintf(stdout,"input");
  kill(getpid(),SIGINT);
  return 0;
}
int main() {
  static int c=0;

  fault_set_int_trap(testctrlc);
  for (;c<4;) {
    if (setjmp(g_slv_int_env)==0) {
      testdooley2();
    } else {
#if RESETTRAPSIGNALS
      fault_set_int_trap(testctrlc);
#endif
      c++;
    }
  }
  if (c==4) c=0;
  return c;
}
#endif /* !interactive */

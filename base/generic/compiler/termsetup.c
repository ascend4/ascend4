/*
 *  Interface Implementation - terminal setup
 *  Tom Epperly
 *  Created: 1/17/90
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: termsetup.c,v $
 *  Date last modified: $Date: 1997/07/18 12:35:28 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include "utilities\ascConfig.h"
 
#ifndef __WIN32__

#include<unistd.h>
#ifndef linux
#include<sgtty.h>
#endif
#ifdef linux
#include<bsd/sgtty.h>
#endif
#if defined(sun) || defined(__sun) /* sun */
#include<curses.h>
#if defined(__SVR4) /* solaris */
#include<term.h>
#endif /*solaris */
#endif /* sun */
#include<sys/ioctl.h>

#ifdef _SGI_SOURCE
#define _HPUX_SOURCE 1
#include <sys/ttold.h>
#define RAW O_RAW
#define CBREAK O_CBREAK
#endif

#define TERMBUF 1024

/* if the term is unknown, then try to use the following as
   the terminal type before exiting */
#define UNKNOWN_TERM_DEFAULT "dumb"

/* global variables */
struct sgttyb g_old_terminal_settings,g_new_terminal_settings;
unsigned g_terminal_inited=0;
int g_filenum=0;
char code_buf[TERMBUF],termcap_buffer[2048];
char *g_backspace=NULL,*g_clear=NULL,*g_clear_eol=NULL,*g_bell=NULL;


int OutputChar(c)
     char c;
{
  return putchar(c);
}

void DeleteBackOne()
{
  tputs(g_backspace,1,OutputChar);
  putchar(' ');
  tputs(g_backspace,1,OutputChar);
}

void ClearScreen()
{
  tputs(g_clear,1,OutputChar);
}

void Bell()
{
  tputs(g_bell,1,OutputChar);
}

void ClearLine()
{
  putchar('\r');
  tputs(g_clear_eol,1,OutputChar);
}

void SetupTermcapStuff()
{
#ifndef __STDC__
  int tgetent();
  int tgetnum();
  int tgetflag();
  char *tgetstr();
#else
  int tgetent(char *, char *);
  int tgetnum(char *);
  int tgetflag(char *);
  char *tgetstr(char *, char **);
#endif
  char *buffer = code_buf;

  switch(tgetent(termcap_buffer,getenv("TERM"))){
  case -1:
    fprintf(stderr,"Unable to open termcap file.\n");
    exit(2);/*NOTREACHED*/
  case 0:
    fprintf(stderr,"There is no definition for %s in the termcap.\n",
	    getenv("TERM"));
    /* try to use the default term type */
    if (tgetent(termcap_buffer,UNKNOWN_TERM_DEFAULT) == 1) {
      /* successful...set the TERM envar to this and tell user */
      putenv("TERM=" UNKNOWN_TERM_DEFAULT);
      fprintf(stderr,"Using \"%s\" terminal type.\n\n", UNKNOWN_TERM_DEFAULT);
    }
    else {
      /* the alternate term type failed also */
      exit(2); /*NOTREACHED*/
    }
  }
  /* bell */
  if (!(g_bell = tgetstr("bl",&buffer)))
    if (!(g_bell = tgetstr("vb",&buffer))){
      fprintf(stderr,"Terminal doesn't support a bell.\n");
      fprintf(stderr,"Continuing on without bell.\n");
      g_bell = "\007";
    }
  assert(buffer < (code_buf + TERMBUF));
  /* backspacing */
  if (tgetflag("bs")){
    g_backspace = "\010";
  }
  else{
    if (!(g_backspace = tgetstr("bc",&buffer))){
      fprintf(stderr,"Terminal doesn't support backspacing.\n");
      fprintf(stderr,"ASCEND needs backspacking.\n");
      /*exit(2) NOTREACHED*/
    }
  }
  assert(buffer < (code_buf + TERMBUF));
  /* clear to end of line */
  if (!(g_clear_eol = tgetstr("ce",&buffer))){
    fprintf(stderr,"Terminal doesn't support clearing to end of line.\n");
    fprintf(stderr,"ASCEND needs clearing to end of line.\n");
    /*exit(2) NOTREACHED*/
  }
  assert(buffer < (code_buf + TERMBUF));
  /* clear screen */
  if (!(g_clear = tgetstr("cl",&buffer))){
    fprintf(stderr,"Terminal doesn't support clear screen.\n");
    fprintf(stderr,"Continuing on without screen clearing.\n");
  }
  assert(buffer < (code_buf + TERMBUF));
}

void InterfaceError()
{
  fprintf(stderr,"Error changing terminal characteristics\n");
  /* exit(2); NOTREACHED*/
}


#ifdef _HPUX_SOURCE
#define CBREAK         8
# ifndef TIOCGETP
#   define TIOCGETP        _IOR('t', 8,struct sgttyb)
# endif
#define TIOCSETN        _IOW('t', 9,struct sgttyb)
#endif

void SetupTerminal()
{
#ifndef _HPUX_SOURCE
  g_filenum = fileno(stdin);
  if (ioctl(g_filenum,TIOCGETP,&g_old_terminal_settings)==-1) InterfaceError();
  g_new_terminal_settings = g_old_terminal_settings;
  g_new_terminal_settings.sg_flags =
    (g_new_terminal_settings.sg_flags|CBREAK)&(~ECHO);
  if (ioctl(g_filenum,TIOCSETN,&g_new_terminal_settings)==-1) InterfaceError();
  g_terminal_inited = 1;
#endif
}

void RestoreTerminal()
{
#ifndef _HPUX_SOURCE
  if (g_terminal_inited){
    if (ioctl(g_filenum,TIOCSETN,&g_old_terminal_settings)==-1)
      InterfaceError();
    g_terminal_inited = 0;
  }
#endif
}


void TermSetup_ResetTerminal()
{
#ifndef _HPUX_SOURCE
  if (g_terminal_inited){
    if (ioctl(g_filenum,TIOCSETN,&g_new_terminal_settings)==-1)
      InterfaceError();
  }
  else
    SetupTerminal();
  ClearScreen();
#endif  /* ! _HPUX_SOURCE */
}

void ReadString(str,len)
     char *str;
     int *len;
{
  struct sgttyb old,new;
  int filenum;
  filenum = fileno(stdin);
  if (ioctl(filenum,TIOCGETP,&old)==-1) InterfaceError();
  new = old;
  new.sg_flags = (new.sg_flags|ECHO)&(~CBREAK)&(~RAW);
  if (ioctl(filenum,TIOCSETN,&new)==-1) InterfaceError();
  str = gets(str);
  *len = strlen(str);
  if (ioctl(filenum,TIOCSETN,&old)==-1) InterfaceError();
}

#else   /* __WIN32__ */

int OutputChar(char c)
{
  fprintf(stderr,"OutputChar() not implemented in Windows.\n");
  return 0;
}

void DeleteBackOne(void)
{
  fprintf(stderr,"DeleteBackOne() not implemented in Windows.\n");
}

void ClearScreen(void)
{
  fprintf(stderr,"ClearScreen() not implemented in Windows.\n");
}

void Bell(void)
{
  fprintf(stderr,"Bell() not implemented in Windows.\n");
}

void ClearLine(void)
{
  fprintf(stderr,"ClearLine() not implemented in Windows.\n");
}

void SetupTermcapStuff(void)
{
  fprintf(stderr,"SetupTermcapStuff() not implemented in Windows.\n");
}

void InterfaceError(void)
{
  fprintf(stderr,"InterfaceError() not implemented in Windows.\n");
}

void SetupTerminal(void)
{
  fprintf(stderr,"SetupTerminal() not implemented in Windows.\n");
}

void RestoreTerminal(void)
{
  fprintf(stderr,"RestoreTerminal() not implemented in Windows.\n");
}

void TermSetup_ResetTerminal(void)
{
  fprintf(stderr,"TermSetup_ResetTerminal() not implemented in Windows.\n");
}

void ReadString(char *str, int *len)
{
  fprintf(stderr,"ReadString() not implemented in Windows.\n");
}


#endif  /* __WIN32__ */


#ifndef OutputImpl_h_seen
/**
 *  This file is part of the SLV-C++ solver.
 *
 *  Copyright (C) 2000 Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 *
 *  Contents:     C++ output default implementation.
 *  Authors:      Ben Allan
 *  Dates:        08/2000 - original version
 */
#define OutputImpl_h_seen
/** virtual interface definition for output channels. */
class OutputImpl {
public:
  OutputImpl() {
    out_ = err_ = log_ = dbg_ = 0;
  }

  OutputImpl(FILE *outf, FILE *logf, FILE *errf, FILE *dbgf) {
    out_  = outf;
    err_  = errf;
    log_  = logf;
    dbg_  = dbgf;
  }

  virtual ~OutputImpl() {
    out_ = err_ = log_ = dbg_ = 0;
  }

  /** Send high priority message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void out(char *s) { write(out_,s); }

  /** Send low priority message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void log(char *s) { write(log_,s); }

  /** Send a warning/error message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void err(char *s) { write(err_,s); }

  /** Send a debugging message about variable to the developer
      by hook or by crook. This may be any file, UI, or even NULL output. */
  virtual void debug(char *s) { write(dbg_,s); }

  /** Set up the file pointer out. */
  void setOutFile( FILE * out){
    out_ = out;
  }
  void setLogFile( FILE * log) {
    log_ = log;
  }
  void setErrFile( FILE * err) {
    err_ = err;
  }
  void setDebugFile( FILE * dbg) {
    dbg_ = dbg;
  }

  /** Get the file pointer out. */
  void setOutFile( FILE * & out){
    out = out_;
  }
  void setLogFile( FILE * & log) {
    log = log_;
  }
  void setErrFile( FILE * & err) {
    err = err_;
  }
  void setDebugFile( FILE * & dbg) {
    dbg = dbg_;
  }

private:
  FILE *out_;
  FILE *log_;
  FILE *err_;
  FILE *dbg_;

  void write(FILE *f, char *s) {
    if (f != 0) {
      fprintf(f,"%s",s);
    }
  }

};
#endif // OutputImpl_h_seen

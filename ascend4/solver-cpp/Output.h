#ifndef Output_h_seen
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
 *  Contents:     C++ output interface definition.
 *  Authors:      Ben Allan
 *  Dates:        08/2000 - original version
 */
#define Output_h_seen
/** virtual interface definition for output channels. */
class Output {
public:

  virtual ~Output() {}

  /** Send high priority message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void out(char *) = 0;

  /** Send low priority message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void log(char *) = 0;

  /** Send a warning/error message about variable to the user
      via the model server. This may be any file, UI, or even NULL output. */
  virtual void err(char *) = 0;

  /** Send a debugging message about variable to the developer
      by hook or by crook. This may be any file, UI, or even NULL output. */
  virtual void debug(char *) = 0;

};
#endif // Output_h_seen

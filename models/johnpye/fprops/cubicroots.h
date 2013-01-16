/* 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007, 2009 Brian Gough
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
	Calculate roots of a cubic, based on a function from GSL.
	x³ + ax² + bx + c = 0 
	@return the number of real roots (1 or 3)
	@param x0 first real root (smallest)
	@param x1 second real root (middle one), iff three exist
	@param x2 third real root (highest), iff three exist
*/
int  cubicroots(double a, double b, double c, double *x0, double *x1, double *x2);


/* 
 * Copyright (C) 2022 John Pye
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
	@return the number of real roots (can be 1 or 3).
	@param x[3] array of up to three roots, smallest to largest
	Note that x[1] and x[2] will only be written to if 3 real roots are found.
	For details of the method, see https://ascend4.org/Solving_cubic_polynomials
*/
int cubicroots(double a, double b, double c, double x[3]);


// CharactersInDouble
// This code is not protected by copyright and is in the public domain.
// Author: Meritt Reynolds

#include "CharactersInDouble.h"
#include <string.h>
#include <ctype.h>

// CharactersInDouble
//
//	Parse a string and return the number of characters making up the representation 
//	of a floating point number.  No conversion is done here, the goal is to identify 
//	the existence and length of the number.  The conversion is best done by system
//	routines like atof, sscanf, etc.
//
//	Returns 0 if there is no valid number present.
//
//	Does not skip white space so first character must be part of the number.  If you
//	don't care about white space then skip it before calling CharactersInDouble.
//
//	Example:
//
//		while (isspace(p)) p++;
//		n = CharactersInDouble(p);
//		if (n)
//		{
//			x = atof(p);
//			p += n;		// move to unparsed part of string
//		}
//
//	HOW IT WORKS
//
//	A finite state machine with eight states is used to parse numbers that might 
//	be in scientific notation.  Exponent can be indicated by 'e' or 'E'.
//	Might want to generalize to 'd' and 'D' as well so old FORTRAN output
//	doesn't cause problems.
//
//	Valid numbers:
//		1			integer is a double
//		-1			negative numbers OK
//		+1			explicit + sign OK
//		1.			trailing . OK
//		.1			leading . OK
//		1e2			exponent without . before
//		1e+2		exponent with explicit +
//		1e-2		exponent with -
//		1.e2		exponent with . before
//		1.1e1		full mantissa
//		1.34e+4		general test
//
//	There is no "looking forward".  For example, this subroutine does 
//	not forgive trailing 'e'.  If it gets an 'e' it insists on a valid exponent.
//
//	Invalid numbers:
//		" 1"		leading white space
//		-a			no digit after the -
//		a			not a digit
//		1e			e assumed to be part of number, but exponent is missing
//
//	Validly terminated numbers:
//
//		1.7a		a clearly not part of the number  (returns 3)
//		1.e-7e		2nd e clearly not part of the number  (returns 5)
//
//	Thinking of porting this to C# one day, we do not use pointers.

int CharactersInDouble(const char p[])
{
	int len = strlen(p);	// in C# we will use something different
	int bytes = 0;
	int state = 0;
	
	while (1)
	{
		int c, digit;
		
		// Get next character from string.  Treat terminating 0 as
		// a character and let the finite state machine process it.
		
		if (bytes < len)
		{
			c = (int)p[bytes];
		}
		else
		{
			c = 0;
		}
		
		digit = isdigit(c);
		
		if (state == 0)			// start here
		{
			if (c == '+' || c == '-')	state = 1;
			else if (digit)				state = 2;
			else if (c == '.')			state = 3;
			else 						return 0;
		}
		else if (state == 1)	// we had a sign, cannot have another
		{		
			if (c == '.')				state = 4;
			else if (digit)				state = 2;
			else						return 0;
		}
		else if (state == 2)	// had a digit, working on integer part
		{
			if (c == 'e' || c == 'E')	state = 5;
			else if (digit)				state = 2;
			else if (c == '.')			state = 4; 
			else						break;
		}
		else if (state == 3)	// had a leading ., need fractional part
		{
			if (digit)					state = 4;
			else						return 0;
		}
		else if (state == 4)	// working on fractional part
		{
			if (c == 'e' || c == 'E')	state = 5;
			else if (digit)				state = 4;
			else						break;
		}
		else if (state == 5)	// had an 'e', need sign or a digit
		{
			if (c == '+' || c == '-')	state = 6;
			else if (digit)				state = 7;
			else						return 0;
		}
		else if (state == 6)	// had a sign, need a digit
		{
			if (digit)					state = 7;
			else						return 0;
		}
		else if (state == 7)	// working on the exponent
		{
			if (digit)					state = 7;
			else						break;
		}
		
		// Here the character has been accepted.

		bytes++;
	}
	
	return bytes;
}

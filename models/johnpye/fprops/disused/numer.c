/*
 * numer.c
 * This file is part of ASCEND
 *
 * Copyright (C) 2011 - Carnegie Mellon University
 *
 * ASCEND is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ASCEND is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASCEND; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "numer.h"

ComplexRoots solve_cubic(const CubicCoeffs poly){
    ComplexRoots roots;
    double a=poly.a, b=poly.b, c=poly.c, d=poly.d, determinant;
    complex double Q, C, thing;
    determinant=18*a*b*c*d-4*b*b*b*d+b*b*c*c-4*a*c*c*c-27*a*a*d*d;
    thing=b*b-3*a*c;
    Q=csqrt(pow(2*b*b*b-9*a*b*c+27*a*a*d,2)-4*pow(thing,3));
    C=cpow(0.5*(Q+2*b*b*b-9*a*b*c+27*a*a*d),1.0/3.0);
    if(cimag(Q)!=0){
        //Equation has THREE REAL ROOTS
        //TODO: Account for the problematic cases...
        roots.r1=-b/(3*a)-C/(3*a)-thing/(3*a*C);
        roots.r2=-b/(3*a)+C*(1+I*sqrt(3))/(6*a)+(1-I*sqrt(3))*thing/(6*a*C);
        roots.r3=-b/(3*a)+C*(1-I*sqrt(3))/(6*a)+(1+I*sqrt(3))*thing/(6*a*C);
    }
    else{
        //Equation has ONE REAL and TWO COMPLEX CONJUGATE roots
        //TODO: Account for the problematic cases...
        roots.r1=-b/(3*a)-C/(3*a)-thing/(3*a*C);
        roots.r2=-b/(3*a)+C*(1+I*sqrt(3))/(6*a)+(1-I*sqrt(3))*thing/(6*a*C);
        roots.r3=-b/(3*a)+C*(1-I*sqrt(3))/(6*a)+(1+I*sqrt(3))*thing/(6*a*C);
    }
#if 0
    printf("1: %f %+f i\n2: %f %+f i\n3: %f %+f i\n",
           creal(roots.r1), cimag(roots.r1),
           creal(roots.r2), cimag(roots.r2),
           creal(roots.r3), cimag(roots.r3));
#endif
    return roots;
}
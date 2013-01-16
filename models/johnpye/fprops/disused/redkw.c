/*
 * redkw.c
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

 /*
 	Implementation of the Redlich Kwong equation of state.
 	Functions that do not end in _raw take care of which part of the PVT surface we're on,
 	functions ending in _raw do the number crunching to find the property.
 */


#include "dataStructures.h"
#include "numer.h"
#include "redkw.h"
#include "parser.h"
#include "fprops.h"
#include "sat.h"
#include "ideal_impl.h"
#include <math.h>


#ifdef REDKW_TEST
//FIXME: REMOVE GLOBALS!!
double T_ref=298.15;
double p_ref=0.1e6;
double V_ref;
#include <stdio.h>
//Lets do some testing!
int main(int argc, char* argv[]){
    IdealPowTerm oxygen_power_terms[2]={{-0.7151e-5,2},{1.311e-9,3}};
    IdealData ideal_data_oxygen={
        25.46,
        1.519e-2,
        0,
        0,
        2,
        oxygen_power_terms,
        0,
        NULL
    };
	CriticalData critical_data_oxygen={
        154.6,
        0.436e6,
        5.462e6
    };
    ReferenceState reference_point={
        298.15,//T_ref
        0.1e6,//p_ref
        0//????//V_ref
    };
	CubicData cubic_data_oxygen={
        32,
        &critical_data_oxygen,
        0.021,
        &reference_point
    };
    RedKwCoeffs redkw_coeffs_oxygen;
    EosData eos_data_oxygen={.cubicData=&cubic_data_oxygen};
    EosCoeffs eos_coeffs_oxygen={.redKwCoeffs=&redkw_coeffs_oxygen};
    PureFluid fluid_data_oxygen={
        FPROPS_PENGROB,
        &critical_data_oxygen,
        &reference_point,
        &ideal_data_oxygen,
        &eos_data_oxygen,
        &redkw_p,
        &redkw_u_depart,
        &redkw_h_depart,
        &redkw_s_depart,
        &redkw_a_depart,
        &redkw_cv,
        &redkw_cp,
        &redkw_w,
        &redkw_g_depart
    };

    double T, rho, p, p_init, a_dep, s_dep, h_dep, u_dep, g_dep, a, s, h, u, g, Z, V;
    FpropsError err=FPROPS_NO_ERROR;
    T=398.15; p_init=0.1e6; p=p_init;
    redkw_get_coeffs(T, &redkw_coeffs_oxygen, &cubic_data_oxygen, &err);

#define CRIT (*(cubic_data_oxygen.critical))

    V_ref=redkw_p(CRIT.T/T_ref, p_ref, &eos_coeffs_oxygen, &eos_data_oxygen, &err);
    V=redkw_Vm(CRIT.T/T, p, &eos_coeffs_oxygen, &eos_data_oxygen, &err);
    Z=redkw_Z(CRIT.T/T, p, &eos_coeffs_oxygen, &eos_data_oxygen, &err);
    rho=32.0/V;

    p=fprops_p(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);
    a_dep=fprops_a(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);
    s_dep=fprops_s(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);
    h_dep=fprops_h(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);
    u_dep=fprops_u(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);
    g_dep=fprops_g(T, rho, &eos_coeffs_oxygen, &fluid_data_oxygen, &err);

    a=a_dep+helm_ideal(CRIT.T/T, rho/CRIT.rho, &ideal_data_oxygen);
    s=s_dep+(25.46*log(T/298.15)+1.519e-2*(T-298.15)-0.7151e-5/2*(T*T-\
            298.15*298.15)+1.311e-9/3*(T*T*T-298.15*298.15*298.15)-R_UNIVERSAL*log(p/1e5));
    h=h_dep+(25.46*(T-298.15)+1.519e-2/2*(T*T-298.15*298.15)-0.7151e-5/3\
             *(T*T*T-298.15*298.15*298.15)+1.311e-9/4*(T*T*T*T-298.15*298.15*298.15*298.15));
    u=u_dep+0;
    g=g_dep+0;

#undef CRIT

    printf("Redlich Kwong Equation of State\n"
           "Temperature: %g (K) Initial Pressure: %g (Pa)\n"
           "Compressibility Factor: %g\nMolar Volume: %g (m^3/kmol)\nPressure: %g (MPa)\n          %g (bar)\n"
           "Helmholtz: %g (%g%+g)\n"
           "Entropy: %g (%g%+g) (J/mol K)\nEnthlapy: %g (%g%+g) (J/mol)\n"
           "Internal Energy: %g (%g%+g)\nGibbs Energy: %g (%g%+g)\n",
           T,p_init,Z,V*1000,p/1e6, p/1e5, a, a-a_dep, a_dep,s, s-s_dep, s_dep, h, h-h_dep,h_dep, u, u-u_dep, u_dep, g, g-g_dep, g_dep);

    return 0;
}
#endif

/**
 Function to calculate pressure from Helmholtz free energy EOS, given temperature
 and mass density.

 @param tau
 @param delta
 @return pressure in Pa
 TODO: Add error checking using the FPROPS_ERR enum
 */
double redkw_p  (double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData;
    RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    double V=d->M/rho;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);

    double absolutePressure=(R_UNIVERSAL*T)/(V-c->b)-c->a/(V*(V+c->b));
    return absolutePressure;
}

/**
 TODO: Add saturation region calculations...
 */
double redkw_Z  (double tau, double   p, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T;
    double T=T_c/tau;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);

#define R R_UNIVERSAL
    double B=p*c->b/(R*T), A=c->a*p/(R*T*R*T);
#undef R
    //Form the polynomial:
    CubicCoeffs poly={1,-1,A-B*B-B,-A*B};
    //Solve and return (possibly complex) roots:
    ComplexRoots roots=solve_cubic(poly);

    //If we have three real roots then we're in the saturation region:
    if(cimag(roots.r1)==0 && cimag(roots.r2)==0 && cimag(roots.r3)==0){
        //TODO: Saturation stuff here...
        return 0;
    }
    //Otherwise simply return the one real root:
    else{
        if(cimag(roots.r1)==0) return creal(roots.r1);
        else if (cimag(roots.r2)==0) return creal(roots.r2);
        else return creal(roots.r3);
    }
}

double redkw_Vm (double tau, double   p, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T;
    double T=T_c/tau;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    return redkw_Z(tau, p, coeffs, data, err)*R_UNIVERSAL*T/p;
}

double redkw_rho(double tau, double   p, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != crit->T/tau) redkw_refresh_coeffs(crit->T/tau, c, d, err);
    return d->M/redkw_Vm(tau, p, coeffs, data, err);
}

/**
 Do we really need to do this? It'll be tricky...
 */
double redkw_T  (double p, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    return 0;
}


double redkw_a_depart(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    double V=d->M/rho;
    double a_depart=-R_UNIVERSAL*T*log((V-c->b)/V)-c->a/c->b*log((V+c->b)/V)-R_UNIVERSAL*T*log(V/V_ref);
    return a_depart;
}

double redkw_s_depart(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    double V=d->M/rho;
    double s_depart=R_UNIVERSAL*log((V-c->b)/V)-c->a/(2*c->b*T)*log((V+c->b)/V)+R_UNIVERSAL*log(V/V_ref);
    return s_depart;
}

double redkw_h_depart(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    double V=d->M/rho;
    double h_depart=c->b*R_UNIVERSAL*T/(V-c->b)-c->a/(V+c->b)-3*c->a/(2*c->b)*log((V+c->b)/V);
    return h_depart;
}

double redkw_u_depart(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    double V=d->M/rho;
    double u_depart=-3*c->a/(2*c->b)*log((V+c->b)/V);
    return u_depart;
}

double redkw_g_depart(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    CubicData *d=data->cubicData; RedKwCoeffs *c=coeffs->redKwCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) redkw_refresh_coeffs(T, c, d, err);
    double V=d->M/rho;
    double g_depart=c->b*R_UNIVERSAL*T/(V-c->b)-c->a/(V+c->b)-R_UNIVERSAL*T*log((V-c->b)/V)-c->a/c->b*log((V+c->b)/V)-R_UNIVERSAL*T*log(V/V_ref);
    return g_depart;
}


//Haven't worked these out yet:
double redkw_cv(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    return 0;
}

double redkw_cp(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    return 0;
}

double redkw_w(double tau, double delta, EosCoeffs *coeffs, const EosData *data, const FpropsError *err){
    return 0;
}

//Helper functions
//(Call once on loading fluid, temperature can be arbitrary)
void redkw_get_coeffs(double T, RedKwCoeffs *coeffs, const CubicData *data, const FpropsError *err){
    coeffs->b=0.08664*R_UNIVERSAL*data->critical->T/data->critical->p;
    redkw_refresh_coeffs(T, coeffs, data, err);
}

void redkw_refresh_coeffs(double T, RedKwCoeffs *coeffs, const CubicData *data, const FpropsError *err){
    coeffs->a=0.42748*R_UNIVERSAL*R_UNIVERSAL*pow(data->critical->T, 2.5)/(data->critical->p*sqrt(T));
    coeffs->T_last=T;
}
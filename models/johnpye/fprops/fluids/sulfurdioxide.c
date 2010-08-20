/* This file is created by Hongke Zhu, 05-30-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"

#define SULFURDIOXIDE_M 64.0638 /* kg/kmol */
#define SULFURDIOXIDE_R (8314.472/SULFURDIOXIDE_M) /* J/kg/K */
#define SULFURDIOXIDE_TSTAR 430.64 /* K */

const IdealData ideal_data_sulfurdioxide = {
    -4.5328346436 /* constant */
    , 4.4777967379 /* linear */
    , SULFURDIOXIDE_TSTAR /* Tstar */
    , SULFURDIOXIDE_R /* cp0star */
    , 2 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
        ,{0.72453E-04,	1.0}
    }
    , 2 /* exponential terms */
    , (const IdealExpTerm[]){
        {1.0620,	775.0}
        ,{1.9401,	1851.0}
    }
};

const HelmholtzData helmholtz_data_sulfurdioxide = {
	"sulfurdioxide"
    , /* R */ SULFURDIOXIDE_R /* J/kg/K */
    , /* M */ SULFURDIOXIDE_M /* kg/kmol */
    , /* rho_star */ 8.195*SULFURDIOXIDE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ SULFURDIOXIDE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ SULFURDIOXIDE_TSTAR
    , /* rho_c */ 8.195*SULFURDIOXIDE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.2557 /* acentric factor */
    , &ideal_data_sulfurdioxide
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.93061,	0.25,	1.0,	0.0}
        , {-1.9528,	1.25,	1.0,	0.0}
        , {-0.17467,	1.5,	1.0,	0.0}
        , {0.061524,	0.25,	3.0,	0.0}
        , {0.00017711,	0.875,	7.0,	0.0}
        , {0.21615,	2.375,	1.0,	1.0}
        , {0.51353,	2.0,	2.0,	1.0}
        , {0.010419,	2.125,	5.0,	1.0}
        , {-0.25286,	3.5,	1.0,	2.0}
        , {-0.054720,	6.5,	1.0,	2.0}
        , {-0.059856,	4.75,	4.0,	2.0}
        , {-0.016523,	12.5,	2.0,	3.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py sulfurdioxide
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_sulfurdioxide, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_sulfurdioxide, ntd, td);
    return helm_run_test_cases(&helmholtz_data_sulfurdioxide, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-5.0E+1, 1.E-1, 1.55976065755E+3, -5.44936162697E+1, -5.44295038692E+1, -2.24359876616E-1, 8.55052702696E-1, 1.36306167245E+0, 5.80497065192E-1, -4.42770980286E+0}
    , {0.E+0, 1.E-1, 2.89189942162E+0, 3.6129635099E+2, 3.95875700207E+2, 1.50565770404E+0, 5.1560391283E-1, 6.68334570078E-1, 6.08265873198E-1, -4.99740508692E+1}
    , {5.0E+1, 1.E-1, 2.41415595303E+0, 3.87351080805E+2, 4.28773425486E+2, 1.61628083715E+0, 5.17922135872E-1, 6.56840140903E-1, 6.3639527572E-1, -1.34950071719E+2}
    , {1.00E+2, 1.E-1, 2.08002787031E+0, 4.13909135406E+2, 4.619854143E+2, 1.71181192716E+0, 5.3879246579E-1, 6.73445369994E-1, 6.64075196053E-1, -2.24853485215E+2}
    , {1.50E+2, 1.E-1, 1.82945530573E+0, 4.41542185496E+2, 4.96203263952E+2, 1.79783802973E+0, 5.62725049613E-1, 6.95557920933E-1, 6.90360199709E-1, -3.19212976786E+2}
    , {2.00E+2, 1.E-1, 1.6336318679E+0, 4.703294449E+2, 5.31542746357E+2, 1.87675314524E+0, 5.85970659132E-1, 7.17860087813E-1, 7.14577313267E-1, -4.17656305772E+2}
    , {2.50E+2, 1.E-1, 1.47606941765E+0, 5.00216494442E+2, 5.67963985722E+2, 1.94991009988E+0, 6.07345563478E-1, 7.38678516823E-1, 7.36411097179E-1, -5.19878974308E+2}
    , {-5.0E+1, 1.E+0, 1.56069298767E+3, -5.46909833658E+1, -5.40502423566E+1, -2.25245279013E-1, 8.5525958295E-1, 1.36226560685E+0, 5.80497065192E-1, -4.427499354E+0}
    , {0.E+0, 1.E+0, 1.43664658812E+3, 1.33153839087E+1, 1.40114493213E+1, 4.99465425227E-2, 8.15756213017E-1, 1.36833829475E+0, 6.08265873198E-1, -3.27514181316E-1}
    , {5.0E+1, 1.E+0, 1.2946055542E+3, 8.31098739529E+1, 8.38823100024E+1, 2.8467638289E-1, 7.89838929655E-1, 1.44252606731E+0, 6.3639527572E-1, -8.88329917805E+0}
    , {1.00E+2, 1.E+0, 2.23815288465E+1, 4.05161726457E+2, 4.49841426707E+2, 1.38919149578E+0, 5.85618355973E-1, 7.7885687659E-1, 6.64075196053E-1, -1.13215080194E+2}
    , {1.50E+2, 1.E+0, 1.91326304694E+1, 4.35526997708E+2, 4.87793726091E+2, 1.48467720924E+0, 5.83437644095E-1, 7.4852596759E-1, 6.90360199709E-1, -1.92714163383E+2}
    , {2.00E+2, 1.E+0, 1.68248089699E+1, 4.65752701656E+2, 5.25188740531E+2, 1.5682054117E+0, 5.96966054213E-1, 7.49812918412E-1, 7.14577313267E-1, -2.76243688888E+2}
    , {2.50E+2, 1.E+0, 1.50612047109E+1, 4.96523956175E+2, 5.62919707325E+2, 1.64400248879E+0, 6.13952595509E-1, 7.6018308823E-1, 7.36411097179E-1, -3.63535945837E+2}
    , {-5.0E+1, 1.E+1, 1.5697362937E+3, -5.66003051882E+1, -5.0229808411E+1, -2.33891662929E-1, 8.57431887895E-1, 1.35494531521E+0, 5.80497065192E-1, -4.40738060554E+0}
    , {0.E+0, 1.E+1, 1.45065364749E+3, 1.03572299882E+1, 1.72506742072E+1, 3.89834558963E-2, 8.17387839191E-1, 1.35096839926E+0, 6.08265873198E-1, -2.91100989842E-1}
    , {5.0E+1, 1.E+1, 1.31886065979E+3, 7.8092273835E+1, 8.56745759727E+1, 2.68914303225E-1, 7.89762178572E-1, 1.39616275394E+0, 6.3639527572E-1, -8.80738325206E+0}
    , {1.00E+2, 1.0E+1, 1.15844726567E+3, 1.49767692908E+2, 1.58399937376E+2, 4.77926929248E-1, 7.7629629665E-1, 1.53849240128E+0, 6.64075196053E-1, -2.85707407407E+1}
    , {1.50E+2, 1.0E+1, 9.08519314474E+2, 2.35748682969E+2, 2.46755603615E+2, 6.99320946146E-1, 7.91444340066E-1, 2.24424143791E+0, 6.90360199709E-1, -6.01689753927E+1}
    , {2.00E+2, 1.E+1, 2.73152532452E+2, 3.96081236916E+2, 4.32690818787E+2, 1.11549966867E+0, 7.6425932772E-1, 1.90701685647E+0, 7.14577313267E-1, -1.31717431316E+2}
    , {2.50E+2, 1.0E+1, 1.9283594603E+2, 4.51362091591E+2, 5.03219643079E+2, 1.25777526092E+0, 6.92953803241E-1, 1.15725863274E+0, 7.36411097179E-1, -2.0664303616E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif

#include <stdio.h>
#include <math.h>
#include "eqm.h"

static const char *eqm_status_text(int status){
    switch(status){
    case 0:
        return "success";
    case 1:
        return "solved to acceptable level";
    case 2:
        return "IPOPT infeasible problem detected";
    case 3:
        return "IPOPT search direction became too small";
    case 4:
        return "IPOPT diverging iterates";
    case 5:
        return "IPOPT user requested stop";
    case 6:
        return "feasible point found";
    case -1:
        return "IPOPT maximum iterations exceeded";
    case -2:
        return "IPOPT restoration failed";
    case -3:
        return "IPOPT error in step computation";
    case -4:
        return "IPOPT maximum CPU time exceeded";
    case -5:
        return "IPOPT maximum wall time exceeded";
    case -10:
        return "IPOPT not enough degrees of freedom";
    case -11:
        return "FPROPS invalid input, allocation failure, or missing thermo data";
    case -12:
        return "FPROPS unsupported algorithm/path for this problem";
    case -13:
        return "FPROPS numeric failure, IPOPT invalid number, or result rejected by validation";
    case -14:
        return "FPROPS failed to create solver problem or evaluate requested property";
    case -15:
        return "FPROPS unsupported property/model for this calculation";
    case -21:
        return "FPROPS equilibrium package/source preparation failed";
    case -22:
        return "FPROPS solver returned a candidate rejected by validation";
    case -31:
        return "FPROPS nullspace/IPOPT initial-point precheck failed";
    case -100:
        return "unrecoverable IPOPT exception";
    case -101:
        return "non-IPOPT exception thrown";
    case -102:
        return "insufficient memory";
    case -199:
        return "internal IPOPT error";
    case -99:
        return "FPROPS internal dispatch failure";
    default:
        return "unrecognized equilibrium/solver status";
    }
}

int main(void){
    double T = 1173.15;      /* K */
    double P = 101325.0;     /* Pa */

    double n_hematite_in = 1.0;  /* mol Fe2O3 */
    double n_h2_in = 100.0;      /* mol H2 */

    static const char *names[] = {
        "Fe_bcc", "Fe_fcc",
        "Wus_FeO", "Wus_FeO1p5",
        "Sp_Fe2_tet", "Sp_Fe3_tet", "Sp_Fe2_oct", "Sp_Fe3_oct", "Sp_Va_oct",
        "Fe2O3",
        "hydrogen", "water"
    };
    static const char *elements[] = {"Fe", "O", "H"};

    double b[] = {
        2.0 * n_hematite_in,
        3.0 * n_hematite_in,
        2.0 * n_h2_in
    };

    const char *source =
        "Fe_bcc=hidayat_2015;Fe_fcc=hidayat_2015;"
        "Wus_FeO=hidayat_2015;Wus_FeO1p5=hidayat_2015;"
        "Fe2O3=hidayat_2015;"
        "Sp_Fe2_tet=degterov_2001;Sp_Fe3_tet=degterov_2001;"
        "Sp_Fe2_oct=degterov_2001;Sp_Fe3_oct=degterov_2001;Sp_Va_oct=degterov_2001;"
        "hydrogen=helmholtz+ref0:;water=helmholtz+ref0:";

    double n_init[12] = {0};

    n_init[0] = b[0];                              /* Fe_bcc */
    n_init[11] = b[1];                             /* water */
    n_init[10] = 0.5 * b[2] - n_init[11];          /* hydrogen */

    double n[12] = {0};
    int status = eqm_solve_elements(
        names, 12, elements, 3, b,
        source, T, P, "auto", n_init, n
    );

    if(!(status == 0 || status == 1 || status == 6)){
        fprintf(stderr, "equilibrium failed, status=%d (%s)\n", status, eqm_status_text(status));
        for(int i = 0; i < 12; ++i){
            fprintf(stderr, "  %s = %.17g\n", names[i], n[i]);
        }
        return 1;
    }

    double n_fe_metal = n[0] + n[1];
    double n_wus = n[2] + n[3];
    double x_wus = n_wus > 0 ? n[3] / n_wus : NAN;
    double o_per_fe_wus = n_wus > 0 ? 1.0 + 0.5 * x_wus : NAN;

    double n_sp_tet = n[4] + n[5];
    double n_sp_oct = n[6] + n[7] + n[8];
    double n_sp_fu = n_sp_tet; /* spinel formula units, O4 basis */

    printf("Fe metal: %.12g mol (bcc %.12g, fcc %.12g)\n", n_fe_metal, n[0], n[1]);
    printf("hematite Fe2O3: %.12g mol\n", n[9]);

    printf("wustite: %.12g mol, x=%.12g, formula FeO_%.12g\n",
        n_wus, x_wus, o_per_fe_wus);

    if(n_sp_fu > 0){
        double yt2 = n[4] / n_sp_tet;
        double yt3 = n[5] / n_sp_tet;
        double yo2 = n[6] / n_sp_oct;
        double yo3 = n[7] / n_sp_oct;
        double yva = n[8] / n_sp_oct;

        printf("spinel: %.12g formula-units\n", n_sp_fu);
        printf("  tet: Fe2+ %.12g, Fe3+ %.12g\n", yt2, yt3);
        printf("  oct: Fe2+ %.12g, Fe3+ %.12g, Va %.12g\n", yo2, yo3, yva);
        printf("  formula: (Fe2+_%.6g Fe3+_%.6g)[Fe2+_%.6g Fe3+_%.6g Va_%.6g]O4\n",
            yt2, yt3, 2*yo2, 2*yo3, 2*yva);
    }

    printf("gas: H2 %.12g mol, H2O %.12g mol\n", n[10], n[11]);
    return 0;
}

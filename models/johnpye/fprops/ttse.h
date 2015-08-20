
void alloc_tables();
void ttse_prepare(PureFluid *P);
void ttse_destroy(PureFluid *P);

double evaluate_ttse_sat(double T, double *rhof_out, double * rhog_out, PureFluid *P, FpropsError *err);

double evaluate_ttse_p( double t, double rho, Ttse * table);
double evaluate_ttse_h( double t, double rho, Ttse * table);
double evaluate_ttse_s( double t, double rho, Ttse * table);
double evaluate_ttse_u( double t, double rho, Ttse * table);
double evaluate_ttse_g( double t, double rho, Ttse * table);

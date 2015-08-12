
void alloc_tables();
void ttse_prepare(PureFluid *P);

double evaluate_ttse_p( double t, double rho, Ttse * table);
double evaluate_ttse_h( double t, double rho, Ttse * table);
double evaluate_ttse_s( double t, double rho, Ttse * table);
double evaluate_ttse_u( double t, double rho, Ttse * table);
double evaluate_ttse_g( double t, double rho, Ttse * table);

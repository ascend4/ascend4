
void alloc_tables();
void ttse_prepare(PureFluid *P);

double evaluate_ttse_p(PureFluid *P , double t, double rho);
double evaluate_ttse_h(PureFluid *P , double t, double rho);
double evaluate_ttse_s(PureFluid *P , double t, double rho);
double evaluate_ttse_u(PureFluid *P , double t, double rho);
double evaluate_ttse_g(PureFluid *P , double t, double rho);

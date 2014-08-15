/**
	Normalised Planck Integral.

	Returns the integral
	 $ I\left(\frac{1}{\lambda T}\right) = 
		\frac{int_0^\lambda {E_b \left(\lambda,T \right) d\lambda}{\sigma T^4} $

	Inputs are wavelength lambda (in metres) and temperature T (in kelvin).
*/
double plnkin(double lambda, double T);


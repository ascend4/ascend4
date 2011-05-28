#include "sunpos_grena.h"

/* WARNING: this is a FIRST DRAFT of the code and HAS NOT BEEN CHECKED yet. */

void
SunCoord::Calculate(){

	// calculation of JD and JDE
	double dYear, dMonth;
	if(Month <= 2){
		dYear = (double)Year - 1.;
		dMonth = (double)Month + 12.;
	}else{
		dYear = (double)Year;
		dMonth = (double)Month;
	}

	double JD_t = (double)trunc(365.25 * (dYear - 2000))
		+ (double)trunc(30.6001 * (dMonth + 1))
		+ (double)Day + UT/24. - 1158.5;

	double t = JD_t + Delta_t/86400;

#if 0
	// standard JD and JDE (useless for the computation, they are computed for
	// completeness:

	// (omitted)
#endif

	// HELIOCENTRIC LONGITUDE

	// linear increase + annual harmonic

	double ang = 1.72019e-2 * t - 0.0563;
	HeliocLongitude = 1.740940 + 1.7202768683e-2 * t + 3.34118e-2 * sin(ang) + 3.488e-4 * sin(2*ang);

	// moon perturbation

	HeliocLongitude += 3.13e-5 * sin(2.127730e-1*t - 0.585);

	// harmonic correction

	HeliocLongitude += 1.26e-5 * sin(4.243e-3 * t + 1.46)	
		+ 2.35e-5 * sin(1.0727e-2 * t + 0.72)
		+ 2.76e-5 * sin(1.5799e-2 * t + 2.35)
		+ 2.75e-5 * sin(2.1551e-2 * t - 1.98)
		+ 1.26e-5 * sin(3.1490e-2 * t - 0.80);

	// polynomial correction

	double t2 = t/1000;
	HeliocLongitude += ((( -2.30796e-7 * t2 + 3.7976e-6) * t2 - 2.0458e-5) * t + 3.976e-5) * t2*t2;

	// to obtain obtain Heliocentric longitude in the range [0,2pi] uncomment:
	// HeliocLongitude = fmod(HeliocLongitude, 2*PI);

	// END HELIOCENTRIC LONGITUDE CALCULATION

	// Correction to longitude due to nutation

	double delta_psi = 8.33e-5 * sin(9.252e-4 * t - 1.173);

	// Earth axis inclination

	double epsilon = -6.21e-9 * t + 0.409086 + 4.46e-5 * sin(9.252e-4 * t + 0.397);

	// Geocentric global solar coordinates:

	GeocSolarLongitude = HeliocLongitude + PI + delta_psi - 9.932e-5;

	double s_lambda = sin(GeocSolarLongitude);

	RightAscension = atan2(s_lambda * cos(epsilon), cos(GeocSolarLongitude));

	// local hour angle of the sun

	HourAngle = 6.30038809903 * JD_t + 4.8824623 + delta_psi * 0.9174 
		+ ObserverLongitude - RightAscension;

	// to obtain the local hour angle in the range [0,2pi] uncomment:
	// HourAngle = fmod(HourAngle,2*PI);

	// parallax correction to right ascension

	double d_alpha = -4.26e-5 * c_lat * s_H;
	TopocRightAscension = RightAscension + d_alpha;
	TopocHourAngle = HourAngle - d_alpha;

	// parallax correction to declination:

	TopocDeclination = Declination - 4.26e-5 * (s_lat - Declination * c_lat);
	double s_delta_corr = sin(TopocDeclination);
	double c_delta_corr = cos(TopocDeclination);
	double c_H_corr = c_H + d_alpha * s_H;
	double s_H_corr = s_H - d_alpha * c_H;

	// solar elevation angle, without refraction correction
	Elevation_no_refrac =  asin(s_lat * s_delta_corr + c_lat * c_delta_corr * c_H_corr);

	// refraction correction: it is calculated only
	// if Elevation_no_refract > elev_min

	const double elev_min = -0.01;
	
	if(Elevation_no_refrac > elev_min){
		RefractionCorrection = 0.084217 * Pressure / (273 + Temperature) 
			/ tan(Elevation_no_refrac + 0.0031376 / (Elevation_no_refrac + 0.089186));
	}else{
		RefractionCorrection = 0;

	// local coordinates of the sun

	Zenith = PI/2 - Elevation_no_refrac - RefractionCorrection;

	Azimuth = atan2(s_H_corr, c_H_corr*s_lat - s_delta_corr/c_delta_corr*c_lat);
}


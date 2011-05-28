
/* WARNING: this is a FIRST DRAFT of the code and HAS NOT BEEN CHECKED yet. */

/** @FILE
	Code reproduced with minor cosmetic changes from:
	R Grena (2008), An algorithm for the computation of the solar position, 
	Solar Energy (82), pp 462-470.

	This header file contains the declaration of a class which includes all the
	input and output data, and the function that performs the calculation.

	To calculate the sun position, follow these steps:

	1. include this file.
	2. declare a variable of type SunCoord.
	3. Initialise the variable given the 9 input quantities required.
	   This can be done in the declaration, listing the quantities between 
	   commas, or calling the function SetCoord(). In both cases, only the first
	   four quantities are required; the others default to standard values
	   (pressure = 1 atm, T = 20 °C, 0 for all the other quantities) if omitted.
	   Longitude and latitude must be given in RADIANS, pressure in ATM,
	   temperature in °C.
	   Day, Month and Year are integer, UT in decimal hour from 0 to 24 (e.g.
	   3:30 pm becomes 15.5).
	4. Call the Calculate() method of the SunCoord object.

	Example:
	(see the original publication)

	Warning: in order to improve accessibility and efficiency, there is not 
	access control in the class. The user is free to directly access and 
	modify all the data and there is not any control of consistency. Some
	caution in the use of the class is advisable.
*/
#ifndef SUNPOS_GRENA_H
#define SUNPOS_GRENA_H

#include <iostream>
#include <cmath>

#ifndef PI
# define PI 3.14159265358979
#endif


class SunCoord{
public:
	// input data
	double UT; 
	int Day, Month, Year;
	double Delta_t;
	double ObserverLatitude;
	double ObserverLongitude;
	double Pressure;
	double Temperature;

	// output data
	double HourAngle;
	double TopocRightAscension;
	double TopocDeclination;
	double TopocHourAngle;
	double Elevation_no_refrac;
	double RefractionCorrection;
	double Zenith;
	double Azimuth;

	// functions
	/** set the input data quickly */
	void SetCoord(double UT, int Day, int Month, int Year, double Delta_t
		, double ObserverLatitude, double ObserverLongitude, double Pressure
		, double Temperature
	) : UT(UT), Day(Day), Month(Month), Year(Year), Delta_t(Delta_t)
		, ObserverLatitude(ObserverLatitude)
		, ObserverLongitude(ObserverLongitude), Pressure(Pressure)
		, Temperature(Temperature){}

	void Calculate();
}






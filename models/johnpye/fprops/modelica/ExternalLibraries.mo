
model ExternalLibraries
  import SI = Modelica.SIunits;

  function Tsat_p_Na
    input SI.Pressure p;
	output SI.Temperature T;
	external T = Tsat_p_Na(p)
      annotation(Library="libfprops_mo.so", LibraryDirectory="modelica://ExternalLibraries");
  end Tsat_p_Na;

  function T_ph_Na
    input SI.Pressure p;
	input SI.SpecificEnthalpy h;
	output SI.Temperature T;
	external T = T_ph_Na(p,h)
      annotation(Library="libfprops_mo.so", LibraryDirectory="modelica://ExternalLibraries");
  end T_ph_Na;

  // heating up of a mass of fluid

  parameter SI.HeatFlowRate Qdot = 10e6;
  parameter SI.Temperature T_in = 200 + 273.15;
  parameter SI.Pressure p = 1e5;
  parameter SI.Mass m = 1000.;

  SI.Energy E; // fixme need to check isobaric internal energy/enthalpy
  SI.Temperature T(start=100+273.15, fixed=true);
  SI.SpecificEnthalpy h(start=500e3);

equation
  der(E) = Qdot;
  h * m = E;
  T = T_ph_Na(p,h);  
end ExternalLibraries;



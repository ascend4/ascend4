
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

  parameter SI.SpecificEnthalpy h_in = 500e3;
  parameter SI.Temperature T_in = 200 + 273.15;
  parameter SI.HeatFlowRate Qdot = 10;
  parameter SI.MassFlowRate mdot = 1;
  parameter SI.Pressure p = 1e5;

  SI.SpecificEnthalpy h_out;
  SI.Energy E(start=0.); // fixme need to check isobaric internal energy/enthalpy
  SI.Mass m(start=1000.,fixed=true);
  SI.Temperature T(start=100+273.15, fixed=true);
  SI.SpecificEnthalpy h;

initial equation
  T_in = T_ph_Na(p,h_in);

equation
  der(m) = mdot;
  der(E) = mdot*h_in;
  h = E / m;
  T = T_ph_Na(p,h);  
end ExternalLibraries;



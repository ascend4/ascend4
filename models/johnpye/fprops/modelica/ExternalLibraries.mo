model ExternalLibraries

  function ExternalFunc2
    input Real i2;
    output Real o2;
    external "C" 
      annotation(Library="ExternalFunc2", LibraryDirectory="modelica://ExternalLibraries");
  end ExternalFunc2;

  function Tsat_p_Na
    input Real p;
	output Real T;
	external T = Tsat_p_Na(p)
      annotation(Library="libfprops_mo.so", LibraryDirectory="modelica://ExternalLibraries");
  end Tsat_p_Na;

  Real y(start=2.0, fixed=true);
  parameter Real p = 1e5;
  Real T(start=1.0, fixed=true);

equation
  der(y)=-ExternalFunc2(y);
  T = Tsat_p_Na(p);
end ExternalLibraries;


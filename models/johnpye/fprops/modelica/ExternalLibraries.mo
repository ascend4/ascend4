model ExternalLibraries

  function ExternalFunc1
    input Real i1;
    output Real o1;
    external o1=ExternalFunc1_ext(i1) 
      annotation(Library="ExternalFunc1.o", LibraryDirectory="modelica://ExternalLibraries", Include="#include \"ExternalFunc1.h\"");
  end ExternalFunc1;

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
      annotation(Library="fprops_mo", LibraryDirectory="modelica://ExternalLibraries");
  end Tsat_p_Na;

  Real x(start=1.0, fixed=true);
  Real y(start=2.0, fixed=true);
  parameter Real p = 1e5;
  Real T(start=1.0, fixed=true);

equation
  der(x)=-ExternalFunc1(x);
  der(y)=-ExternalFunc2(y);
  T = Tsat_p_Na(p);
end ExternalLibraries;


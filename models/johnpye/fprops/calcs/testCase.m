#!/Applications/Octave.app/Contents/Resources/bin/octave
%Octave test of peng robinson for T=398.15 K and p=10 bar
%Oxygen used as fluid for comparison with Sandler book

%Fluid Properties:
T_c=154.6; p_c=5.462e6; omega=0.021; kappa=0.37464+1.54226*omega-0.26992*omega^2;
%Other Properties:
R=8.314;

%Case Variables:
T=398.15; p=10e6;

%We want to solve Z^3+(B-1)Z^2+(A-3B^2-2B)Z+(B^3+B^2-AB)=0 for Z
alpha_root=1+kappa*(1-sqrt(T/T_c));
alpha=alpha_root^2
a=0.457235529*((R^2*T_c^2)/p_c)*alpha
b=.077796074*R*T_c/p_c
A=a*p/(R*T)^2
B=b*p/(R*T)
%Which leaves us with the polynomial:
poly=[1, B-1, A-3*B^2-2*B, B^3+B^2-A*B];
%Solving:
cubicRoots=roots(poly);
Z=cubicRoots(imag(cubicRoots)==0)

%We can now find the molar volume from Z:
V=Z*R*T/p

%Plugging this back in to the equation to find pressure
%(Test to check we're on the right track: ~4% Error it seems)
p_new=R*T/(V-b)-a/(V*(V+b)+V*(V-b))

%Next we want to evaluate the departure functions, which will give
%us the difference between the ideal and real enthalpy/entropy\
da_dt_2=-0.457235529*T_c^(1.5)*R^2/p_c*kappa*(alpha_root/sqrt(T))
da_dt=-a*kappa/(sqrt(T_c)*alpha_root*sqrt(T))
da_dt_3=-0.45724*T_c*R^2/p_c*kappa*(sqrt(T_c/T)-1)
da_dt_4=-0.45724*T_c*R^2/p_c*kappa*(sqrt(alpha/(T*T_c)))

%Enthalpy:
h_dep=R*T*(Z-1)+(T*da_dt-a)/(2*sqrt(2)*b)*log((Z+(1+sqrt(2))*B)/(Z+(1-sqrt(2))*B));

%Entropy:
s_dep=R*log(Z-B)+da_dt/(2*sqrt(2)*b)*log((Z+(1+sqrt(2))*B)/(Z+(1-sqrt(2))*B));

%We're also interested in the ideal components, so that we can
%calculate real properties:
h_ideal=(25.46*(T-298.15)+1.519e-2/2*(T^2-298.15^2)-0.7151e-5/3*(T^3-298.15^3)+1.311e-9/4*(T^4-298.15^4));
s_ideal=(25.46*log(T/298.15)+1.519e-2*(T-298.15)-0.7151e-5/2*(T^2-298.15^2)+1.311e-9/3*(T^3-298.15^3)-R*log(p/1e5));

%Finally we can calculate the actual properties!!
h=h_ideal+h_dep;
s=s_ideal+s_dep;

%Output the results:
printf("h: %f (%f%+f)\n",h,h_ideal,h_dep);
printf("s: %f (%f%+f)\n",s,s_ideal,s_dep);
%% Peng-Robinson.m
% Matlab / Octave implementation of the Peng-Robinson EOS. This script is
% intended to be a quick implementation to be used for comparison with a
% future C implementation. It is *not* intended to serve any further
% practical purpose.
%% Fluid Properties
T_c=154.59; %Critical Temperature (Kelvin)
P_c=5.046e6; %Critical Pressure (Pascals)
omega=0.021; %Acentric Factor (Dimensionless)
R=8.3144621; %Gas Constant (J/mol K)
%% The Equation
%P=(R*T)/(V_m-b)-a(alpha(T))/(V_m^2+2*V_m*b-b^2)
alpha = @(T)(1+(0.37464+1.54226*omega-0.26992*omega^2)*(1-sqrt(T/T_c)))^2;
a = @(alpha)(0.457236*alpha*R^2*T_c^2)/P_c;
b = (0.0777961*R*T_c)/P_c;
%% Solving for P
% For given temperature and relative volume the equation gives P directly:
T=398.15; V_m=33.1e-3;
P=((R*T)/(V_m-b)-a(alpha(T))/(V_m*(V_m+b)+b*(V_m-b)))
%% Solving for T
% Rearranging the equation into polynomial form gives:
%{
A_star=@(T)(a(alpha(T))*P)/(R^2*T^2);
B_star=@(T)(b*P)/(R*T);
u=2;
w=-1;
Z=@(T)(P*V_m)/(R*T);
pengRob=@(T)Z(T)^3-(1+B_star(T)-u*B_star(T))*Z(T)^2+...
    (A_star(T)+w*B_star(T)^2-u*B_star(T)-...
    u*B_star(T)^2)*Z(T)-A_star(T)*B_star(T)-w*B_star(T)^2-w*B_star(T)^3;
%%
% Given pressure and relative volume, and a good initial guess we can solve
% for temperature numerically:
P=0.1e6; V_m=0.0245; Tguess=300;
T=fzero(pengRob,Tguess)
%% Solving for V_m
% For a given temperature and pressure we can find the roots of the above
% polynomial in Z
P=0.1e6; T=298.15;
poly=[1
    -(1+B_star(T)-u*B_star(T))
    (A_star(T)+w*B_star(T)^2-u*B_star(T)-u*B_star(T)^2)
    -(A_star(T)*B_star(T)+w*B_star(T)^2+w*B_star(T)^3)]';
Z=real(roots(poly));
V_m=(Z(1)*R*T)/(P)%}
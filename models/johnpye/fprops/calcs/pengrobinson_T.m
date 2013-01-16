function [T] = pengrobinson_T(P,V_m,T_c,P_c, omega, R)
    alpha=@(T)(1+(.37464+1.54226*omega-.26992*omega^2)*(1-sqrt(T/T_c)))^2;
    a = @(alpha)(0.457236*alpha*R^2*T_c^2)/P_c;
    b = (0.0777961*R*T_c)/P_c;
    A_star=@(T)(a(alpha(T))*P)/(R^2*T^2);
    B_star=@(T)(b*P)/(R*T);
    u=2; w=-1;
    Z=@(T)(P*V_m)/(R*T);
    pengRob=@(T)Z(T)^3-(1+B_star(T)-u*B_star(T))*Z(T)^2+...
    (A_star(T)+w*B_star(T)^2-u*B_star(T)-...
    u*B_star(T)^2)*Z(T)-A_star(T)*B_star(T)-w*B_star(T)^2-w*B_star(T)^3;
    Tguess=300;
    T=fzero(pengRob,Tguess);
end
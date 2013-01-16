function [P] = pengrobinson_P(T, V_m, T_c, P_c, omega, R)
    alpha = @(T)(1+(0.37464+1.54226*omega-0.26992*omega^2)*(1-sqrt(T/T_c)))^2;
    a = @(alpha)(0.457236*alpha*R^2*T_c^2)/P_c;
    b = (0.0777961*R*T_c)/P_c;
    P=(R*T)/(V_m-b)-a(alpha(T))/(V_m^2+2*V_m*b-b^2);
end
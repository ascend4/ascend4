This folder contains the DOPRI5 one-step Runge-Kutta integrator, which
is being developed as a external integrator, rather than one that is 
built in to ASCEND. It hasn't yet been finished, and doesn't even compile yet.

The DOPRI5 algorithm should be effectively the same as used in the Matlab
'ode45' function, and according to Ascher and Petzold (Computer Methods for
Ordinary Differential Equations and Differential-Algebraic Equations, SIAM,
1998) using the Dormand-Price (ie DOPRI) approach 'reflects the accumulated
experience' that it performed better in practise that the Fehlberg 4(5) 
embedded pair for Runge-Kutta integration.

Part of the development of this integrator will involve some changes to the
base code to allow solvers/integrators to be loaded and registered at run-time.

If you have a need for this integrator, however, feel free to tinker with the
code or contact John Pye for suggstions/help.

May 2007.


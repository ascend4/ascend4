DOPRI5
------

This folder contains the DOPRI5 one-step Runge-Kutta integrator.

Please consult the ASCEND wiki for more information:
http://ascendwiki.cheme.cmu.edu/DOPRI5

DOPRI5 was written by Ernst Hairer (c) 2004. Details are available from
http://www.unige.ch/~hairer/software.html.
See also LICENSE.txt in the same directory as this file.

The situations where the DOPRI5 integrator is appropriate are discuessed in the
excellent book by Ascher and Petzold, 'Computer Methods for Ordinary 
Differential Equations and Differential-Algebraic Equations', ISBN 0898714125.

This ASCEND solver was first written in May 2007.

As of Feb 2008, it is being moved to the standard ASCEND 'solvers' directory,
to ensure that it receives the same maintenance as the other solvers.

This solver appears to essentially work OK, but it has not been extensively
checked.

For example, the models/test/dopri5/aren.a4c model seems to behave well for a
single orbit, but diverges after that. The reason for the divergence is not
yet clear.

-- 
John Pye
6 Feb 2008


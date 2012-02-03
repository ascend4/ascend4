
import ascpy
import TReport


import matplotlib as mpl
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import matplotlib.pyplot as plt


## CODE BEGINS


lib = ascpy.Library()
lib.load('test/ida/leon/bouncingball.a4c')

M = lib.findType('bouncingball').getSimulation('sim')
M.setSolver(ascpy.Solver('QRSlv'))

P = M.getParameters()

I = ascpy.Integrator(M)


I.setEngine('IDA')
I.setLinearTimesteps(ascpy.Units("s"), 0.0, 23, 300);
I.setMinSubStep(0.1);
I.setMaxSubStep(1);
I.setInitialSubStep(0.001);
I.setMaxSubSteps(400);

I.setParameter('autodiff', False);
I.analyse();
reporter = TReport.TestReporter(I);
I.setReporter(reporter)
I.solve();

fig = plt.figure()
data = reporter.data;
plt.plot(reporter.t, data)
plt.show()

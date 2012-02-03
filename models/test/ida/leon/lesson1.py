
import ascpy
import TReport


import matplotlib as mpl
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import matplotlib.pyplot as plt


## CODE BEGINS


lib = ascpy.Library()
lib.load('test/ida/leon/electron.a4c')

M = lib.findType('electron').getSimulation('sim')
M.setSolver(ascpy.Solver('QRSlv'))

P = M.getParameters()

I = ascpy.Integrator(M)


I.setEngine('IDA')
I.setLinearTimesteps(ascpy.Units("s"), 0.0, 40.0, 60);
I.setMinSubStep(0.0001);
I.setMaxSubStep(0.1);
I.setInitialSubStep(0.001);
I.setMaxSubSteps(200);

I.setParameter('autodiff', False);
I.analyse();
reporter = TReport.TestReporter(I);
I.setReporter(reporter)
I.solve();

x1 = []
x2 = []
data = reporter.data;
for i in range(len(data)):
    x1.append(data[i][0])
    x2.append(data[i][1])

fig = plt.figure()
ax = Axes3D(fig)
ax.plot(x1, x2, reporter.t)
ax.set_zlabel("t")
plt.xlabel("X")
plt.ylabel("y")
ax.set_xlim3d(0, 20)
ax.set_ylim3d(-15, 5)
plt.show()

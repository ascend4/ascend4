#!/usr/bin/python

import numpy as np
import matplotlib.pyplot as plt

with open("test_res.dat") as f:
    data = f.read()

data = data.split('\n')
del data[0]
del data[len(data)-1]
x = [row.split('\t')[0] for row in data]
y = [row.split('\t')[3] for row in data]

fig = plt.figure()

ax1 = fig.add_subplot(111)

ax1.set_title("Density profile")    
ax1.set_xlabel('Temperature [K]')
ax1.set_ylabel('Density [kg/cu.m]')

ax1.plot(x,y, c='r')

leg = ax1.legend()

plt.show()




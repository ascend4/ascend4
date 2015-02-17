#!/usr/bin/env python
# file 'example.py':

import ascpy
from ascpy import *
L = ascpy.Library()

L.load("ksenija/derivative1.a4c")

T = L.findType("test1")
M = T.getSimulation('sim',1)

x = M.x
t1 = M.t1
print "Der(x,t1) = ",float(der(x,t1))

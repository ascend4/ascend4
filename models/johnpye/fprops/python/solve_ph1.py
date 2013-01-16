from fprops import *

F = fluid("water","helmholtz")

F.set_ph(150e5, 1000e3)

print "T =", F.T



from fprops import *
from numpy import *
import sys
import fileinput

testfluids = ['water','toluene','carbon_dioxide','xenon','ammonia','methane','hydrogen']

myfile = open("out.txt",mode = 'w')
for i in testfluids:
    D = fluid(i, 'rpp','pengrob')
    TT = linspace(D.T_t+.001, D.T_c-.015, 20);
    for t in TT:
        try: # try FPROPS
            p, rhof, rhog = D.sat_T_cubic(round(t,3))
            p=p/1000
        except:
            p='not converged'
            rhof ='not converged'
            rhog = 'not converged'
        
        myfile.write('%-15s %-15s %-15s %-15s %s\n' % (i,str(round(t,3)),str(p),str(rhof),str(rhog)))



myfile.close()

with open('src.txt', 'r') as infile:
    suffix=[]
    i=0
    for line in infile:
        suffix.insert(i,line.rstrip('\n'))
        i=i+1
i=0     
x = fileinput.input('out.txt',inplace=1)
for line in x:
    line = line.replace(line,suffix[i]+'             '+line)
    print line
    i=i+1


print suffix









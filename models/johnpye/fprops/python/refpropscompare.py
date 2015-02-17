#Python 3.2 required!
import refprop as rp #This import requires an installation of Refprops and a compiled shared object file!
import numpy
import sys

#The following import call imports the test methods in rptest.  Uncomment to run tests
#from rptest import *
#The following calls all the refprop tests in rptest.py
#settest('refprop') 

testfluids = ['water','toluene','co2','xenon','ammonia','methane','hydrogen']

'''Most of these functions return an array of values.The variable 'props' is used to capture this array,
then the information desired is extracted from the variable.'''

myfile = open("src.txt",mode = 'w', encoding = 'utf-8')
for i in testfluids:
    rp.setup('def',i)
    print(i)
    rp.preos(2) #ensures that all refprops functions use PR EOS
    x = [1]
    prop = rp.critp(x)
    tcrit = prop['tcrit']
    prop = rp.info(1)
    ttrip = prop['ttrp']
    TT = numpy.linspace(ttrip+.001, tcrit-.015, 20)
    for t in TT:
        try: # try REPROPS
            prop = rp.satt(round(t,3), x)
            dliq = prop['Dliq'] #returns molar density in mol/L so must convert
            dvap = prop['Dvap']
            satpressure = prop['p'] #kPa
            prop = rp.wmol(x)
            molwt = prop['wmix']
            dliq = dliq*molwt
            dvap = dvap*molwt
        except:
            satpressure = 'not converged'
            dliq = 'not converged'
            dvap = 'not converged'

        myfile.write('%-15s %-15s %-10s\n' % (i,str(round(t,3)),str(satpressure)))

    rp.preos(0)



myfile.close()










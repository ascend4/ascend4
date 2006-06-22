import unittest

class AscendTest(unittest.TestCase):

	def testloading(self):
		import ascpy

	def testsystema4l(self):
		import ascpy
		L = ascpy.Library()
		L.load('simpleflowsheet01.a4c')
		S = L.getType('test_controller').getSimulation('mysim')
		print "LOADED test_controller"	
		
if __name__=='__main__':
	unittest.main()


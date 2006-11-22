import unittest

class AscendTest(unittest.TestCase):

	def testloading(self):
		import ascpy

	def testsystema4l(self):
		import ascpy
		L = ascpy.Library()
		L.load('simpleflowsheet01.a4c')
		print "LOADED simpleflowsheet01"

	def testIDA(self):
		import ascpy
		try:
			L = ascpy.Library()
			L.load('johnpye/shm.a4c')
			M = L.findType('shm').getSimulation('sim')
		except Exception,e:
			fail(str(e))
		
if __name__=='__main__':
	unittest.main()


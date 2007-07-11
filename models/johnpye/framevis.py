# ASCEND modelling environment
# Copyright (C) 2007 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

#---------------------------------------------------------------------
# Frame visualisation module. This file provides the ExtPy
# external python method for use with specially-arranged ASCEND
# models describing three-dimensional wire-frame geometry in terms
# of nodes and edges. The 'framevis' method provided by this
# file will display an OpenGL visualisation of the wireframe
# and permit 3D manipulation of it.
#
# By John Pye, July 2007.

import extpy

try:
	import pygtk
	pygtk.require('2.0')
	from gtk.gtkgl.apputils import *
	gtkgl_deps = 1

	class AscFrame:
		def __init__(self,model):
			self.model = model
		def draw(self):
			for e in self.model.edges.getChildren():
				A = [float(i) for i in [e.A.x,e.A.y,e.A.z]]
				B = [float(i) for i in [e.B.x,e.B.y,e.B.z]]
				glBegin(GL_LINE_STRIP)
				glVertex3f(*A)
				glVertex3f(*B)
				glEnd()

	class AscScene(GLScene,
	             GLSceneButton,
	             GLSceneButtonMotion):

		def __init__(self, frame):
			GLScene.__init__(self,
			    gtk.gdkgl.MODE_RGB   |
			    gtk.gdkgl.MODE_DEPTH |
			    gtk.gdkgl.MODE_DOUBLE
			)

			self.rotx = 0
			self.roty = 0

			self.is_solid = False

			self.beginx = 0
			self.beginy = 0

			objectradius = 20.;
			self.location = [0,0,-objectradius*2.0]
			self.far = 5 * objectradius
			self.near = 1.0
			self.fov = 55 # field of view, degrees

			self.frame = frame
	    
		def init(self):
			glMaterial(GL_FRONT, GL_AMBIENT,   [0.2, 0.2, 0.2, 1.0])
			glMaterial(GL_FRONT, GL_DIFFUSE,   [0.8, 0.8, 0.8, 1.0])
			glMaterial(GL_FRONT, GL_SPECULAR,  [1.0, 0.0, 1.0, 1.0])
			glMaterial(GL_FRONT, GL_SHININESS, 50.0)

			glLight(GL_LIGHT0, GL_AMBIENT,  [0.0, 1.0, 0.0, 1.0])
			glLight(GL_LIGHT0, GL_DIFFUSE,  [1.0, 1.0, 1.0, 1.0])
			glLight(GL_LIGHT0, GL_SPECULAR, [1.0, 1.0, 1.0, 1.0])
			glLight(GL_LIGHT0, GL_POSITION, [1.0, 1.0, 1.0, 0.0])

			glLightModel(GL_LIGHT_MODEL_AMBIENT, [0.2, 0.2, 0.2, 1.0])

			glEnable(GL_LIGHTING)
			glEnable(GL_LIGHT0)
			glDepthFunc(GL_LESS)
			glEnable(GL_DEPTH_TEST)
	    
		def reshape(self, width, height):
			self.width = width
			self.height = height
			glViewport(0, 0, width, height)
			self.reorient()

		def reorient(self):
			glMatrixMode(GL_PROJECTION)
			glLoadIdentity()
			gluPerspective(self.fov,float(self.width)/self.height,self.near,self.far)
			glMatrixMode(GL_MODELVIEW)

		def display(self, width, height):
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
			glLoadIdentity()
			glTranslate(*(self.location))
			glRotate(self.rotx, 1, 0, 0)
			glRotate(self.roty, 0, 1, 0)
			self.frame.draw()

		def button_press(self, width, height, event):
		    self.beginx = event.x
		    self.beginy = event.y

		def button_release(self, width, height, event):
		    pass

		def button_motion(self, width, height, event):
			if event.state & (gtk.gdk.BUTTON3_MASK | gtk.gdk.BUTTON1_MASK):
				# ROTATE
			    self.rotx = self.rotx + ((event.y-self.beginy)/width)*360.0
			    self.roty = self.roty + ((event.x-self.beginx)/height)*360.0

			elif event.state & gtk.gdk.BUTTON2_MASK:
				# ZOOM
				self.fov = self.fov + ((event.y-self.beginy)/width)*180
				if self.fov < 5:
					self.fov = 5;
				print "FOV =",self.fov
				self.reorient()

			elif event.state & gtk.gdk.BUTTON3_MASK:
				# PAN
				print "PAN"

			self.beginx = event.x
			self.beginy = event.y

			self.invalidate()


	# A simple window to show the Teapot scene
	# in a GLArea widget along with two
	# sliders for rotating the teapot rendered
	# in the scene. The teapot can also be
	# rotated using mouse button drag motion.

	class ModelWindow(gtk.Window):
		def __init__(self,frame):
			gtk.Window.__init__(self)

			# Set self attfibutes.
			self.set_title('pyramid')
			if sys.platform != 'win32':
			    self.set_resize_mode(gtk.RESIZE_IMMEDIATE)
			self.set_reallocate_redraws(True)
			self.connect('destroy', lambda quit: gtk.main_quit())
				        
			# The Teapot scene and the
			# GLArea widget to
			# display it.
			self.scene = AscScene(frame)
			self.glarea = GLArea(self.scene)
			self.glarea.connect("expose_event",self.on_expose_event)
			self.glarea.set_size_request(300,300)
			self.glarea.show()
			self.add(self.glarea)

		def on_expose_event(self,*args):
			print "EXPOSE EVENT"
			self.scene.invalidate()

		def run(self):
		    self.show()
		    gtk.main()

except:
	gtkgl_deps = 0

#---------------------------------------------------------------------
def framevis(self):
	"""Visualise the frame using OpenGL and GtkGlExt"""

	browser = extpy.getbrowser()
	if not browser:
		print "no 'browser'"
		return 1

	if not gtkgl_deps:
		browser.reporter.reportError("Unable to load PyGtkGlExt")
		return 1		

	REP = browser.reporter.reportNote

	frame = AscFrame(self)

	win = ModelWindow(frame)
	win.run()

extpy.registermethod(framevis)


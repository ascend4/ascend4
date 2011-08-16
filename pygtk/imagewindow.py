# General-purpose popup window for reporting graphical stuff

import gtk, pango
import ascpy
from varentry import *
import os,shutil

class ImageWindow:
	def __init__(self,browser,parent,imagefilename,title,delete=False):
		self.browser = browser;
		self.imagefilename = imagefilename
		self.delete = delete

		# GUI config
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["imagewindow"])
		self.window = self.browser.builder.get_object("imagewindow")
		self.vbox = self.browser.builder.get_object("vbox1")
		self.closebutton = self.browser.builder.get_object("closebutton")
		self.window.set_title(title)

		if self.browser.icon:
			self.window.set_icon(self.browser.icon)

		self.parent = None
		if parent:
			self.parent = parent
			self.window.set_transient_for(self.parent)
		
		self.pixbuf = gtk.gdk.pixbuf_new_from_file(imagefilename)
		self.w_orig = self.pixbuf.get_width()
		self.h_orig = self.pixbuf.get_height()

		self.scrollwin = gtk.ScrolledWindow()
		self.scrollwin.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
		self.imageview = gtk.Image()
		
		self.scrollwin.add_with_viewport(self.imageview)
		self.imageview.show()
		self.scrollwin.show()
		self.vbox.add(self.scrollwin)

		self.browser.builder.connect_signals(self)

		# more than 100% is pointless
		self.zoom_max = 1
		self.zoom_step = 1.5;

		self.zoom(fit=True)
		self.zoom_fit = self.zoom_current

	def on_save_clicked(self,*args):
		self.browser.reporter.reportNote("SAVE %s" % self.imagefilename) 
		chooser = gtk.FileChooserDialog(title="Save Incidence Graph (PNG)",action=gtk.FILE_CHOOSER_ACTION_SAVE,
			buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_SAVE,gtk.RESPONSE_OK))
		chooser.set_current_name("incidence.png")
		
		response = chooser.run()
		if response==gtk.RESPONSE_OK:
			if os.path.exists(chooser.get_filename()):
				label = gtk.Label("File Already Exists, Overwrite?")
				dialog = gtk.Dialog("Error",None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                    gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
				dialog.vbox.pack_start(label)
				label.show()
				response = dialog.run()
				if response == gtk.RESPONSE_ACCEPT:
					shutil.copy(self.imagefilename,chooser.get_filename())
					dialog.destroy()
					self.browser.reporter.reportWarning("FILE SAVED: '%s'" % chooser.get_filename())
					chooser.destroy()
				else:
					dialog.destroy()
					self.browser.reporter.reportWarning("FILE NOT SAVED")
					chooser.destroy()
			else:
				shutil.copy(self.imagefilename,chooser.get_filename())
				self.browser.reporter.reportWarning("FILE SAVED: '%s'" % chooser.get_filename())
				chooser.destroy()

	def on_zoomfit_clicked(self,*args):
		self.zoom(fit=1)

	def on_zoomnormal_clicked(self,*args):
		self.zoom()

	def on_zoomin_clicked(self,*args):
		newzoom = self.zoom_current * self.zoom_step
		if newzoom > self.zoom_max:
			newzoom = self.zoom_max
		self.zoom(newzoom)

	def on_zoomout_clicked(self,*args):
		newzoom = float(self.zoom_current) / self.zoom_step
		self.zoom(newzoom)

	def zoom(self,ratio=1,fit=False):
		self.browser.reporter.reportNote("ZOOM TO %d %%" % (ratio*100))
		w_view,h_view = self.window.get_size()

		r_w = float(w_view) / self.w_orig	
		r_h = float(h_view) / self.h_orig
		if r_w < r_h:
			ratio_fit = r_w
		else:
			ratio_fit = r_h

		if fit:
			ratio = ratio_fit
			self.is_fit = True
		else:
			self.is_fit = False

		if ratio < ratio_fit:
			ratio = ratio_fit
	
		w = int(self.w_orig * ratio)
		h = int(self.h_orig * ratio)
		if ratio==1:		
			self.imageview.set_from_pixbuf(self.pixbuf)
		else:
			pixbufnew = self.pixbuf.scale_simple(w,h,gtk.gdk.INTERP_BILINEAR)
			self.imageview.set_from_pixbuf(pixbufnew)

		self.zoom_current = ratio

	def run(self):
		self.window.show()

	def on_imagewindow_remove(self,*args):
		if self.delete:
			os.unlink(self.imagefilename)

	def on_imagewindow_size_request(self,*args):
		if self.is_fit:
			self.zoom(fit=1)
			


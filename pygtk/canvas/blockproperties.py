#!/usr/bin/env python

import gtk, gobject, os, gtksourceview, pango, re
import blockinstance
import preferences

UNITS_RE = re.compile("([-+]?(\d+(\.\d*)?|\d*\.d+)([eE][-+]?\d+)?)\s*(.*)");

class BlockProperties:
	def __init__(self, parent, item, tab=None):
		bi = item.blockinstance

		icons = gtk.IconFactory()
		
		_fixedicon = gtk.Image()
		_fixedicon.set_from_file(os.path.join("../glade/locked.png"))
		icons.add("ascend-fixed-icon",gtk.IconSet(_fixedicon.get_pixbuf()))
	
		_freeicon = gtk.Image()
		_freeicon.set_from_file(os.path.join("../glade/unlocked.png"))
		icons.add("ascend-free-icon",gtk.IconSet(_freeicon.get_pixbuf()))

		icons.add_default()

		print "Opening Block Properties Dialog..."
		self.bpdialog = gtk.Dialog()
		self.bpdialog.set_size_request(450,400)
		self.bpdialog.set_title('Properties for block "%s"' % bi.name)
		#self.bpdialog.set_transient_for(parent)
		windowicon = gtk.Image()
		windowicon.set_from_file(os.path.join("../glade/ascend.svg"))
		self.bpdialog.set_icon(windowicon.get_pixbuf())
		
		# create the 'General' tab

		table = gtk.Table(3, 6, False)
		self.bpdialog.vbox.pack_start(table, True, True, 0)
		self.notebook = gtk.Notebook()
		self.notebook.set_tab_pos(gtk.POS_TOP)
		table.attach(self.notebook, 0,6,0,1)
		self.show_tabs = True
		self.show_border = True
		
		text = bi.name
		text += " - Block Description : \n\n"
		self.pnamelbl = gtk.Label("Name:")
		self.pname = gtk.Entry()
		self.pname.set_text(bi.name)
		hb = gtk.HBox()
		
		hb.pack_start(self.pnamelbl, True, False)
		hb.pack_start(self.pname, True, False)
		text2 = "\nType:\n\t\t%s\n" % bi.blocktype.type.getName()
		text2 += "\nPorts:\n"
		for k,v in bi.ports.iteritems():
			text2 += "\t\t%s" % v.name
	
			if v.type == blockinstance.PORT_IN:
				text2 += " (IN)\n"
			elif v.type == blockinstance.PORT_OUT:
				text2 += " (OUT)\n"
			elif v.type == blockinstance.PORT_INOUT:
				text2 += " (IN/OUT)\n"
				
		if hasattr(bi,"params"):
			#text2 += "\nParameters:\n"
			
			for k,v in bi.params.iteritems():
				pass
				#text2 += "\t\t%s = %s\n" % (v.name, v.value)
			if bi.instance:
				text2 += "\nInstance exists"
				self.inst = True
			else:
				text2 += "\nNo instance exists"
				self.inst = False
				
		scrolled_window0 = gtk.ScrolledWindow()
		scrolled_window0.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
   		#genf = gtk.Frame('General Block Info')
		#genf.set_border_width(0)
		lbl1 = gtk.Label(text)
		lbl2 = gtk.Label(text2)
		vbox = gtk.VBox()
		vbox.pack_start(lbl1, True, True)
		vbox.pack_start(hb, True, True)
		vbox.pack_start(lbl2, True, True)
		
		#if self.inst == True:
			#import modeltree
			#MT = modeltree.TreeView(bi.instance)
			#vbox.pack_start(MT.treeview, True, True)
			
		scrolled_window0.add_with_viewport(vbox)
   		label0 = gtk.Label('General')
		self.notebook.append_page(scrolled_window0, label0)
		
		# Create the 'Parameters' tab

		paramsvbox = gtk.VBox(homogeneous=True, spacing=5)

		self.button = {}
		self.buttonflag = {}
		self.textbox = {}
		self.paramflag = 0
		if hasattr(bi,"params"):
			for k,v in bi.params.iteritems():
				self.paramflag = 1
				paramvbox = gtk.VBox()
				lbl = gtk.Label(v.name) 
				hbox=gtk.HBox()
				hbox.pack_start(lbl, True, False, 0)
				self.button[v.name] = gtk.Button()
				#self.buttonflag[v.name] = bi.pfix[v.name]
				self.buttonflag[v.name] = v.fix
				if self.buttonflag[v.name] == 0:
					self.button[v.name].set_image(gtk.image_new_from_stock("ascend-free-icon",gtk.ICON_SIZE_BUTTON))
				else:
					self.button[v.name].set_image(gtk.image_new_from_stock("ascend-fixed-icon",gtk.ICON_SIZE_BUTTON))
				self.button[v.name].connect("clicked", self.update_FIXed_button, v.name, item )
				hbox.pack_start(self.button[v.name], False, False, 0)
				self.textbox[v.name] = gtk.Entry()
				self.textbox[v.name].set_text(str(v.value))
				self.textbox[v.name].connect("key-press-event",self.FIX_button, v.name)
				hbox.pack_start(self.textbox[v.name], True, False, 0)
				paramvbox.pack_start(hbox, False, False, 0)
				desclbl = gtk.Label(v.get_description())
				paramvbox.pack_start(desclbl, False, False, 0)
				paramsvbox.pack_start(paramvbox)
			
		if self.paramflag:	
			scrolled_window1 = gtk.ScrolledWindow()
			scrolled_window1.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
			#scrolled_window1.set_border_width(10)
			label = gtk.Label("Parameters")
			scrolled_window1.add_with_viewport(paramsvbox)
			self.notebook.append_page(scrolled_window1, label)
			#self.paramflag = 0	
				
		#create the 'custom method' entry tab
		
		methodvbox = gtk.VBox(homogeneous=True)
		methodframe = gtk.Frame("Enter your custom METHODs for the block below")
		methodentry = gtksourceview.SourceView()
		methodentry.set_show_line_numbers(True) 
		self.sb = gtksourceview.SourceBuffer()
		self.sb.set_check_brackets(True)
		self.sb.set_highlight(True)
		self.sb.set_text(bi.usercode)
		methodentry.set_buffer(self.sb)
		scrolled_window2 = gtk.ScrolledWindow()
		scrolled_window2.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		label = gtk.Label("Custom Method")
		methodframe.add(methodentry)
		scrolled_window2.add_with_viewport(methodframe)
		self.notebook.append_page(scrolled_window2, label)
		
		#the instance view tab
		if self.inst == True:
			scrolled_window0 = gtk.ScrolledWindow()
			scrolled_window0.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
			#genf = gtk.Frame('General Block Info')
			#genf.set_border_width(0)
			vbox = gtk.VBox()
			#vbox.pack_start(lbl1, True, True)
			
			import modeltree
			MT = modeltree.TreeView(bi.instance)
			vbox.pack_start(MT.treeview, True, True)
				
			scrolled_window0.add_with_viewport(vbox)
			label0 = gtk.Label('Instance')
			self.notebook.append_page(scrolled_window0, label0)
		
						
		self.notebook.show_all()
		if tab == None:
			self.notebook.set_current_page(item.blockinstance.tab)
		else:
			if self.paramflag :
				self.notebook.set_current_page(tab)
			else:	
				self.notebook.set_current_page(tab-1)			
		#action area buttons
		#help = self.bpdialog.add_button(gtk.STOCK_HELP, gtk.RESPONSE_OK)
		#appl = self.bpdialog.add_button(gtk.STOCK_APPLY, gtk.RESPONSE_OK)
		cancel = self.bpdialog.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		ok = self.bpdialog.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		ok.connect("clicked", self.on_properties_ok_clicked, self.buttonflag, item,parent )
		ok.grab_default()
		
		self.bpdialog.show_all()
	
	def run(self):
		self.bpdialog.run()
		print "Closing Block Properties..."
		self.bpdialog.hide()
		 
	def update_FIXed_button(self, widget, vname, item):
		if self.buttonflag[vname] == 0:
			self.button[vname].set_image(gtk.image_new_from_stock("ascend-fixed-icon",gtk.ICON_SIZE_BUTTON)) 
			self.buttonflag[vname] = 1
		elif self.buttonflag[vname] == 1:
			self.button[vname].set_image(gtk.image_new_from_stock("ascend-free-icon",gtk.ICON_SIZE_BUTTON)) 
			self.buttonflag[vname] = 0
							 
	def FIX_button(self, widget,event, vname):
		self.button[vname].set_image(gtk.image_new_from_stock("ascend-fixed-icon",gtk.ICON_SIZE_BUTTON)) 
		self.buttonflag[vname] = 1 
			 
	def on_properties_ok_clicked(self, widget, buttonflag, item,parent):
		for k,v in item.blockinstance.params.iteritems():
			try:
				self.checkEntry(str(self.textbox[v.name].get_text()))
			except:
				m = gtk.MessageDialog(None, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, "Please enter proper values.")
				m.run()	
	   			m.destroy()
				self.run()
				return
			if buttonflag[v.name] == 1:
				v.fix = 1
				print 'fixed ' + item.blockinstance.name+'.'+v.name
			elif buttonflag[v.name] == 0:
				v.fix = 0
				print 'freed ' + item.blockinstance.name+'.'+v.name
			item.blockinstance.name = self.pname.get_text()			
			v.value = str(self.textbox[v.name].get_text())

		startiter = self.sb.get_start_iter()
		enditer = self.sb.get_end_iter()
		usertext = self.sb.get_text(startiter, enditer)	
		if usertext == "" :
			pass
		else:	
			item.blockinstance.usercode = usertext	
		item.blockinstance.tab = self.notebook.get_current_page()	
		parent.view.canvas.canvasmodelstate = 'Modified'
		parent.status.push(0,"Canvasmodel state : Modified")
		parent.view.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse('#FFF'))

	def checkEntry(self, _param_value):
		# match a float with option text afterwards, optionally separated by whitespace
		# temporary solution provided here, more powerful typechecking needed to be supported
		#try:
			_match = re.match(UNITS_RE,_param_value)
			#if not _match:
				#print _param_val
				#raise InputError("Not a valid value-and-optional-units")
				#parent.reporter.reportError("Not a valid value-and-optional-units")
			_val = _match.group(1) # the numerical part of the input
			_units = _match.group(5) # the text entered for units
			return str(_val)+'{'+str(_units)+'}'
				
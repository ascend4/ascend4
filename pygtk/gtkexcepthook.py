# vim: sw=4 ts=4:
#
# (c) 2003 Gustavo J A M Carneiro gjc at inescporto.pt
# 	2004-2005 Filip Van Raemdonck
#
# http://www.daa.com.au/pipermail/pygtk/2003-August/005775.html
# Message-ID: <1062087716.1196.5.camel@emperor.homelinux.net>
# 	"The license is whatever you want."

import inspect, linecache, pydoc, sys, traceback, webbrowser
from io import StringIO
from gettext import gettext as _
from smtplib import SMTP
#import os

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import Pango

#def analyse (exctyp, value, tb):
#	trace = StringIO()
#	traceback.print_exception (exctyp, value, tb, None, trace)
#	return trace

def lookup (name, frame, lcls):
	'''Find the value for a given name in the given frame'''
	if name in lcls:
		return 'local', lcls[name]
	elif name in frame.f_globals:
		return 'global', frame.f_globals[name]
	elif '__builtins__' in frame.f_globals:
		builtins = frame.f_globals['__builtins__']
		if type (builtins) is dict:
			if name in builtins:
				return 'builtin', builtins[name]
		else:
			if hasattr (builtins, name):
				return 'builtin', getattr (builtins, name)
	return None, []

def analyse (exctyp, value, tb):
	import tokenize, keyword

	trace = StringIO()
	nlines = 3
	frecs = inspect.getinnerframes (tb, nlines)
	trace.write ('Traceback (most recent call last):\n')
	for frame, fname, lineno, funcname, context, cindex in frecs:
		trace.write ('  File "%s", line %d, ' % (fname, lineno))
		args, varargs, varkw, lcls = inspect.getargvalues (frame)

		def readline (lno=[lineno], *args):
			if args: print(args)
			try: return linecache.getline (fname, lno[0])
			finally: lno[0] += 1
		all, prev, name, scope = {}, None, '', None
		for ttype, tstr, stup, etup, line in tokenize.generate_tokens (readline):
			if ttype == tokenize.NAME and tstr not in keyword.kwlist:
				if name:
					if name[-1] == '.':
						try:
							val = getattr (prev, tstr)
						except AttributeError:
							# XXX skip the rest of this identifier only
							break
						name += tstr
				else:
					assert not name and not scope
					scope, val = lookup (tstr, frame, lcls)
					name = tstr
				if val:
					prev = val
				#print '  found', scope, 'name', name, 'val', val, 'in', prev, 'for token', tstr
			elif tstr == '.':
				if prev:
					name += '.'
			else:
				if name:
					all[name] = (scope, prev)
				prev, name, scope = None, '', None
				if ttype == tokenize.NEWLINE:
					break

		trace.write (funcname +
		  inspect.formatargvalues (args, varargs, varkw, lcls, formatvalue=lambda v: '=' + pydoc.text.repr (v)) + '\n')
		trace.write (''.join (['    ' + x.replace ('\t', '  ') for x in [a for a in context if a.strip()]]))
		if len (all):
			trace.write ('  variables: %s\n' % str (all))

	trace.write ('%s: %s' % (exctyp.__name__, value))
	return trace

def _info (exctyp, value, tb):
	if exctyp.__name__=='KeyboardInterrupt':
		sys.stderr.write ('%s: %s\n' % (exctyp.__name__, value))
		sys.exit(1)

	trace = None
	dialog = Gtk.MessageDialog (parent=None, flags=0, type=Gtk.MessageType.WARNING, buttons=Gtk.ButtonsType.NONE)
	dialog.set_title (_("Bug Detected"))
#if Gtk.check_version (2, 4, 0) is not None:
#		dialog.set_has_separator (False)

	primary = _("<big><b>A programming error has been detected during the execution of this program.</b></big>")
	secondary = _("It probably isn't fatal, but should be reported to the developers nonetheless.")

	try:
		setsec = dialog.format_secondary_text
	except AttributeError:
		raise
		dialog.vbox.get_children()[0].get_children()[1].set_markup ('%s\n\n%s' % (primary, secondary))
		#lbl.set_property ("use-markup", True)
	else:
		del setsec
		dialog.set_markup (primary)
		dialog.format_secondary_text (secondary)

	try:
		email = feedback
		dialog.add_button (_("Report..."), 3)
	except NameError:
		# could ask for an email address instead...
		pass
	dialog.add_button (_("Details..."), 2)
	dialog.add_button (Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE)
	dialog.add_button (Gtk.STOCK_QUIT, 1)
	dialog.add_button (_("Report Bug"),3)

	while True:
		resp = dialog.run()
		if resp == 3:
#if trace == None:
#				trace = analyse (exctyp, value, tb)

			# TODO: prettyprint, deal with problems in sending feedback, &tc
			handle = webbrowser.get()
			print((dir(handle)))
			handle.open("http://bugs.ascend4.org/bug_report_page.php")
			if trace == None:
				trace = analyse (exctyp, value, tb)
			print((trace.getvalue()))
			

#			try:
#				server = smtphost
#			except NameError:
#				server = 'localhost'
#
#			message = 'From: buggy_application"\nTo: bad_programmer\nSubject: Exception feedback\n\n%s' % trace.getvalue()
#
#			s = SMTP()
#			s.connect (server)
#			s.sendmail (email, (email,), message)
#			s.quit()
			break

		elif resp == 2:
			if trace == None:
				trace = analyse (exctyp, value, tb)

			# Show details...
			details = Gtk.Dialog (_("Bug Details"), dialog,
			  Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
			  (Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE, ))
#details.set_property ("has-separator", False)

			textview = Gtk.TextView(); textview.show()
			textview.set_editable (False)
			textview.modify_font (Pango.FontDescription ("Monospace"))

			sw = Gtk.ScrolledWindow(); sw.show()
			sw.set_policy (Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
			sw.add (textview)
			details.vbox.add (sw)
			textbuffer = textview.get_buffer()

#			envtext = "\n".join(["%s = %s" % (k,os.environ[k]) for k in sorted(os.environ.keys())])
#			textbuffer.set_text (trace.getvalue() + "\n\n\nENVIRONMENT\n\n" +  envtext)
			textbuffer.set_text (trace.getvalue())
            
			screen = dialog.get_screen()
			monitor = screen.get_monitor_at_window (screen.get_active_window())
			area = screen.get_monitor_geometry (monitor)
			try:
				w = area.width // 1.6
				h = area.height // 1.6
			except SyntaxError:
				# python < 2.2
				w = area.width / 1.6
				h = area.height / 1.6
			sw.set_size_request(int(w),int(h/2))
			details.set_default_size (int (w), int (h))

			details.run()
			details.destroy()

		elif resp == 1 and Gtk.main_level() > 0:
			Gtk.main_quit()
			break
		else:
			break

	dialog.destroy()

sys.excepthook = _info

if __name__ == '__main__':
	class X (object):
		pass
	x = X()
	x.y = 'Test'
	x.z = x
	w = ' e'

	#feedback = 'developer@bigcorp.comp'
	#smtphost = 'mx.bigcorp.comp'
	1, x.z.y, f, w

	while 1:
		pass

	raise Exception (x.z.y + w)

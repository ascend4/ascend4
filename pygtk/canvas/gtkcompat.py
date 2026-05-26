"""Small GTK 2 compatibility facade backed by PyGObject/Gtk 3.

The canvas code predates PyGObject and uses many ``gtk.*`` constants and
``gtk.gdk`` helpers.  Keeping those names here makes the first Gtk3 port
mechanical and keeps the remaining application logic readable.
"""

from __future__ import annotations

from types import SimpleNamespace

import gi

gi.require_version("Gtk", "3.0")

try:
	gi.require_version("GtkSource", "3.0")
except (ValueError, ImportError):
	GtkSource = None
else:
	from gi.repository import GtkSource as GtkSource

from gi.repository import Gdk, GdkPixbuf, GObject, Gtk, Pango


class _BuilderXML:
	"""Tiny replacement for the old ``gtk.glade.XML`` wrapper."""

	def __init__(self, filename, root=None, domain=None, typedict=None):
		self.builder = Gtk.Builder()
		if root:
			self.builder.add_objects_from_file(filename, [root])
		else:
			self.builder.add_from_file(filename)

	def get_widget(self, name):
		return self.builder.get_object(name)

	def signal_autoconnect(self, obj):
		self.builder.connect_signals(obj)


class _GdkCompat(SimpleNamespace):
	def __init__(self):
		super().__init__(
			Pixbuf=GdkPixbuf.Pixbuf,
			Cursor=Gdk.Cursor,
			Color=Gdk.Color,
			BUTTON_PRESS=Gdk.EventType.BUTTON_PRESS,
			CONTROL_MASK=Gdk.ModifierType.CONTROL_MASK,
			SHIFT_MASK=Gdk.ModifierType.SHIFT_MASK,
			MOD1_MASK=Gdk.ModifierType.MOD1_MASK,
			BUTTON2_MASK=Gdk.ModifierType.BUTTON2_MASK,
			SCROLL_UP=Gdk.ScrollDirection.UP,
			SCROLL_DOWN=Gdk.ScrollDirection.DOWN,
			color_parse=Gdk.color_parse,
			keyval_name=Gdk.keyval_name,
			pixbuf_new_from_file=GdkPixbuf.Pixbuf.new_from_file,
			pixbuf_new_from_file_at_size=GdkPixbuf.Pixbuf.new_from_file_at_size,
		)


class _GtkCompat:
	gdk = _GdkCompat()
	glade = SimpleNamespace(XML=_BuilderXML)

	def __getattr__(self, name):
		return getattr(Gtk, name)


gtk = _GtkCompat()
gdk = gtk.gdk
gobject = GObject
pango = Pango


def _stock(name: str, fallback: str) -> str:
	return getattr(Gtk, name, fallback)


for _name, _value in {
	"STOCK_ABOUT": _stock("STOCK_ABOUT", "help-about"),
	"STOCK_CANCEL": _stock("STOCK_CANCEL", "gtk-cancel"),
	"STOCK_CLOSE": _stock("STOCK_CLOSE", "gtk-close"),
	"STOCK_CONVERT": _stock("STOCK_CONVERT", "gtk-convert"),
	"STOCK_DELETE": _stock("STOCK_DELETE", "gtk-delete"),
	"STOCK_DIALOG_ERROR": _stock("STOCK_DIALOG_ERROR", "dialog-error"),
	"STOCK_DIALOG_INFO": _stock("STOCK_DIALOG_INFO", "dialog-information"),
	"STOCK_DIALOG_WARNING": _stock("STOCK_DIALOG_WARNING", "dialog-warning"),
	"STOCK_EXECUTE": _stock("STOCK_EXECUTE", "gtk-execute"),
	"STOCK_FULLSCREEN": _stock("STOCK_FULLSCREEN", "view-fullscreen"),
	"STOCK_INFO": _stock("STOCK_INFO", "dialog-information"),
	"STOCK_NEW": _stock("STOCK_NEW", "document-new"),
	"STOCK_OK": _stock("STOCK_OK", "gtk-ok"),
	"STOCK_OPEN": _stock("STOCK_OPEN", "document-open"),
	"STOCK_PRINT": _stock("STOCK_PRINT", "document-print"),
	"STOCK_PRINT_PREVIEW": _stock("STOCK_PRINT_PREVIEW", "document-print-preview"),
	"STOCK_PROPERTIES": _stock("STOCK_PROPERTIES", "document-properties"),
	"STOCK_QUIT": _stock("STOCK_QUIT", "application-exit"),
	"STOCK_REDO": _stock("STOCK_REDO", "edit-redo"),
	"STOCK_SAVE": _stock("STOCK_SAVE", "document-save"),
	"STOCK_SAVE_AS": _stock("STOCK_SAVE_AS", "document-save-as"),
	"STOCK_UNDO": _stock("STOCK_UNDO", "edit-undo"),
	"STOCK_YES": _stock("STOCK_YES", "gtk-yes"),
	"STOCK_ZOOM_IN": _stock("STOCK_ZOOM_IN", "zoom-in"),
	"STOCK_ZOOM_OUT": _stock("STOCK_ZOOM_OUT", "zoom-out"),
	"STOCK_ZOOM_FIT": _stock("STOCK_ZOOM_FIT", "zoom-fit-best"),
	"ICON_SIZE_MENU": Gtk.IconSize.MENU,
	"RESPONSE_ACCEPT": Gtk.ResponseType.ACCEPT,
	"RESPONSE_APPLY": Gtk.ResponseType.APPLY,
	"RESPONSE_CANCEL": Gtk.ResponseType.CANCEL,
	"RESPONSE_CLOSE": Gtk.ResponseType.CLOSE,
	"RESPONSE_OK": Gtk.ResponseType.OK,
	"RESPONSE_YES": Gtk.ResponseType.YES,
	"DIALOG_DESTROY_WITH_PARENT": Gtk.DialogFlags.DESTROY_WITH_PARENT,
	"DIALOG_MODAL": Gtk.DialogFlags.MODAL,
	"MESSAGE_ERROR": Gtk.MessageType.ERROR,
	"MESSAGE_WARNING": Gtk.MessageType.WARNING,
	"BUTTONS_CLOSE": Gtk.ButtonsType.CLOSE,
	"FILE_CHOOSER_ACTION_OPEN": Gtk.FileChooserAction.OPEN,
	"FILE_CHOOSER_ACTION_SAVE": Gtk.FileChooserAction.SAVE,
	"FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME": Gtk.FileChooserConfirmation.ACCEPT_FILENAME,
	"FILE_CHOOSER_CONFIRMATION_CONFIRM": Gtk.FileChooserConfirmation.CONFIRM,
	"FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN": Gtk.FileChooserConfirmation.SELECT_AGAIN,
	"FILL": Gtk.AttachOptions.FILL,
	"POLICY_AUTOMATIC": Gtk.PolicyType.AUTOMATIC,
	"POS_TOP": Gtk.PositionType.TOP,
	"SHADOW_ETCHED_IN": Gtk.ShadowType.ETCHED_IN,
	"STATE_NORMAL": Gtk.StateType.NORMAL,
}.items():
	setattr(gtk, _name, _value)


if not hasattr(Gtk.Dialog, "vbox"):
	Gtk.Dialog.vbox = property(lambda self: self.get_content_area())

if not hasattr(Gtk.Widget, "modify_bg"):
	def _modify_bg(self, _state, _color):
		return None

	Gtk.Widget.modify_bg = _modify_bg


if GtkSource is not None:
	gtksourceview = SimpleNamespace(
		View=GtkSource.View,
		Buffer=GtkSource.Buffer,
		language_manager_get_default=GtkSource.LanguageManager.get_default,
	)
else:
	class _FallbackBuffer(Gtk.TextBuffer):
		def set_language(self, _language):
			pass

		def set_highlight_syntax(self, _enabled):
			pass

	class _FallbackLanguageManager:
		def __init__(self):
			self._search_path = []

		def get_search_path(self):
			return list(self._search_path)

		def set_search_path(self, search_path):
			self._search_path = list(search_path)

		def get_language(self, _language_id):
			return None

	gtksourceview = SimpleNamespace(
		View=Gtk.TextView,
		Buffer=_FallbackBuffer,
		language_manager_get_default=lambda: _FallbackLanguageManager(),
	)

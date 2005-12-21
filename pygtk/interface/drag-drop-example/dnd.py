# dndtester.py

import pygtk
pygtk.require("2.0")

import gtk
import gtk.glade

class DNDTester(object):
    def __init__(self):
        filename = 'dnd.glade'
        windowname = 'DNDTester'
        self.wTree = gtk.glade.XML(filename, windowname)
        self.log_buffer = gtk.TextBuffer()
        self.setupWidgets()
        
    def setupWidgets(self):
        HANDLERS_AND_METHODS = {
            "on_dndtester_destroy": self.destroy,
            "on_drag_data_received": self.on_log_drag_data_received
            }

        log = self.wTree.get_widget("log")
        log.set_buffer(self.log_buffer)
        self.wTree.signal_autoconnect(HANDLERS_AND_METHODS)

    def on_log_drag_data_received(self, data):
        self.log_buffer.insert_at_cursor(data+'\n', len(data))

    def destroy(self, data):
        gtk.mainquit()

if __name__ == "__main__":
    app = DNDTester()
    gtk.mainloop()

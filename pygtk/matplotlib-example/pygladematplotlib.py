import sys
import matplotlib
matplotlib.use('GTK')
 
from matplotlib.figure import Figure
from matplotlib.axes import Subplot
from matplotlib.backends.backend_gtk import FigureCanvasGTK, NavigationToolbar
 
from matplotlib.numerix import arange, sin, pi
 
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.glade
except:
    sys.exit(1)
 
host = "***"
user = "***"
passwd = "***"
db = "***"
 
class appGui:
    def __init__(self):
        gladefile = "project2.glade"
        self.windowname = "gtkbench"
        self.wTree = gtk.glade.XML(gladefile, self.windowname)
        self.win = self.wTree.get_widget("gtkbench")
        self.win.maximize()
        dic = {"on_window1_destroy" : gtk.main_quit,
            "on_button1_clicked" : self.submitDB,
            "on_button3_clicked" : self.fillTree,
            "on_notebook1_switch_page" : self.selectNotebookPage,
            "on_treeview1_button_press_event" : self.clickTree,
                "on_button2_clicked" : self.createProjectGraph
            }
         
        self.wTree.signal_autoconnect(dic)
        # start with database selection
        self.wDialog = gtk.glade.XML("project2.glade", "dbSelector")
         
        # setup matplotlib stuff on first notebook page (empty graph)
        self.figure = Figure(figsize=(6,4), dpi=72)
        self.axis = self.figure.add_subplot(111)
         
        self.axis.set_xlabel('Yepper')
        self.axis.set_ylabel('Flabber')
        self.axis.set_title('An Empty Graph')
        self.axis.grid(True)
     
        self.canvas = FigureCanvasGTK(self.figure) # a gtk.DrawingArea
        self.canvas.show()
        self.graphview = self.wTree.get_widget("vbox1")
        self.graphview.pack_start(self.canvas, True, True)
         
        # setup listview for database
        self.listview = self.wTree.get_widget("treeview1")
        self.listmodel = gtk.ListStore(str, int, int, str, str)
        self.listview.set_model(self.listmodel)
 
        renderer = gtk.CellRendererText()
         
        column = gtk.TreeViewColumn("Name",renderer, text=0)
        column.set_clickable(True)
        column.set_sort_column_id(0)
        column.connect("clicked", self.createDBGraph)
        column.set_resizable(True)
        self.listview.append_column(column)
        #renderer = gtk.CellRendererText()
         
        column = gtk.TreeViewColumn("Age",renderer, text=1)
        column.set_clickable(True)
        column.set_sort_column_id(1)
        column.connect("clicked", self.createDBGraph)
        column.set_resizable(True)
        self.listview.append_column(column)
        #self.listview.show()
         
        column = gtk.TreeViewColumn("Shoesize",renderer, text=2)
        column.set_clickable(True)
        column.set_sort_column_id(2)
        column.connect("clicked", self.createDBGraph)
        column.set_resizable(True)
        self.listview.append_column(column)
        #self.listview.show()
 
        column = gtk.TreeViewColumn("Created",renderer, text=3)
        column.set_clickable(True)
        column.set_sort_column_id(3)
        column.connect("clicked", self.createDBGraph)
        column.set_resizable(True)
        self.listview.append_column(column)
        #self.listview.show()
        #renderer = gtk.CellRendererText()
 
        column = gtk.TreeViewColumn("Updated",renderer, text=4)
        column.set_clickable(True)
        column.set_sort_column_id(4)
        column.connect("clicked", self.createDBGraph)
        column.set_resizable(True)
        self.listview.append_column(column)
 
 
        return
     
    # callbacks.
    def submitDB(self, widget):
        while True:
            try:
                name = self.wTree.get_widget("entry1").get_text()
                age = self.wTree.get_widget("entry2").get_text()
                size = self.wTree.get_widget("entry3").get_text()
                assert name != ""
                assert age != ""
                assert size != ""
                dataUsr = name, age, size                
                sd = DBStuff.Eb_db(host, user, passwd, db)
                sd.subMit(dataUsr)
                break
            except AssertionError:
                self.wDialog = gtk.glade.XML("project2.glade", "dbWarningEmpty")
                close = self.wDialog.get_widget("dbWarningEmpty")
                response = close.run()
                if response == gtk.RESPONSE_CLOSE:
                    close.destroy()
                break
            except DBStuff.MySQLdb.IntegrityError:
                def callback():
                    self.wDialog = gtk.glade.XML("project2.glade", "dbWarningOverwrite")
                    close = self.wDialog.get_widget("dbWarningOverwrite")
                    response = close.run()
                    if response == gtk.RESPONSE_CANCEL:
                        close.destroy()
                    if response == gtk.RESPONSE_OK:
                        sd.delRow(name)
                        wd = DBStuff.Eb_db(host, user, passwd, db)
                        wd.subMit(dataUsr)
                        close.destroy()
                callback()
                break
             
    def fillTree(self, widget):
        model = self.listmodel
        self.listmodel.clear()
        fg = DBStuff.Eb_db(host, user, passwd, db)
        fg.getData("Age")
 
        for i in range(len(fg.data)):
            # note: all data from table "bench" is appended, but that is something you don't want
            # possible solution: create a seperate table for the listbox (eg. ommit Timestamp, keep it in another table)
            self.listmodel.append(fg.data[i])
 
        self.createDBGraph(self)
         
    def selectNotebookPage(self, widget, notebookpage, page_number):
        # if database page is selected (nr. 1 for now!), retrieve data
        if page_number == 1:
            self.fillTree(self)
             
        if page_number == 0:
            print "clicked first tab"
         
    def clickTree(self, treeview, event):
        if event.button == 3:
            x = int(event.x)
            y = int(event.y)
            time = event.time
            pthinfo = treeview.get_path_at_pos(x, y)
            if pthinfo != None:
                path, col, cellx, celly = pthinfo
                treeview.grab_focus()
                treeview.set_cursor( path, col, 0)
                self.wDialog = gtk.glade.XML("project2.glade", "treeRightClick")
                close = self.wDialog.get_widget("treeRightClick")
                response = close.run()
                if response == gtk.RESPONSE_OK:
                    close.destroy()
 
                print x
                print y
                print pthinfo
                #self.popup.popup( None, None, None, event.button, time)
            return 1
         
    def createProjectGraph(self, widget):
         
        while True:
            try:
                # empty axis if neccesary, and reset title and stuff
                self.axis.clear()
                self.axis.set_xlabel('Yepper')
                self.axis.set_ylabel('Flabber')
                self.axis.set_title('A Graph')
                self.axis.grid(True)
 
                # get data
                age = self.wTree.get_widget("entry2").get_text()
                size = self.wTree.get_widget("entry3").get_text()
                age != ""
                size != ""
                 
                N = 1
                ind = arange(N)  # the x locations for the groups
                width = 0.35       # the width of the bars
                 
                p1 = self.axis.bar(ind, int(age), width, color='r')
                p2 = self.axis.bar(ind+width, int(size), width, color='y')
                 
                self.axis.legend((p1[0], p2[0]), ("Age", "Size"), shadow = True)
                #self.axis.set_xticks(ind+width, ('G1') )
                self.axis.set_xlim(-width,len(ind))
                 
                self.canvas.destroy()
                self.canvas = FigureCanvasGTK(self.figure) # a gtk.DrawingArea
                self.canvas.show()
                self.grahview = self.wTree.get_widget("vbox1")
                self.grahview.pack_start(self.canvas, True, True)
                break
             
            except ValueError:
                self.wDialog = gtk.glade.XML("project2.glade", "cannotCreateProjGraph")
                close = self.wDialog.get_widget("cannotCreateProjGraph")
                response = close.run()
                if response == gtk.RESPONSE_OK:
                    close.destroy()
                break
 
    def createDBGraph(self, widget):
        self.axis.clear()
        self.axis.set_xlabel('Samples (n)')
        self.axis.set_ylabel('Value (-)')
        self.axis.set_title('Another Graph (click on the columnheader to sort)')
        self.axis.grid(True)
 
        # get columns from listmodel
        age = []
        for row in self.listmodel:
            age.append(row[1])
        size = []
        for row in self.listmodel:
            size.append(row[2])
             
        # get number of rows
        N = len(age)
         
        ind = arange(N)  # the x locations for the groups
        width = 0.35       # the width of the bars
        p1 = self.axis.bar(ind, age, width, color='b')
        p2 = self.axis.bar(ind+width, size, width, color='r')
        # destroy graph if it already exists
        while True:
            try:
                self.canvas2.destroy()
                break
            except:
                print "nothing to destroy"
                break
             
        self.canvas2 = FigureCanvasGTK(self.figure) # a gtk.DrawingArea
        self.canvas2.show()
        self.grahview = self.wTree.get_widget("vbox2")
        self.grahview.pack_start(self.canvas2, True, True)
     
app = appGui()
gtk.main()
     
 

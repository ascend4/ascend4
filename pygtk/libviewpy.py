import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

class Browser:
    def make_row( self, piter, name, value ):
        info = repr(value)
        if not hasattr(value, "__dict__"):
            if len(info) > 80:
                # it's a big list, or dict etc. 
                info = info[:80] + "..."
        _piter = self.treestore.append( piter, [ name, type(value).__name__, info ] )
        return _piter

    def make_instance( self, value, piter ):
        if hasattr( value, "__dict__" ):
            for _name, _value in value.__dict__.items():
                _piter = self.make_row( piter, "."+_name, _value )
                _path = self.treestore.get_path( _piter )
                self.otank[ _path.to_string() ] = (_name, _value)

    def make_children(self, value, piter ):
		children = []
		if hasattr(value,"children"):
			children=value.children;
		for child in children:
			_name = child.name;
			_piter = self.make_row(piter,_name,child)
			_path = self.treestore.get_path(_piter)
			self.otank[_path.to_string()]=(_name,child)

    def make_mapping( self, value, piter ):
        keys = []
        if hasattr( value, "keys" ):
            keys = value.keys()
        elif hasattr( value, "__len__"):
            keys = range( len(value) )
        for key in keys:
            _name = "[%s]"%str(key)
            _piter = self.make_row( piter, _name, value[key] )
            _path = self.treestore.get_path( _piter )
            self.otank[ _path.to_string() ] = (_name, value[key])

    def make(self, name=None, value=None, path=None, depth=1):
        if path is None:
            # make root node
            piter = self.make_row( None, name, value )
            path = self.treestore.get_path( piter )
            self.otank[ path.to_string() ] = (name, value)
        else:
            name, value = self.otank[ path.to_string() ]

        piter = self.treestore.get_iter( path )
        if not self.treestore.iter_has_child( piter ):
            #self.make_mapping( value, piter )
            #self.make_instance( value, piter )
            self.make_children(value,piter)

        if depth:
            for i in range( self.treestore.iter_n_children( piter ) ):
                path.append_index(i)
                self.make( path=path, depth = depth - 1 )

    def row_expanded( self, treeview, piter, path ):
        self.make( path = path )

    def delete_event(self, widget, event, data=None):
        Gtk.main_quit()
        return False

    def __init__(self, name, value):
        self.window = Gtk.Window(Gtk.WindowType.TOPLEVEL)
        self.window.set_title("Browser")
        self.window.set_size_request(512, 320)
        self.window.connect("delete_event", self.delete_event)

        # we will store the name, the type name, and the repr 
        columns = [str,str,str]
        self.treestore = Gtk.TreeStore(*columns)

        # the otank tells us what object we put at each node in the tree
        self.otank = {} # map path -> (name,value)
        self.make( name, value )

        self.treeview = Gtk.TreeView(self.treestore)
        self.treeview.connect("row-expanded", self.row_expanded )

        self.tvcolumns = [ Gtk.TreeViewColumn() for _type in columns ]
        i = 0
        for tvcolumn in self.tvcolumns:
            self.treeview.append_column(tvcolumn)
            cell = Gtk.CellRendererText()
            tvcolumn.pack_start(cell, True)
            tvcolumn.add_attribute(cell, 'text', i)
            i = i + 1

        self.window.add(self.treeview)
        self.window.show_all()

def dump( name, value ):
    browser = Browser( name, value )
    Gtk.main()

def test():
    class Thing:
		def __init__(self,name,value):
			self.children = []
			self.name=name
			self.value=value
			self.index=0
		def findChildren(self):
			print "FINDING CHILDREN OF ", self.name
		def addChild(self,thing):
			print thing.name,"is a child of ",self.name
			self.children.append(thing);
		def __repr__(self):
			return self.name
		def __iter__(self): 
			for i in range(len(self.children)):
				yield self.children[i]

    a = Thing("a","A value")
    b = Thing("b","B Another value")
    c = Thing("c","C Another value")
    d = Thing("d","D Yet another value")
    a.addChild(b)
    b.addChild(c)
    c.addChild(d)
    d.addChild(a) # circular chain
    dump( "a", a )

if __name__ == "__main__":
    test()

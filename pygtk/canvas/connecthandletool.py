import cairo
from gaphas.tool import HandleTool
from gaphas.connector import *

class DisconnectHandle(object):

    def __init__(self, item, handle):
        self.canvas = item.canvas
        self.item = item
        self.handle = handle

    def __call__(self):
        self.handle_disconnect()

    def handle_disconnect(self):
        canvas = self.canvas
        item = self.item
        handle = self.handle
        try:
            canvas.solver.remove_constraint(handle.connection_data)
        except KeyError:
            print 'constraint was already removed for', item, handle
            pass # constraint was alreasy removed
        else:
            print 'constraint removed for', item, handle
        handle.connection_data = None
        handle.connected_to = None
        # Remove disconnect handler:
        handle.disconnect = None

class ConnectHandleTool(HandleTool):
    """
    Modified version of ConnectHandleTool (see Gaphas `tool.py`) that allows 
    for undefined item and handle in the `glue` method.
    """
    # distance between line and item
    GLUE_DISTANCE = 10

    def glue(self, view, line, handle, vpos):
        """
        Find an item for connection with a line.

        Method looks for items in glue rectangle (which is defined by
        ``vpos`` (vx, vy) and glue distance), then finds the closest port.

        Glue position for closest port is calculated as well. Handle of
        a line is moved to glue point to indicate that connection is about
        to happen.

        Found item and its connection port are returned. If item is not
        found nor suitable port, then tuple `(None, None)` is returned.

        :Parameters:
         view
            View used by user.
         line
            Connecting item.
         handle
            Handle of line (connecting item).
        """
        if handle and not handle.connectable:
            return None

        item, port, gluepos = self.find_connectable_port(view, vpos)

        # if gluable port found...
        if port is not None:
            # check if line and found item can be connected on closest port
            if not self.can_glue(view, line, handle, item, port):
                item, port = None, None

            if line is not None and handle is not None:
                # transform coordinates from view space to the line space and
                # update position of line's handle
                v2i = view.get_matrix_v2i(line).transform_point
                handle.pos = v2i(*glue_pos)
        # else item and port will be set to None

        return item, port

    def find_connectable_port(self, view, vpos):
        dist = self.GLUE_DISTANCE
        max_dist = dist
        port = None
        glue_pos = None
        item = None
        v2i = view.get_matrix_v2i
        vx, vy = vpos

        rect = (vx - dist, vy - dist, dist * 2, dist * 2)
        items = view.get_items_in_rectangle(rect, reverse=True)
        for i in items:
            if i is line:
                continue
            for p in i.ports():
                if not p.connectable:
                    continue

                ix, iy = v2i(i).transform_point(vx, vy)
                pg, d = p.glue((ix, iy))

                if d >= max_dist:
                    continue

                item = i
                port = p

                # transform coordinates from connectable item space to view
                # space
                i2v = view.get_matrix_i2v(i).transform_point
                glue_pos = i2v(*pg)

        return item, port, glue_pos

    def can_glue(self, view, line, handle, item, port):
        """
        Determine if line's handle can connect to a port of an item.

        `True` is returned by default. Override this method to disallow
        glueing in higher level of application stack (i.e. when classes of
        line and item does not match).

        :Parameters:
         view
            View used by user.
         line
            Item connecting to connectable item.
         handle
            Handle of line connecting to connectable item.
         item
            Connectable item.
         port
            Port of connectable item.
        """
        return True


    def post_connect(self, line, handle, item, port):
        """
        The method is invoked just before connection is performed by
        `ConnectHandleTool.connect` method. It can be overriden by deriving
        tools to perform connection in higher level of application stack.

        :Parameters:
         line
            Item connecting to connectable item.
         handle
            Handle of line connecting to connectable item.
         item
            Connectable item.
         port
            Port of connectable item.
        """
        pass


    def connect(self, view, line, handle, vpos):
        """
        Connect a handle of a line to connectable item.

        Connectable item is found by `ConnectHandleTool.glue` method.

        :Parameters:
         view
            View used by user.
         line
            Connecting item.
         handle
            Handle of connecting item.
        """
        # find connectable item and its port
        item, port = self.glue(view, line, handle, vpos)

        # disconnect when
        # - no connectable item
        # - currently connected item is not connectable item
        if not item \
                or item and handle.connected_to is not item:
            handle.disconnect()

        # no connectable item, no connection
        if not item:
            return

        # low-level connection
        self.connect_handle(line, handle, item, port)
        # connection in higher level of application stack
        self.post_connect(line, handle, item, port)


    def connect_handle(self, line, handle, item, port):
        """
        Create constraint between handle of a line and port of connectable
        item.

        :Parameters:
         line
            Connecting item.
         handle
            Handle of connecting item.
         item
            Connectable item.
         port
            Port of connectable item.
        """
        ConnectHandleTool.create_constraint(line, handle, item, port)

        handle.connected_to = item
        handle.disconnect = DisconnectHandle(line, handle)


    def disconnect(self, view, line, handle):
        """
        Disconnect line (connecting item) from an item.

        :Parameters:
         view
            View used by user.
         line
            Connecting item.
         handle
            Handle of connecting item.
        """
        if handle.disconnect:
            handle.disconnect()


    @staticmethod
    def find_port(line, handle, item):
        """
        Find port of an item at position of line's handle.

        :Parameters:
         line
            Line supposed to connect to an item.
         handle
            Handle of a line connecting to an item.
         item
            Item to be connected to a line.
        """
        port = None
        max_dist = sys.maxint
        canvas = item.canvas

        # line's handle position to canvas coordinates
        i2c = canvas.get_matrix_i2c(line)
        hx, hy = i2c.transform_point(*handle.pos)

        # from canvas to item coordinates
        c2i = canvas.get_matrix_c2i(item)
        ix, iy = c2i.transform_point(hx, hy)

        # find the port using item's coordinates
        for p in item.ports():
            pg, d = p.glue((ix, iy))
            if d >= max_dist:
                continue
            port = p
            max_dist = d

        return port


    @staticmethod
    def create_constraint(line, handle, item, port):
        """
        Create connection constraint between line's handle and item's port.

        If constraint already exists, then it is removed and new constraint
        is created instead.

        :Parameters:
         line
            Line connecting to an item.
         handle
            Handle of a line connecting to an item.
         item
            Item to be connected to a line.
         port
            Item's port used for connection with a line.
        """
        canvas = line.canvas
        solver = canvas.solver

        if handle.connection_data:
            solver.remove_constraint(handle.connection_data)

        constraint = port.constraint(canvas, line, handle, item)
        handle.connection_data = constraint
        solver.add_constraint(constraint)


    @staticmethod
    def remove_constraint(line, handle):
        """
        Remove connection constraint created between line's handle and
        connected item's port.

        :Parameters:
         line
            Line connecting to an item.
         handle
            Handle of a line connecting to an item.
        """
        if handle.connection_data:
            line.canvas.solver.remove_constraint(handle.connection_data)
            handle.connection_data = None

class PortHandleTool(ConnectHandleTool):
    """
    Subclass of ConnectHandleTool that records connection information in the
    LineInstance of a Line.
    """
    def post_connect(self,line, handle, item, port):
        # this handle belongs to the line.
        pass

# vim: sw=4:et:ai

#!/usr/bin/env python
'''
Undo/Redo support for Canvas Based Graphical Modeller for ASCEND.

Author: Grivan Thapar, June 2010

Gaphas implements a basic functionality of storing all the changes in the Gaphas Classes and providing a mechanism of
adding an 'observer' that observes these changes through a set of observers.
In addition to that it can emit callable signals which enable actual undoing.
What remains is to classify changes as a transaction. This module does that work.
Between every two user interactions a Transaction is saved.

TODO ::
    1. For now there is no End of Transaction, the End of Transaction is marked by start of another Transaction.
    Any Transaction is recorded between 2 mouse events. Or maybe this is not needed.
    2. Use /tmp or swap to store the Transaction undo/redo lists, reduce program memory load.
    3. Implement Redo.
'''

import threading

from gaphas.state import observers,revert_handler,subscribers,saveapply
from decorator import decorator
from gaphas.tool import Tool


mutex = threading.Lock()

block_observers = set()


def block_observed(func):
    # Almost copied from gaphas.state
    """
    Simple observer, dispatches events to functions registered in the observers
    list.

    On the function an ``__observer__`` property is set, which references to
    the observer decorator. This is necessary, since the event handlers expect
    the outer most function to be returned (that's what they see).

    Also note that the events are dispatched *before* the function is invoked.
    This is an important feature, esp. for the reverter code.
    """
    def wrapper(func, *args, **kwargs):
        o = func.__observer__
        acquired = mutex.acquire(False)
        try:
            if acquired:
                block_dispatch((o, args, kwargs), queue=block_observers)
            return func(*args, **kwargs)
        finally:
            if acquired:
                mutex.release()
    dec = decorator(wrapper)(func)
    func.__observer__ = dec
    return dec


def block_dispatch(event, queue):
    # Copied from gaphas.state
    """
    Dispatch an event to a queue of event handlers.
    Event handlers should have signature: handler(event).

    >>> def handler(event):
    ...     print 'event handled', event
    >>> observers.add(handler)
    >>> @observed
    ... def callme():
    ...     pass
    >>> callme() # doctest: +ELLIPSIS
    event handled (<function callme at 0x...>, (), {})
    >>> class Callme(object):
    ...     @observed
    ...     def callme(self):
    ...         pass
    >>> Callme().callme() # doctest: +ELLIPSIS
    event handled (<function callme at 0x...), {})
    >>> observers.remove(handler)
    >>> callme()
    """
    for s in queue:
        s(event)


class UndoMonitorTool(Tool):
    '''
    This tool captures all the mouse button press events, returns False
    '''
    def __init__(self, view=None):
        super(UndoMonitorTool, self).__init__(view)

    @block_observed
    def on_button_press(self, event):
        return False

    @block_observed
    def on_button_release(self, event):
        return False

    @block_observed
    def on_double_click(self, event):
        return False

    @block_observed
    def on_triple_click(self, event):
        return False


class Transaction(object):
    '''
    An Action stack, consisting of Gaphas revert handlers
    '''
    def __init__(self, event):
        self.event = event
        self.actions = []
        #print self.event

    def add(self, action):
        self.actions.append(action)

    def can_execute(self):
        return self.actions and True or False

    def execute(self):
        # type: () -> object
        #print '1', self.actions
        self.actions.reverse()
        #print '2', self.actions

        for action in self.actions:
            try:
                #print action
                action()
            except Exception, e:
                print 'Undo Error: ', e
'''
    def anti_execute(self):
        # type: () -> object
        # print self.actions
        self.actions.reverse()
        for action in self.actions:
            try:
                print "event: ", self.event
                # print action
                action()
            except Exception, e:
                print 'Undo Error: ', e
'''


class UndoManager(object):
    '''
    Transaction manager, provides encapsulation for gaphas.state
    '''
    def __init__(self, app):
        self._undo_stack = []
        self._redo_stack = []
        self._current_transaction = None
        self.app = app
        self.block_observers = block_observers
        self._stack_depth = 20

        self.undo_observers = block_observers

    def start(self):
        self.en_gaphas_state()
        self.block_observers.add(self._block_transaction_handler)
        #print self.block_observers

    def undo_observe(self):
        pass

    def reset(self):
        del self._undo_stack[:]
        del self._redo_stack[:]
        self._current_transaction = None

    def shutdown(self):
        del self._undo_stack[:]
        del self._redo_stack[:]
        self._current_transaction = None
        self.block_observers.clear()
        observers.clear()
        subscribers.clear()

    def en_gaphas_state(self):
        observers.add(revert_handler)
        subscribers.add(self._gaphas_event_handler)

    def dis_gaphas_state(self):
        observers.remove(revert_handler)
        subscribers.remove(self._gaphas_event_handler)

    def clear_undo_stack(self):
        self._undo_stack = []
        self._current_transaction = None

    def clear_redo_stack(self):
        del self._redo_stack[:]

    def begin_transaction(self, block_event=None):
        assert self._current_transaction == None
        self._current_transaction = Transaction(block_event or None)

    def commit_transaction(self):

        assert self._current_transaction is not None

        if self._current_transaction.can_execute():
            self._undo_stack.append(self._current_transaction)
        else:
            pass
        self._current_transaction = None

    def _gaphas_event_handler(self, event):
        try:
            self._current_transaction.add(lambda: saveapply(*event))
        except Exception as e:
            pass

    def _block_transaction_handler(self, event):
        if self._current_transaction:
            self.commit_transaction()
        self.begin_transaction(event)

    def undo(self):
        ''' previous code - changed by Mike June17 2016
        if self._current_transaction:
            self.commit_transaction()

        if len(self._undo_stack) == 0:
            self.app.status.push(0,"Undo Unavailable")
            return

        transaction = self._undo_stack.pop()
        undo_stack = list(self._undo_stack)
        redo_stack = list(self._redo_stack)
        self._undo_stack = []

        try:
            transaction.execute()
        except Exception as e:
            print 'Undo Error: \n',e
        finally:
            self._redo_stack = redo_stack
            if self._undo_stack:
                self._redo_stack.extend(self._undo_stack)
            self._undo_stack = undo_stack

        while len(self._redo_stack) > self._stack_depth:
            del self._redo_stack[0]

        self.app.status.push(0,"Undid Previous Action")
        '''
        # TODO: redo have been implemented, but undo haven't yet
        if self._current_transaction:
            self.commit_transaction()

        if len(self._undo_stack) == 0:
            self.app.status.push(0, "Undo Unavailable")
            return

        transaction = self._undo_stack.pop()
        undo_stack = list(self._undo_stack)
        redo_stack = list(self._redo_stack)
        redo_stack.append(transaction)

        try:
            transaction.execute()
        except Exception as e:
            print 'Undo Error: \n', e
        finally:
            self._redo_stack = redo_stack
            self._undo_stack = undo_stack

        while len(self._redo_stack) > self._stack_depth:
            del self._redo_stack[0]

        self.app.status.push(0, "Undid Previous Action")

    def redo(self):
        # TODO: redo haven't been fully implemented
        if len(self._redo_stack) == 0:
            self.app.status.push(0, "Redo Unavailable")
            return

        transaction = self._redo_stack.pop()

        undo_stack = list(self._undo_stack)
        redo_stack = list(self._redo_stack)
        undo_stack.append(transaction)

        try:
            transaction.execute()
        except Exception as e:
            print 'Redo Error: \n', e
        finally:
            self._undo_stack = undo_stack
            self._redo_stack = redo_stack

        while len(self._undo_stack) > self._stack_depth:
            del self._undo_stack[0]

        self.app.status.push(0, "Redid Previous Action")
        return

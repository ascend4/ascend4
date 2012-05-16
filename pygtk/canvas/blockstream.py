#Type level information and other data of streams

import ascpy 

class BlockStream(object):
    def __init__(self,typedesc,notesdb):
        self.type = typedesc
        self.notesdb = notesdb
        self.stream_properties = {}
        
        nn = notesdb.getTypeRefinedNotesLang(self.type,ascpy.SymChar("inline"))
        
        for n in nn:
            self.stream_properties[n.getId()]=[]
                     
    def reattach_ascend(self,library, notesdb):
        raise NotImplemented
        
    def __getstate__(self):
        raise NotImplemented
    
    def __setstate(self):
        raise NotImplemented
    
    def __str__(self):
        return str(self.type)
    
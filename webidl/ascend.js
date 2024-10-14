#
# pseudocode for javascript derived from pygtk driver

#-----------------------------------------------------------------------
# Get locations of the installed files. On windows, these are defined in
# in the registry. On other systems, these are defined at compile time
# by @VARNAME@ substitution.

SEP = ":"

# ASCEND_PYTHON = "@INSTALL_PYTHON_ASCEND@"
# all the following must exist in some way relative to a base url,
# or in the web users local cache
# LDPATHVAR = 'LD_LIBRARY_PATH'
# INSTALL_LIB="@INSTALL_LIB@"
# INSTALL_ASCDATA="@INSTALL_ASCDATA@"
# INSTALL_MODELS = "@INSTALL_MODELS@"
# INSTALL_SOLVERS = "@INSTALL_SOLVERS@"
# DEFAULT_ASCENDLIBRARY="""@DEFAULT_ASCENDLIBRARY@"""

#-----------------------------------------------------------------------

# print("Running with...")
# print(("   %s = %s" % (LDPATHVAR, os.environ.get(LDPATHVAR))))
# print(("   sys.path = %s" % sys.path))
# print(("   argv = %s" % sys.argv))
# for e in os.environ:
# 	print(("   %s = %s" % (e, os.environ[e])))
#
#import ascpy
#from gtkbrowser import *
# B = Browser( librarypath=os.environ.get('ASCENDLIBRARY') ,assetspath=os.path.join(INSTALL_ASCDATA,"glade"))
# B.run()
#
# we next dig into pygtk/gtkbrowser:Browser to see about what to wrap first 

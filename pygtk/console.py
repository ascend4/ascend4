# CONSOLE ACCESS to ASCEND from PYTHON

argv=['-gthread','-pi1','In <\\#>:','-pi2','   .\\D.:','-po','Out<\\#>:','-noconfirm_exit']
banner = "\n\n>>> ASCEND PYTHON CONSOLE: type 'help(ascpy)' for info, ctrl-D to resume ASCEND"
exitmsg = '>>> CONSOLE EXIT'

try:
	from IPython.Shell import IPShellEmbed
	have_ipython=True;
	embed = IPShellEmbed(argv,banner=banner,exit_msg=exitmsg);
except ImportError,e:
	have_ipython=False;
	embed = lambda:None

def start(browser):
	embed();


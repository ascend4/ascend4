
Syntax highlighting for ASCEND files in VIM
-------------------------------------------

I learnt VIM on some crumby old green-on-black terminals at University of Melbourne.
They were slow and only barely usable. But somehow, the habit of using VI (and now
VIM) stuck, and I still like to use it for quick edits.

If you also like VIM, place the ascend.vim syntax file in the correct place on your
system (probably /usr/share/vim/syntax or something like that). You also 
need to register the syntax file with a change to your ~/.vimrc file.

If you use MinGW/MSYS then you can use the example .vimrc file (vimrc-mingw.txt)
which will enable colour syntax highlighting. It seems to work for both the 'mintty'
terminal as well as for the 'cygwin' (regular MSYS command-line) terminal. Enjoy!

UPDATE (Jul 2016): The correct location for syntax files in Ubuntu 16.04 is 
/usr/share/vim/vim74/syntax/
Also, the file 'vimrc-mingw.txt' works fine for Linux, too:
cat vimrc-mingw.txt >> ~/.vimrc

UPDATE (Sep 2017): On Ubuntu 17.04, the file 'ascend.vim' goes into
/usr/share/vim/vim80/syntax/

-- 
John Pye
Feb 2012


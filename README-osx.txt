README for Mac OS X Users
========================

This is ASCEND for Mac OS X. If you are reading this file, we will assume
that you have managed to download the ASCEND .dmg file, and you're
reading this file from the folder that has appeared on your machine.

To complete installation of ASCEND, you need to drag the "ASCEND"
application icon shown beside this file into your applications folder.
We've provided a convenient alias for next to this file as well, to 
make that easy for you.

Running ASCEND
--------------

Once you've dragged ASCEND to your Applications folder, you can start using 
ASCEND right away.

The only tricky thing with ASCEND on Mac is that the usual "Model Library"
is a bit hard to locate. All the model files are embedded within the
Application. To see them you need to open the Applications folder, then
right-click (or ctrl-click) on ASCEND and hit "Show Package Contents".
A new folder will open up. Click on "Contents" then click on "Models". You
will now be looking at the ASCEND Model Library. You can open any of 
these files using ASCEND.

TODO
----

ASCEND still doesn't register its file-types, so double-click of .a4c or
.a4l files won't work yet.

It's not sure if we'll ever be able to make that work, as it seems that 
OS X uses 'Apple Events' to open files, and GTK+ and PyGTK might not
understand those things.

There is as-yet no recommended tool for editing ASCEND model files on Mac.

We need a way of checking the configuration settings for a particular
system.

-- 
John Pye
24 Sept 2009.

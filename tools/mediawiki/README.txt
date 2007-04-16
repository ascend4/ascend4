
MediaWiki support for Andre Simon's 'highlight' program
=======================================================

This directory contains files to provide support in MediaWiki for Andre Simon's
'highlight' program, a source code syntax highlighting program written in C++
that provides support for over 120 different programming languages.

http://www.andre-simon.de/


Installation
------------

To install this extension, download the files and place them in your MediaWiki
'extensions' folder (on my system, it's /var/www/wiki/extensions'). You might
want to keep them self-contained, eg /var/www/wiki/extensions/ASHighlight.

Then, edit your LocalSettings.php file to include the ashighlight.php file, eg

require_once( "{$IP}/extensions/ASHighlight/ashighlight.php" );


Usage
-----

Insert source code into your wiki pages using the following syntax:

<source lang="LANGNAME">
...
</source>

where LANGNAME is the normal file extension used by source code in the language
you are using. If you choose an invalid LANGNAME value, your wiki page will
show a list of the languages supported on your machine.

There is also experimental support for the following additional parameters. 
Please try them out and report any problems.
 * tabwidth=N (replace all tabs with N spaces)
 * line=1 (add line numbers)
 * start=N (start line numbers at N)


Known limitations
-----------------

If you try out this extension and have any problems with it, please let me know:
Mail to john at curioussymbols. dot c o m.

Currently, there are likely to be problems with using multiple different
languages on a single web page, due to the way that CSS tags are currently
being added to the page. If you know how to fix this on MediaWiki 1.9, please
let me know.







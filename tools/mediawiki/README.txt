
MediaWiki support for Andre Simon's 'highlight' program
=======================================================

This directory contains files to provide support in MediaWiki for Andre Simon's
'highlight' program, a source code syntax highlighting program written in C++
that provides support for over 120 different programming languages.

http://www.andre-simon.de/


Installation (MediaWiki 1.43)
-----------------------------

To install this extension, download the files and place them in your MediaWiki
`extensions` folder (for example, `/var/www/wiki/extensions/ASHighlight`).

Then, edit your `LocalSettings.php` file to load the extension:

wfLoadExtension( 'ASHighlight' );

Configuration options (in `LocalSettings.php`):
* $wgASHighlightBinary (default: /usr/bin/highlight)
* $wgASHighlightLangRoot (default: /usr/share/highlight/langDefs)
* $wgASHighlightTimeout (default: 10 seconds)
* $wgASHighlightTempDir (default: null, uses wfTempDir())

CSS theme
---------

The ResourceLoader module uses `resources/ashighlight.css`. To match the theme
used by your installed `highlight`, regenerate that file with:

  /usr/bin/highlight --style-outfile=resources/ashighlight.css --syntax=txt

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
Mail to john at curioussymbols. dot c o m. Or file a bug via github.com/ascend4.

If your highlight `.lang` files are in a non-standard location, set
`$wgASHighlightLangRoot` so the extension can list valid `lang=` values.






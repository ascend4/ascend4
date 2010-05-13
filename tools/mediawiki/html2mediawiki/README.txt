This folder contains some scripts used to rescue a wiki from Google's
Cache. We had a disaster with our wiki recently and this cod is being
used to attempt to ressurect our wiki to the extent that's possible.

Goals:
 - make use of a bunch of HTML grabbed from Google Cache and saved
   as separate files.

 - take thos files and strip out the Mediawiki header and footer as
   well as the Google Cache header.

 - check that the page's title correspond to the page name and give
   errors if not (maybe pages were save incorrectly)

 - Remove the 'contents' navigation block

 - Convert headings

 - Convert bold, italics

 - Convert numbered and unnumbered lists

 - Convert ALT text from math images back into Math markup

 - Deal with <ref> citations

 - Deal with HTML tables

 - Preserve ungrokable stuff as <pre> text.

 - Try to recognise code snippets with syntax highlighting?

 - Anything we can do to recognize when <nowiki> might be necessary?



This folder contains robots.txt and some URLS to test the robots.txt file.
We use this file to prevent Google from going crazy while indexing our
ViewVC code repository.

The problem essentially is the ViewVC allows diffs to be generated for every
version of a file with all other earlier versions. This means that as the
repository grows, the Google index of this size grows exponentially (or
is it O(N²)...?) in size.


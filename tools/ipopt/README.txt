The files in this directory can be used for packaging of a recent version of
IPOPT for Fedora (tested with Fedora 17) and Ubuntu.

See the file 'debian/changelog' to found out which version of IPOPT is currently supported for DEB builds.

See ipopt.spec for the IPOPT version supported for RPM builds.

http://ascend4.org/IPOPT
http://www.coin-or.org/download/source/Ipopt/

This packaging hasn't recently been tested (since ~2012), and may require some tweaking.

IMPORTANT NOTE:
The .spec file is not present in the distribution tarball, due to problems with 'rpmbuild -ta'
with tarballs containing multiple .spec files. Please access the .spec file in our code repository.

-- 
John Pye
25 Jun 2014


# This file is part of the Ascend modeling library to
# demonstrate octest.asc and is released under the GNU
# Public License as noted at the beginning of octest.asc.
# $Id: octest.s,v 1.1 1997/02/22 22:27:09 ballan Exp $

DELETE TYPES;
READ FILE octest.asc;
COMPILE oct noc;
RUN oct.st.reset;
SOLVE oct.st;
RUN oct.rc.reset;
SOLVE oct.rc;

# here we need to check for correct numeric answers.

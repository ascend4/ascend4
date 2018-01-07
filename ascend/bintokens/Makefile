# This makefile is used by the Tcl/Tk GUI to build bintokens. It can also be
# enabled by changing the buildcommand in BinTokenSetOptions (see 
# ascend/compoiler/bintoken.[ch]). 2018, we added BinTokenSetOptionsDefault
# which tries to create a portable way to run the bintokens build that doesn't
# require so much per-system customisation. This new approach runs 'gcc'
# directly, and doesn't use this makefile any more. But you can still call
# BinTokenSetOptions and still use this makefile if you wish to -- JP.

CC=gcc
MAKEFILE_PATH:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

all:
	$(CC) -shared -fPIC -I. -I$(MAKEFILE_PATH) -o$(ASCBT_TARGET) $(ASCBT_SRC) -lascend -L.


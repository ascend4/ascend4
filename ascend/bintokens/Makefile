ASC_INCDIR=$(HOME)/ascend/python3
ASC_LIBDIR=$(HOME)/ascend/python3
CC=gcc

all:
	$(CC) -shared -I$(ASC_INCDIR) -o $(ASCBT_TARGET) $(ASCBT_SRC) -lascend -L$(ASC_LIBDIR)


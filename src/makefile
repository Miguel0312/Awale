IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR =../lib

_DEPS_CLIENT = client.h Awale.h LinkedList.h
DEPS_CLIENT = $(patsubst %,$(IDIR)/%,$(_DEPS_CLIENT))

_DEPS_SERVER = server.h Awale.h LinkedList.h
DEPS_SERVER = $(patsubst %,$(IDIR)/%,$(_DEPS_SERVER))

_OBJ_CLIENT = client.o Awale.o LinkedList.o
OBJ_CLIENT = $(patsubst %,$(ODIR)/%,$(_OBJ_CLIENT))

_OBJ_SERVER = server.o Awale.o LinkedList.o
OBJ_SERVER = $(patsubst %,$(ODIR)/%,$(_OBJ_SERVER))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS)

server: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ 
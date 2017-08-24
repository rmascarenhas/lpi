CC = cc
CFLAGS = -g -O2  -Wall -Wextra

LIBOBJ = tsbintree.o
OBJ = sems.o parser.o ds.o
BIN = nv

all: nv

sems.o: sems.h sems.c
parser.o: parser.h parser.c
ds.o: sems.o ds.h ds.c

nv: sems.o parser.o ds.o nv.c
	$(CC) -o $(BIN) $(CFLAGS) nv.c $(OBJ)

clean:
	rm -fv *.o $(BIN)

.PHONY: run clean

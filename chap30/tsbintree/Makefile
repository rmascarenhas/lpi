CC = cc
CFLAGS = -g -O2 -I. -Wall -Wextra $(DEFINES)

LIBOBJ = tsbintree.o
OBJ = $(LIBOBJ) console.o
TEST_OBJ = test/threaded_operations.o
TEST_BIN = test/threaded_operations

all: console

tsbintree.o: tsbintree.h tsbintree.c
console.o: tsbintree.h console.c

console: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run:
	@./console

check: $(LIBOBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(LIBOBJ) $(TEST_OBJ) -lpthread
	@./$(TEST_BIN)

clean:
	rm -fv {test/,}*.o console $(TEST_BIN)

.PHONY: run clean

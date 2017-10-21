## Makefile for Anakata
## Author- Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com)
CC = g++
CC_OPTIONS = -ggdb3 -Wall -std=c++11 -lpthread
CFLAGS = $(CC_OPTIONS)

ODIR = obj
BUILD = build
BIN = bin
SDIR = src
CLIENT = KatClient
SERVER = KatServ

.PHONY: clean cleano client server

all: client server

run:
	$(CC) $(CC_OPTIONS) -o $(EXE) $(SRC)
	./$(EXE)

config:
	$(CC) $(CC_OPTIONS) $(SDIR)/configure.cpp -o $(BUILD)/$@
	cd $(BUILD); ./$@

# this target compiles and links the client
client: clean
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(CLIENT)/KatClient.cpp -o $(ODIR)/KatClient.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(CLIENT)/katalyn.cpp -o $(ODIR)/katalyn.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/logger.cpp -o $(ODIR)/logger.o
	$(CC) $(CC_OPTIONS) $(ODIR)/*.o  -o $(BUILD)/$@
	cp $(BUILD)/* $(BIN)/

# this target compiles and links the server
server: clean
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(SERVER)/KatServ.cpp -o $(ODIR)/KatServ.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(SERVER)/trackr.cpp -o $(ODIR)/trackr.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/logger.cpp -o $(ODIR)/logger.o
	$(CC) $(CC_OPTIONS) $(ODIR)/*.o  -o $(BUILD)/$@
	cp $(BUILD)/* $(BIN)/

cleano:
	rm -f $(ODIR)/*.o

clean:
	rm -f $(BUILD)/* $(ODIR)/*.o

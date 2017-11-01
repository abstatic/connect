## Makefile for Connect
## Author- Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com)

CC = g++
# CC_OPTIONS = -ggdb3 -Wall -std=c++11 -lpthread
CC_OPTIONS = -ggdb3 -w -std=c++11 -pthread -lcrypto
CFLAGS = $(CC_OPTIONS)

ODIR = obj
BUILD = build
BIN = bin
SDIR = src
CLIENT = nodeClient

.PHONY: clean cleano client

all: client

run:
	$(CC) $(CC_OPTIONS) -o $(EXE) $(SRC)
	./$(EXE)

config:
	$(CC) $(CC_OPTIONS) $(SDIR)/configure.cpp -o $(BUILD)/$@
	cd $(BUILD); ./$@

# this target compiles and links the client
client: clean
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(CLIENT)/connect.cpp -o $(ODIR)/connect.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/$(CLIENT)/nodeClient.cpp -o $(ODIR)/nodeClient.o
	$(CC) $(CC_OPTIONS) -c $(SDIR)/logger.cpp -o $(ODIR)/logger.o
	$(CC) $(CC_OPTIONS) $(ODIR)/*.o  -o $(BUILD)/$@
	cp $(BUILD)/* $(BIN)/

cleano:
	rm -f $(ODIR)/*.o

clean:
	rm -f $(BUILD)/* $(ODIR)/*.o

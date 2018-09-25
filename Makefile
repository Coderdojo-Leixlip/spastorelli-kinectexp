CC=clang++
LD=clang++
CFLAGS=-g -Wall -std=c++11 -stdlib=libc++
LIBS=-lboost_system
INCLUDES=-I ./libs/websocketpp/
BUILDDIR=build
OBJ=server.o
BIN=server

all: dir $(BUILDDIR)/$(BIN)

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: %.cpp
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/$(BIN): $(BUILDDIR)/$(OBJ)
	$(LD) $(LIBS) $(BUILDDIR)/$(OBJ) -o $(BUILDDIR)/$(BIN)

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(BIN)

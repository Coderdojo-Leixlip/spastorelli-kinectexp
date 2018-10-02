CC=clang++
LD=clang++
CFLAGS=-g -Wall -std=c++11 -stdlib=libc++
LIBS=-lboost_system `pkg-config --libs libfreenect`
INCLUDES=-I ./libs/websocketpp/ `pkg-config --cflags libfreenect`
BUILDDIR=build
OBJS=$(addprefix $(BUILDDIR)/,server.o device.o)
BIN=$(addprefix $(BUILDDIR)/,server)

all: $(BIN)

$(BUILDDIR)/%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
	$(LD) $(CFLAGS) $(LIBS) -o $(BIN) $(OBJS)

$(OBJS): | $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

format:
	clang-format -style=file -i *.cc *.h

clean:
	rm -rf $(BUILDDIR)/*.o $(BIN)
CC=clang++
LD=clang++
CFLAGS=-O2 -g -Wall -std=c++11 -stdlib=libc++
LIBS=-lboost_system `pkg-config --libs libfreenect`
FAKENECT=OFF
FAKENECT_LIB=/usr/local/lib/fakenect/
FAKENECT_TEST_DATA=test_data
INCLUDES=-I ./libs/websocketpp/ `pkg-config --cflags libfreenect`
BUILDDIR=build
OBJS=$(addprefix $(BUILDDIR)/,server.o device.o)
BIN=$(addprefix $(BUILDDIR)/,server)

all: $(BIN)

$(BUILDDIR)/%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
ifeq ($(FAKENECT),ON)
	$(LD) $(CFLAGS) -L$(FAKENECT_LIB) $(LIBS) -o $(BIN) $(OBJS)
else
	$(LD) $(CFLAGS) $(LIBS) -o $(BIN) $(OBJS)
endif

$(OBJS): | $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(BIN)
ifeq ($(FAKENECT),ON)
	DYLD_LIBRARY_PATH=$(FAKENECT_LIB) FAKENECT_PATH=$(FAKENECT_TEST_DATA) $(BIN)
else
	$(BIN)
endif

fakenect: run FAKENECT=ON

format:
	clang-format -style=file -i *.cc *.h

clean:
	rm -rf $(BUILDDIR)/*.o $(BIN)
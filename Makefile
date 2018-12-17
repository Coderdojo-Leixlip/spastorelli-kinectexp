CC=clang++
LD=clang++
CFLAGS=-O2 -g -Wall -std=c++11 -stdlib=libc++
LIBS=-lboost_system `pkg-config --libs libfreenect`
FAKENECT=OFF
FAKENECT_LIB=/usr/local/lib/fakenect/
FAKENECT_TEST_DATA=test_data
INCLUDES=-I ./libs/websocketpp/ `pkg-config --cflags libfreenect`
BUILD_DIR=build
BUILD_BIN_DIR=$(BUILD_DIR)/bin
BUILD_LIBS_DIR=$(BUILD_DIR)/libs
BUILD_TESTS_DIR=$(BUILD_DIR)/tests
SRC_DIR=src
TESTS_DIR=tests
GTEST_SRC_DIR=libs/googletest/googletest
GTEST_LIB=$(BUILD_LIBS_DIR)/libgtest.a
OBJS=$(addprefix $(BUILD_LIBS_DIR)/,run_server.o server.o channel.o publisher.o device.o)
TESTS=$(addprefix $(BUILD_TESTS_DIR)/,sample_test)
BIN=$(addprefix $(BUILD_BIN_DIR)/,kinect_serve)

all: $(BIN)

$(BUILD_LIBS_DIR)/%.o: $(SRC_DIR)/%.cc
	mkdir -p $(BUILD_LIBS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
	mkdir -p $(BUILD_BIN_DIR)
ifeq ($(FAKENECT),ON)
	$(LD) $(CFLAGS) -L$(FAKENECT_LIB) $(LIBS) -o $(BIN) $(OBJS)
else
	$(LD) $(CFLAGS) $(LIBS) -o $(BIN) $(OBJS)
endif

$(OBJS): | flatbuf-cpp

$(BUILD_TESTS_DIR)/%_test: $(TESTS_DIR)/%_test.cc
	mkdir -p $(BUILD_TESTS_DIR)
	$(CC) $(CFLAGS) -isystem $(GTEST_SRC_DIR)/include -pthread $(GTEST_LIB) \
		-o $@ $<

$(GTEST_LIB):
	mkdir -p $(BUILD_LIBS_DIR)
	$(CC) $(CFLAGS) -isystem $(GTEST_SRC_DIR)/include -I$(GTEST_SRC_DIR) \
		-pthread -c $(GTEST_SRC_DIR)/src/gtest-all.cc -o $(BUILD_LIBS_DIR)/gtest-all.o
	ar -rv $(GTEST_LIB) $(BUILD_LIBS_DIR)/gtest-all.o

$(TESTS): $(GTEST_LIB)

run: $(BIN)
ifeq ($(FAKENECT),ON)
	DYLD_LIBRARY_PATH=$(FAKENECT_LIB) FAKENECT_PATH=$(FAKENECT_TEST_DATA) $(BIN)
else
	$(BIN)
endif

run-tests: $(TESTS)
	$(TESTS)

tests: $(TESTS)

flatbuf-cpp:
	flatc --cpp --scoped-enums -o protocol/ protocol/protocol.fbs

flatbuf-js:
	flatc --js -o protocol/ protocol/protocol.fbs

fakenect: run FAKENECT=ON

format:
	clang-format -style=file -i *.cc *.h

clean:
	rm -rf $(BUILD_DIR)
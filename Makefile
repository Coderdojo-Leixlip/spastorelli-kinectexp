CC=clang++
LD=clang++
OPTIMIZER_OPTS=-O2
LIBS=-lboost_system
INCLUDES=-I ./libs/websocketpp/ `pkg-config --cflags libfreenect`
GTEST_SRC_DIR=libs/googletest/googletest
GTEST_INCLUDES=-isystem $(GTEST_SRC_DIR)/include -I$(GTEST_SRC_DIR)

SRC_DIR=src
TESTS_DIR=tests
PROTO_DIR=protocol
BUILD_DIR=build
BUILD_BIN_DIR=$(BUILD_DIR)/bin
BUILD_COVERAGE_DIR=$(BUILD_DIR)/coverage
BUILD_DEPS_DIR=$(BUILD_DIR)/deps
BUILD_LIBS_DIR=$(BUILD_DIR)/libs
BUILD_TESTS_DIR=$(BUILD_DIR)/tests

$(shell mkdir -p $(BUILD_BIN_DIR) $(BUILD_COVERAGE_DIR) $(BUILD_DEPS_DIR) \
	$(BUILD_LIBS_DIR) $(BUILD_TESTS_DIR) \
> /dev/null)
SRCS=$(wildcard $(SRC_DIR)/*.cc)
SRCS+=$(wildcard $(TESTS_DIR)/*.cc)
HEADERS=$(wildcard $(SRC_DIR)/*.h)
HEADERS+=$(wildcard $(TESTS_DIR)/*.h)
INCLUDES+=-I $(SRC_DIR)

LD_RPATHS=-rpath,$(PWD)/$(BUILD_LIBS_DIR)

COVERAGE=OFF
ifeq ($(COVERAGE), ON)
	COVERAGE_FLAGS=--coverage
	OPTIMIZER_OPTS=-O0
endif

SANITIZER=OFF
ifeq ($(SANITIZER), ON)
	SANITIZER_FLAGS=-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
	OPTIMIZER_OPTS=-O1
endif

CFLAGS=-g -Wall -std=c++11 -stdlib=libc++
CFLAGS+=$(OPTIMIZER_OPTS) $(SANITIZER_FLAGS) $(COVERAGE_FLAGS)
DEPFLAGS= -MT $@ -MMD -MP -MF $(BUILD_DEPS_DIR)/$*.Td

COMPILE.cc=$(CC) $(DEPFLAGS) $(CFLAGS) -c
POSTCOMPILE=@(mv -f $(BUILD_DEPS_DIR)/$*.Td $(BUILD_DEPS_DIR)/$*.d && touch $@)
LINK.cc=$(LD) $(CFLAGS) -Wl,$(LD_RPATHS) $(LIBS)

BIN_OBJS=$(addprefix $(BUILD_LIBS_DIR)/, \
	run_server.o server.o channel.o \
	command.o publisher.o openkinect_device.o)
BIN=$(addprefix $(BUILD_BIN_DIR)/,kinect_serve)

FAKENECT=OFF
FREENECT_LIB=`pkg-config --libs libfreenect`
FAKENECT_LIB=/usr/local/lib/fakenect/
FAKENECT_TEST_DATA=test_data

ifeq ($(FAKENECT),ON)
	LIBS+=-L$(FAKENECT_LIB) $(FREENECT_LIB)
	BIN=$(addprefix $(BUILD_BIN_DIR)/,fakenect_serve)
	BIN_VARS=DYLD_LIBRARY_PATH=$(FAKENECT_LIB) FAKENECT_PATH=$(FAKENECT_TEST_DATA)
else
	LIBS+=$(FREENECT_LIB)
endif

include $(TESTS_DIR)/tests.mk

all: $(BIN) $(TESTS)

$(BUILD_LIBS_DIR)/%_test.o: $(TESTS_DIR)/%_test.cc
$(BUILD_LIBS_DIR)/%_test.o: $(TESTS_DIR)/%_test.cc $(BUILD_DEPS_DIR)/%_test.d
	$(COMPILE.cc) $(INCLUDES) $(GTEST_INCLUDES) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(BUILD_LIBS_DIR)/%.o: $(SRC_DIR)/%.cc
$(BUILD_LIBS_DIR)/%.o: $(SRC_DIR)/%.cc $(BUILD_DEPS_DIR)/%.d
	$(COMPILE.cc) $(INCLUDES) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(BIN): $(BIN_OBJS)
	$(LINK.cc) $(COVERAGE_FLAGS) -o $(BIN) $(BIN_OBJS)

$(BIN_OBJS): $(PROTO_DIR)/protocol_generated.h

.SECONDEXPANSION:
$(TESTS): $$($$@_OBJS) $(BUILD_LIBS_DIR)/gtest-main.a
	$(LINK.cc) -lpthread \
		-o $(addprefix $(BUILD_TESTS_DIR)/,$@) $^

$(BUILD_LIBS_DIR)/gtest-all.o:
	$(CC) $(CFLAGS) $(GTEST_INCLUDES) -c \
		-c $(GTEST_SRC_DIR)/src/gtest-all.cc -o $@

$(BUILD_LIBS_DIR)/gtest-main.o:
	$(CC) $(CFLAGS) $(GTEST_INCLUDES) -c \
		-c $(GTEST_SRC_DIR)/src/gtest_main.cc -o $@

$(BUILD_LIBS_DIR)/gtest.a: $(BUILD_LIBS_DIR)/gtest-all.o
	$(AR) -rv $@ $<

$(BUILD_LIBS_DIR)/gtest-main.a: $(BUILD_LIBS_DIR)/gtest-all.o \
		$(BUILD_LIBS_DIR)/gtest-main.o
	$(AR) -rv $@ $^

run: $(BIN)
	$(BIN_VARS) $(BIN)

run-tests: $(TESTS)
	$(foreach TEST,$(TESTS), \
		echo "\nRunnning $(TEST)..." ; \
		./$(BUILD_TESTS_DIR)/$(TEST) ; \
	)

coverage:
	./coverage.sh

fakenect: run FAKENECT=ON

format:
	clang-format -style=file -i $(SRCS) $(HEADERS)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DEPS_DIR)/%.d: ;
.PRECIOUS: $(BUILD_DEPS_DIR)/%.d %.o

include $(wildcard $(patsubst %,$(BUILD_DEPS_DIR)/%.d,$(basename $(SRCS))))
CC=clang++
LD=clang++
OPTIMIZER_OPTS=-O2
LIBS=-lboost_system
INCLUDES=-I ./libs/websocketpp/
GTEST_SRC_DIR=libs/googletest/googletest
GTEST_INCLUDES=-isystem $(GTEST_SRC_DIR)/include -I$(GTEST_SRC_DIR)

SRC_DIR=src
LIBS_DIR=libs
TESTS_DIR=tests
FBS_DIR=protocol
GEN_FBS_DIR=$(SRC_DIR)/$(FBS_DIR)
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
FBS_SCHEMAS=$(wildcard $(FBS_DIR)/*.fbs)
GEN_FBS_HEADERS=$(patsubst \
	$(FBS_DIR)/%.fbs,$(GEN_FBS_DIR)/%_generated.h,$(FBS_SCHEMAS))
INCLUDES+=-I $(SRC_DIR)

LD_RPATHS=-rpath,$(PWD)/$(BUILD_LIBS_DIR)

ifeq ($(COVERAGE), ON)
	COVERAGE_FLAGS=--coverage
	OPTIMIZER_OPTS=-O0
endif

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
	command.o publisher.o)
BIN=$(addprefix $(BUILD_BIN_DIR)/,kinect_serve)

LPTC_DEVICE?=OpenKinect
LPTC_DEVICE_DEPS=
ifeq ($(LPTC_DEVICE),OpenKinect)
	include openkinect_device.mk
else ifeq ($(LPTC_DEVICE),OpenNI2)
	include openni2_device.mk
endif

include $(TESTS_DIR)/tests.mk

default_target: $(BIN)

$(GEN_FBS_DIR)/%_generated.h: $(FBS_DIR)/%.fbs
	flatc --cpp --scoped-enums -o $(GEN_FBS_DIR) $<

$(BUILD_LIBS_DIR)/%_test.o: $(TESTS_DIR)/%_test.cc
$(BUILD_LIBS_DIR)/%_test.o: $(TESTS_DIR)/%_test.cc $(BUILD_DEPS_DIR)/%_test.d
	$(COMPILE.cc) $(INCLUDES) $(GTEST_INCLUDES) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(wildcard $(SRC_DIR)/*.cc): $(GEN_FBS_HEADERS)

$(BUILD_LIBS_DIR)/%.o: $(SRC_DIR)/%.cc
$(BUILD_LIBS_DIR)/%.o: $(SRC_DIR)/%.cc $(BUILD_DEPS_DIR)/%.d
	$(COMPILE.cc) $(INCLUDES) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(LPTC_DEVICE_DEPS):
	./build_deps.sh -o $(BUILD_LIBS_DIR) $(LPTC_DEVICE)

$(BIN): $(LPTC_DEVICE_DEPS) $(BIN_OBJS)
	$(LINK.cc) $(COVERAGE_FLAGS) -o $(BIN) $(BIN_OBJS)

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

.PHONY: all clean coverage default_target fakenect format run run-tests

all: $(BIN) $(TESTS)

run: $(BIN)
	(cd $(BUILD_BIN_DIR) && exec $(BIN_VARS) $(addprefix ./, $(notdir $(BIN))))

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
	rm -rf $(GEN_FBS_DIR)
	rm -rf $(BUILD_DIR)

$(BUILD_DEPS_DIR)/%.d: ;
.PRECIOUS: $(BUILD_DEPS_DIR)/%.d %.o

include $(wildcard $(patsubst %,$(BUILD_DEPS_DIR)/%.d,$(basename $(SRCS))))
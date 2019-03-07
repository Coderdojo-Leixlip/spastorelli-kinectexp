FREENECT_LIB_DIR=$(BUILD_LIBS_DIR)
ifeq ($(FAKENECT),ON)
	FREENECT_LIB_DIR=$(BUILD_LIBS_DIR)/fakenect/
	FAKENECT_TEST_DATA=$(PWD)/test_data/

	BIN=$(addprefix $(BUILD_BIN_DIR)/,fakenect_serve)
	BIN_VARS=DYLD_LIBRARY_PATH=$(FAKENECT_LIB_DIR) FAKENECT_PATH=$(FAKENECT_TEST_DATA)
endif
LIBS+=-L$(FREENECT_LIB_DIR) -lfreenect

LPTC_DEVICE_DEPS=$(addprefix $(BUILD_LIBS_DIR), \
	/libfreenect.dylib \
	/fakenect/libfreenect.dylib)

INCLUDES+=`pkg-config --cflags libusb-1.0`
INCLUDES+=-I $(LIBS_DIR)/libfreenect/include/ \
	-I $(LIBS_DIR)/libfreenect/wrappers/cpp/
BIN_OBJS+=$(BUILD_LIBS_DIR)/openkinect_device.o
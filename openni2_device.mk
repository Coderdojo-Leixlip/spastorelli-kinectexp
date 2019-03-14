OPENNI2_INCLUDES=-I $(LIBS_DIR)/OpenNI2/Include/
OPENNI2_LIB=-L$(BUILD_LIBS_DIR) -lOpenNI2

LPTC_DEVICE_DEPS=$(addprefix $(BUILD_LIBS_DIR), \
	/OpenNI2/Drivers/libFreenectDriver.dylib \
	/OpenNI2/Drivers/libOniFile.dylib \
	/libOpenNI2.dylib)

INCLUDES+=$(OPENNI2_INCLUDES)
LIBS+=$(OPENNI2_LIB)
BIN_OBJS+=$(BUILD_LIBS_DIR)/openni2_device.o
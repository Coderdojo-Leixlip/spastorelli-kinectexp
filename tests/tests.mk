TESTS=server_test sample_test
server_test_OBJS=$(addprefix $(BUILD_LIBS_DIR)/,server_test.o server.o channel.o publisher.o device.o)
sample_test_OBJS=$(addprefix $(BUILD_LIBS_DIR)/,sample_test.o)
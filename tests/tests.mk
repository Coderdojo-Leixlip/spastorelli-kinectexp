TESTS=command_test sample_test
command_test_OBJS=$(addprefix $(BUILD_LIBS_DIR)/,command_test.o command.o)
sample_test_OBJS=$(addprefix $(BUILD_LIBS_DIR)/,sample_test.o)
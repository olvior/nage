CC = gcc
CFLAGS = -Wall -g -DDEBUG -MMD
# CFLAGS = -Wall -O3 -MMD
LFLAGS = -lvulkan -lglfw
BUILD_DIR = bin
SRC_DIR = src
SHADER_DIR = src/shaders
TARGET = vk_engine

# Finds all the c files in 1 lvl or 2 lvl in side $(SRC_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(SRC_DIR)/*/*.c)
SRCS += $(wildcard $(SRC_DIR)/*/*/*.c)
SHADERS = $(wildcard $(SHADER_DIR)/shader.*)

INCLUDES = $(SRCS:%.c=%.h)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = ${OBJS:%.o=%.d}
SPV_SHADERS = $(SHADERS:$(SHADER_DIR)/shader.%=$(BUILD_DIR)/%.spv)


# MACOS wants a -rpath
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LFLAGS += -rpath /usr/local/lib
endif

all: $(BUILD_DIR)/$(TARGET) $(SPV_SHADERS)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	 $(CC) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) $(OBJS) $(LFLAGS)

ifeq ($(UNAME_S),Darwin)
	dsymutil $(BUILD_DIR)/$(TARGET)
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) ${CFLAGS} -c $< -o $@

$(BUILD_DIR)/%.spv: $(SHADER_DIR)/shader.%
	glslc $< -o $@

-include ${DEPS}

.PHONY: clean debug run all

run: $(BUILD_DIR)/$(TARGET) $(SPV_SHADERS)
	(cd $(BUILD_DIR); ./$(TARGET))

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(SPV_SHADERS)
	rm -rf $(BUILD_DIR)/$(TARGET).dSYM

debug:
	@echo SRCS is: $(SRCS)
	@echo SHADERS is: $(SHADERS)
	@echo INCLUDES is: $(INCLUDES)
	@echo OBJS is: $(OBJS)
	@echo DEPS is: $(DEPS)
	@echo SPV_SHADERS is: $(SPV_SHADERS)



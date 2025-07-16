CC = gcc
CCP = g++
CFLAGS = -Wall -g -DDEBUG -MMD
# CFLAGS = -Wall -O3 -MMD
CFLAGS += -Ithird_party/include
LFLAGS = -lvulkan -lglfw
BUILD_DIR = bin
SRC_DIR = src
SHADER_DIR = src/shaders
RUNTIME_DIR = out
TARGET = nage

# Finds all the c files in 1, 2, and 3 lvl directories $(SRC_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(SRC_DIR)/*/*.c)
SRCS += $(wildcard $(SRC_DIR)/*/*/*.c)
VMA_USAGE = $(wildcard $(SRC_DIR)/*/*/*.cpp)
SHADERS = $(wildcard $(SHADER_DIR)/shader.*)

INCLUDES = $(SRCS:%.c=%.h)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJS += $(VMA_USAGE:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = ${OBJS:%.o=%.d}
SPV_SHADERS = $(SHADERS:$(SHADER_DIR)/shader.%=$(RUNTIME_DIR)/%.spv)

IMGUI_DIR = third_party/imgui
IMGUI_SRCS = $(wildcard $(IMGUI_DIR)/*.cpp)
OBJS += $(IMGUI_SRCS:$(IMGUI_DIR)/%.cpp=$(BUILD_DIR)/$(IMGUI_DIR)/%.o)

# MACOS wants a -rpath
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LFLAGS += -rpath /usr/local/lib
endif

all: $(RUNTIME_DIR)/$(TARGET) $(SPV_SHADERS)

$(RUNTIME_DIR)/$(TARGET): $(OBJS)
	mkdir -p $(RUNTIME_DIR)
	$(CCP) $(CFLAGS) -o $(RUNTIME_DIR)/$(TARGET) $(OBJS) $(LFLAGS)

ifeq ($(UNAME_S),Darwin)
	dsymutil $(RUNTIME_DIR)/$(TARGET)
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) ${CFLAGS} -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CCP) -std=c++20 ${CFLAGS} -w -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CCP) -std=c++20 ${CFLAGS} -w -c $< -o $@

$(RUNTIME_DIR)/%.spv: $(SHADER_DIR)/shader.%
	glslc $< -o $@

-include ${DEPS}

.PHONY: clean debug run all

run: $(RUNTIME_DIR)/$(TARGET) $(SPV_SHADERS)
	(cd $(RUNTIME_DIR); ./$(TARGET))

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(SPV_SHADERS)
	rm -rf $(RUNTIME_DIR)/$(TARGET).dSYM

debug:
	@echo SRCS is: $(SRCS)
	@echo SHADERS is: $(SHADERS)
	@echo INCLUDES is: $(INCLUDES)
	@echo OBJS is: $(OBJS)
	@echo DEPS is: $(DEPS)
	@echo SPV_SHADERS is: $(SPV_SHADERS)
	@echo IMGUI_SRCS is: $(IMGUI_SRCS)



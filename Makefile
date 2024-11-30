DEBUG ?= 0
TEST ?= 0
EXECUTABLE_NAME = main

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = tests
UNITY_DIR = tests/unity

CC = gcc
C_STD = c11
CFLAGS := -std=$(C_STD) -Wall -Wextra -pedantic 
LDFLAGS = -lcurl
CPPFLAGS := -I$(INCLUDE_DIR) 

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
EXECUTABLE_NAME = dbg_main
else
CFLAGS += -O3
endif


C_SOURCES = $(wildcard $(SRC_DIR)/*.c)

ifeq ($(TEST), 1)
CFLAGS += -DTEST
C_SOURCES += $(wildcard $(TEST_DIR)/*.c)
C_SOURCES += $(wildcard $(UNITY_DIR)/*.c)
EXECUTABLE_NAME = test_main
endif

C_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

COMPILER_CALL = $(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)

#################
### TARGETS #####
#################

create:
	@mkdir -p build

build: create $(C_OBJECTS)
	$(COMPILER_CALL) $(C_OBJECTS) $(LDFLAGS) -o $(BUILD_DIR)/$(EXECUTABLE_NAME)

execute:
	./$(BUILD_DIR)/$(EXECUTABLE_NAME)

clean:
	rm -rf $(BUILD_DIR)/*.o
	rm -rf $(BUILD_DIR)/$(EXECUTABLE_NAME)

#################
### PATTERNS ####
#################

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(COMPILER_CALL) -c $< -o $@

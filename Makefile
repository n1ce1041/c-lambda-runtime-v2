DEBUG ?= 0
SANITIZE ?= 0

CC ?= cc
CPPFLAGS := -Iinclude
CFLAGS := -std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow \
	-Wconversion -Wstrict-prototypes
LDLIBS := -lcurl

BUILD_DIR := build
RUNTIME := $(BUILD_DIR)/bootstrap
TEST_BINARY := $(BUILD_DIR)/test_main
SOURCES := src/runtime.c src/handler.c
HEADERS := include/runtime.h include/handler.h

ifeq ($(DEBUG),1)
CFLAGS += -O0 -g3
else
CFLAGS += -O2
endif

ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

.PHONY: all build create test integration-test sanitize execute clean container

all: build

build: $(RUNTIME)

create:
	mkdir -p $(BUILD_DIR)

$(RUNTIME): $(SOURCES) $(HEADERS) | create
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(SOURCES) $(LDLIBS) -o $@

$(TEST_BINARY): $(SOURCES) $(HEADERS) tests/test_main.c | create
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTEST $(LDFLAGS) $(SOURCES) \
		tests/test_main.c $(LDLIBS) -o $@

test: $(TEST_BINARY)
	./$(TEST_BINARY)

integration-test: $(RUNTIME)
	python3 tests/integration_test.py ./$(RUNTIME)

sanitize:
	$(MAKE) clean
	$(MAKE) SANITIZE=1 test
	$(MAKE) SANITIZE=1 integration-test

execute: $(RUNTIME)
	./$(RUNTIME)

container:
	docker build -t c-lambda-runtime-v2 .

clean:
	rm -f $(RUNTIME) $(TEST_BINARY)
	rmdir $(BUILD_DIR) 2>/dev/null || true

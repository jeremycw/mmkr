SOURCES=server.c tictoc.c pool.c merge_sort.c thpool.c core.c
TEST_SRCS=merge_sort_test.c segment_test.c pool_test.c parallel_map_test.c match_test.c
BINARY_NAME=mmkr
LFLAGS+=-lpthread

CC=clang
CFLAGS+=-Wall -Wextra -Werror -std=c99

SRC_DIR=src
TEST_SRC_DIR=test
DEBUG_DIR=bin/debug
RELEASE_DIR=bin/release
TEST_DIR=bin/test
ASM_DIR=bin/asm

DEBUG_OBJS = $(SOURCES:%.c=$(DEBUG_DIR)/%.o)
RELEASE_OBJS = $(SOURCES:%.c=$(RELEASE_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:%.c=$(TEST_DIR)/%.o)
ASMS = $(SOURCES:%.c=$(ASM_DIR)/%.s)

.PHONY: debug test release clean asm

debug: $(DEBUG_DIR)/$(BINARY_NAME)
release: $(RELEASE_DIR)/$(BINARY_NAME)
test: $(TEST_DIR)/$(BINARY_NAME)
	$(TEST_DIR)/$(BINARY_NAME)

$(DEBUG_DIR)/$(BINARY_NAME): $(DEBUG_OBJS) $(DEBUG_DIR)/main.o
	$(CC) $(DEBUG_OBJS) $(DEBUG_DIR)/main.o $(LFLAGS) -o $@

$(RELEASE_DIR)/$(BINARY_NAME): $(RELEASE_OBJS) $(RELEASE_DIR)/main.o
	$(CC) $(RELEASE_OBJS) $(RELEASE_DIR)/main.o $(LFLAGS) -o $@

$(TEST_DIR)/$(BINARY_NAME): $(TEST_OBJS) $(DEBUG_OBJS) $(TEST_DIR)/test.o
	$(CC) $(DEBUG_OBJS) $(TEST_OBJS) $(TEST_DIR)/test.o $(LFLAGS) -o $@

asm: $(ASMS) $(ASM_DIR)

# pull in dependency info for *existing* .o files
-include $(TEST_OBJS:.o=.d)
-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)

# compile and generate dependency info
$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	$(CC) -c -g -MMD $(CFLAGS) $< -o $@

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	$(CC) -c -O3 -MMD $(CFLAGS) $< -o $@

$(TEST_DIR)/%.o: $(TEST_SRC_DIR)/%.c | $(TEST_DIR)
	$(CC) -c -g -MMD $(CFLAGS) $< -o $@

$(ASM_DIR)/%.s: $(SRC_DIR)/%.c | $(ASM_DIR)
	$(CC) -S -O3 -mllvm --x86-asm-syntax=intel $(CFLAGS) $< -o $@

$(DEBUG_DIR):
	mkdir -p $@

$(RELEASE_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

$(ASM_DIR):
	mkdir -p $@

clean:
	@rm -rf bin

CC=gcc
CFLAGS=-Wall -Wno-implicit-fallthrough -Wextra -Werror -std=c11 -O3

PREFIX ?= /usr/local

SRC=src/siphash.c src/map.c src/hashmap.c
OBJS=siphash.o map.o hashmap.o
TARGET=libhashmap.a

TEST_SRC=test/test_siphash.c test/test_random.c test/test_map.c test/test_hashmap.c test/test_main.c
TEST_OBJS=test_siphash.o test_random.o test_map.o test_hashmap.o test_main.o
TEST_TARGET=hashmap_test

.PHONY:all clean test install uninstall help

all: $(TARGET) $(TEST_TARGET)

$(OBJS): $(SRC)
	$(CC) $(CFLAGS) -c -Isrc/ -Iinclude/ $(SRC)

$(TEST_OBJS): $(TEST_SRC)
	$(CC) $(CFLAGS) -c -Isrc/ -Iinclude/ $(TEST_SRC)

$(TEST_TARGET): $(OBJS) $(TEST_OBJS)
	$(CC) -o $(TEST_TARGET) $(OBJS) $(TEST_OBJS)

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $(OBJS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

install: $(TARGET)
	install -d $(PREFIX)/lib/
	install $(TARGET) $(PREFIX)/lib/
	install -d $(PREFIX)/include/hashmap/
	install include/hashmap.h $(PREFIX)/include/hashmap/
	rm -f $(OBJS) $(TARGET)

uninstall:
	rm -f $(PREFIX)/lib/$(TARGET)
	rm -rf $(PREFIX)/include/hashmap/

clean:
	rm -r $(OBJS) $(TEST_OBJS) $(TEST_TARGET)

help:
	@echo "Available targets:"
	@echo " all          - Build the library and test binary"
	@echo " test         - Run the test binary"
	@echo " install      - Install the library and header files to system directories specified by PREFIX"
	@echo " uninstall    - Remove files installed by the 'install' target"
	@echo " clean        - Remove object and binary files"
	@echo " help         - Display this help message"

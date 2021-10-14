CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c11 -g -O3

SRC=src/siphash.c src/map.c src/hashmap.c
OBJS=siphash.o map.o hashmap.o

TEST_SRC=test/test_siphash.c test/test_random.c test/test_map.c test/test_hashmap.c test/test_main.c
TEST_OBJS=test_siphash.o test_random.o test_map.o test_hashmap.o test_main.o
TEST_TARGET=test_main

.PHONY:all clean test

all: $(TEST_TARGET)

$(OBJS): $(SRC)
	$(CC) $(CFLAGS) -c -Isrc/ -Iinclude/ $(SRC)

$(TEST_OBJS): $(TEST_SRC)
	$(CC) $(CFLAGS) -c -Isrc/ -Iinclude/ $(TEST_SRC)

$(TEST_TARGET): $(OBJS) $(TEST_OBJS)
	$(CC) -o $(TEST_TARGET) $(OBJS) $(TEST_OBJS)

test:
	./$(TEST_TARGET)

clean:
	rm -r $(OBJS)
	rm -r $(TEST_OBJS)
	rm -r $(TEST_TARGET)

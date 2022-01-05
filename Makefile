CC=gcc
FLAGS=-std=c99 -g -pedantic-errors -Wunused
TARGET=mem

tests:
	$(CC) $(FLAGS) test.c my_mem.c -o $(TARGET)

main:
	$(CC) $(FLAGS) main.c my_mem.c -o $(TARGET)

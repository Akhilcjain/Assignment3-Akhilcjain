# Use cross compiler if provided, otherwise use native gcc
CC := $(CROSS_COMPILE)gcc

# Compiler flags
CFLAGS := -Wall -Werror

# Default target
all: aesdsocket

# Build writer
writer: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket

# Compile writer.c into writer.o
aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o

# Clean build artifacts
clean:
	rm -f aesdsocket aesdsocket.o

.PHONY: all clean


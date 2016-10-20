CC = gcc
CFLAGS = -O3 -pthread
LDFLAGS = -pthread

LIB_NAME = libtrace_buffer.so
EXAMPLE_NAME = trace_buffer_ex

.phony: all clean

all: $(LIB_NAME)
example: $(LIB_NAME) $(EXAMPLE_NAME)

$(EXAMPLE_NAME): trace_buffer_example.c
	$(CC) $(CFLAGS) -o $(EXAMPLE_NAME) trace_buffer_example.c ./libtrace_buffer.so
	./$(EXAMPLE_NAME)

$(LIB_NAME): trace_buffer.o
	$(CC) $(LDFLAGS) -shared -o $(LIB_NAME) trace_buffer.o

trace_buffer.o: trace_buffer.c trace_buffer.h
	$(CC) $(CFLAGS) -fPIC -c -o trace_buffer.o trace_buffer.c

clean:
	rm -f $(LIB_NAME) $(EXAMPLE_NAME) trace_buffer.o
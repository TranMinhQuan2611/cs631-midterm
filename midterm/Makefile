CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = ls
OBJS = ls.o print.o

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(CFLAGS) -o $(TARGET) $(OBJS)

ls.o: ls.c ls.h print.h
	gcc $(CFLAGS) -c ls.c

print.o: print.c print.h ls.h
	gcc $(CFLAGS) -c print.c

clean:
	rm -f $(TARGET) $(OBJS)

TARGET = main

SRC = service.c siftp.c main.c

OBJS = $(SRC:.c=.o)

CC = gcc

INCLUDE = -I/usr/include/ -I/usr/include/ncurses

LIBS := -lfuse

CFLAGS = -g -Wall -D_FILE_OFFSET_BITS=64 -DNODEBUG

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o $(TARGET)
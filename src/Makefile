TARGET = main

SRC = ftpLib/service.c ftpLib/siftp.c main.c

OBJS = ftpLib/siftp.o ftpLib/service.o main.o

CC = gcc

INCLUDE = -I/usr/include/ -I/usr/include/ncurses

LIBS := -lfuse

CFLAGS = -g -w -D_FILE_OFFSET_BITS=64 -DNODEBUG

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

service.o: ftpLib/service.c
	$(CC) $(CFLAGS) -c ftpLib/service.c

siftp.o: ftpLib/siftp.c
	$(CC) $(CFLAGS) -c ftpLib/siftp.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm *.o $(TARGET)
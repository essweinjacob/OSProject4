CC = gcc
CFLAGS = -g
TARGET1 = oss
OBJS1 = oss.o
.SUFFIXES: .c .o

all: oss

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)
.c .o:
	$(CC) $(CFLAGS) -c $<
clean:
	/bin/rm -f *.o $(TARGET1)

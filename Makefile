CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = my_tar

all: $(TARGET)

$(TARGET): my_tar.o
	$(CC) $(CFLAGS) -o $(TARGET) my_tar.o

my_tar.o: my_tar.c
	$(CC) $(CFLAGS) -c my_tar.c

fclean:
	rm -f *.o $(TARGET)

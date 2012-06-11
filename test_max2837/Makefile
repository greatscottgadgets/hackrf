# Makefile

CC=mingw32-gcc

CFLAGS=-c -Wall -DTEST -DBUS_PIRATE -DDEBUG -Dgpio_set
LDFLAGS=-fno-exceptions -s

all: max2837.exe

%.exe:%.o
	$(CC) $(LDFLAGS) $< -o $@

%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	-$(RM) *.o
	-$(RM) *.exe
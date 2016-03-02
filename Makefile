VERSION = 0.0.0

TARGET = content.h kringkastty

OS := $(shell uname)
ifeq ($(OS),Darwin)
	CC = clang
	CFLAGS = -I/usr/include/machine -O2 -Wall
	LIBS =
else
	CC = gcc
	CFLAGS = -DSVR4 -D_GNU_SOURCE -O2 -g -fomit-frame-pointer -Wall
	LIBS =
endif

all: $(TARGET)

content.h: content.html
	xxd -i content.html content.h

kringkastty: content.h kringkastty.o io.o mongoose.o
	$(CC) $(CFLAGS) -o kringkastty mongoose.o kringkastty.o io.o $(LIBS)

clean:
	rm -f *.o $(TARGET) ttyrecord *~ content.h


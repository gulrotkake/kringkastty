CC = clang
CFLAGS = -O2 -Wall
VERSION = 0.0.0

TARGET = content.h kringkastty

all: $(TARGET)

content.h: content.html
	xxd -i content.html content.h

kringkastty: content.h kringkastty.o io.o mongoose.o
	$(CC) $(CFLAGS) -o kringkastty mongoose.o kringkastty.o io.o

clean:
	rm -f *.o $(TARGET) ttyrecord *~ content.h


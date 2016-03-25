CC = gcc
BUILD = build
TARGET = kringkastty
OS := $(shell uname)

ifeq ($(OS),Darwin)
	CFLAGS = -g -Ibuild -Iinclude -I/usr/include/machine -O2 -Wall
else
	CFLAGS = -DSVR4 -D_GNU_SOURCE -O2 -g -fomit-frame-pointer -Wall
endif

OBJS = $(BUILD)/kringkastty.o $(BUILD)/mongoose.o $(BUILD)/io.o \
	$(BUILD)/lw_terminal_vt100.o $(BUILD)/lw_terminal_parser.o \
    $(BUILD)/buffer.o

all: $(TARGET)

$(BUILD)/%.o: src/%.c $(BUILD)/content.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD)/content.h: extra/js/*.js extra/css/content.css extra/html/content.tmpl
	mkdir -p $(BUILD)
	cat extra/html/content.tmpl > $(BUILD)/content.html
	sed -e '/\[\[main\]\]/ {' -e 'r extra/js/main.js' -e 'd' -e '}' -i '' $(BUILD)/content.html
	sed -e '/\[\[style\]\]/ {' -e 'r extra/css/content.css' -e 'd' -e '}' -i '' $(BUILD)/content.html
	sed -e '/\[\[term\]\]/ {' -e 'r extra/js/term.js' -e 'd' -e '}' -i '' $(BUILD)/content.html
	xxd -i $(BUILD)/content.html $(BUILD)/content.h

kringkastty: $(OBJS)
	$(CC) -o $(BUILD)/$@ $^ $(LIBS)

clean:
	rm $(BUILD)/*.o $(BUILD)/*.html $(BUILD)/*.h $(BUILD)/kringkastty
	rmdir $(BUILD)

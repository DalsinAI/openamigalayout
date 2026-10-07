CC ?= cc
AR ?= ar
CFLAGS ?= -O2
WARN = -std=c99 -Wall -Wextra -Werror -pedantic
BUILD = build

.PHONY: all test clean

all: $(BUILD)/libopenlayout.a $(BUILD)/test_layout

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/openlayout.o: src/openlayout.c include/openlayout.h | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) -Iinclude -c src/openlayout.c -o $@

$(BUILD)/libopenlayout.a: $(BUILD)/openlayout.o
	$(AR) rcs $@ $<

$(BUILD)/test_layout: tests/test_layout.c $(BUILD)/libopenlayout.a
	$(CC) $(CFLAGS) $(WARN) -Iinclude tests/test_layout.c $(BUILD)/libopenlayout.a -o $@

test: $(BUILD)/test_layout
	./$(BUILD)/test_layout

clean:
	rm -rf $(BUILD)

CC := /usr/local/opt/llvm/bin/clang
CXX := /usr/local/opt/llvm/bin/clang++

.DEFAULT_GOAL := all

_build/src/girafsrc-plugin/girafsrc.o: src/girafsrc-plugin/girafsrc.c src/girafsrc-plugin/girafsrc.h src/girafsrc-plugin/gstgirafsrc.h
	mkdir -p $(dir $@)
	$(CC) -O2 -Wall -c $(shell pkg-config --cflags gstreamer-1.0 gstreamer-gl-1.0) -MJ./$@.json -o $@ $<

_build/src/girafsrc-plugin/gstgirafsrc.so: src/girafsrc-plugin/gstgirafsrc.c src/girafsrc-plugin/gstgirafsrc.h _build/src/girafsrc-plugin/girafsrc.o
	mkdir -p $(dir $@)
	$(CC) -O2 -shared -Wall $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-gl-1.0) -MJ./$@.json _build/src/girafsrc-plugin/girafsrc.o -o $@ $<

_build/%.so: %.c
	mkdir -p $(dir $@)
	$(CC) -O2 -shared -Wall $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-gl-1.0) -MJ./$@.json -o $@ $<

_build/src/gstreamermm/%: src/gstreamermm/%.cpp
	mkdir -p $(dir $@)
	$(CC) -O2 -Wall -lc++ $(shell pkg-config --cflags --libs gstreamermm-1.0) -MJ./$@.json -o $@ $<

_build/src/basic-tutorial-5: src/basic-tutorial-5.c
	mkdir -p $(dir $@)
	$(CC) -g -Wall $(shell pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0) -MJ./$@.json -o $@ $<

_build/src/basic-tutorial-8: src/basic-tutorial-8.c
	mkdir -p $(dir $@)
	$(CC) -g -Wall $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-audio-1.0) -MJ./$@.json -o $@ $<

_build/%: %.c
	mkdir -p $(dir $@)
	$(CC) -O2 -Wall $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./$@.json -o $@ $<

_build/%: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -O2 -Wall -std=c++17 $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./$@.json -o $@ $<

compile_commands.json: $(wildcard _build/*.json)
	@echo writing $@...
	@echo '[' > $@
	@find _build -name '*.json' | xargs cat >> $@
	@echo ']' >> $@

clean:
	$(RM) -r _build
	$(RM) compile_commands.json
	$(RM) $(patsubst %.c,%,$(wildcard *.c))

all: $(addprefix _build/,$(basename $(shell rg src --files -g '*.c' -g '*.cpp' -g '!*plugin*')))
all: $(patsubst %.c,_build/%.so,$(shell rg src --files -g '**/*-plugin/*.c' -g '**/*-plugin/*.cpp'))
all: compile_commands.json

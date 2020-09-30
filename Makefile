CC := /usr/local/opt/llvm/bin/clang
CXX := /usr/local/opt/llvm/bin/clang++

.DEFAULT_GOAL := all

_build/src/metaballs-src-plugin/metaballssrc.so: src/metaballs-src-plugin/metaballssrc.c $(wildcard src/metaballs-src-plugin/*)
	mkdir -p $(dir $@)
	for f in $(shell rg --files -g '*.frag' -g '*.vert' src/metaballs-src-plugin); do \
		mono tools/shader_minifier.exe --no-renaming --no-sequence $$f -o _build/$$f.h; \
	done
	$(CC) -O2 -shared -Wall -I$(dir $@) $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-gl-1.0) -MJ./$@.json -o $@ $<

_build/src/gstreamermm/%: src/gstreamermm/%.cpp
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

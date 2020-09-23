CC := /usr/local/opt/llvm/bin/clang
CXX := /usr/local/opt/llvm/bin/clang++

.DEFAULT_GOAL := all

_build/src/gstreamermm/%: src/gstreamermm/%.cpp
	mkdir -p $(dir $@)
	$(CC) -O2 -Wall -lc++ $(shell pkg-config --cflags --libs gstreamermm-1.0) -MJ./$@.json -o $@ $<

_build/%: %.c
	mkdir -p $(dir $@)
	$(CC) -O2 $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./$@.json -o $@ $<

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

all: $(addprefix _build/,$(basename $(shell rg src --files -g '*.c' -g '*.cpp'))) compile_commands.json

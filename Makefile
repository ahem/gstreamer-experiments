CC := /usr/local/opt/llvm/bin/clang
CXX := /usr/local/opt/llvm/bin/clang

.DEFAULT_GOAL := all


_build/%: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./$@.json -o $@ $<

_build/%: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) -Wall -std=c++17 $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./$@.json -o $@ $<

compile_commands.json: $(wildcard _build/*.json)
	@echo writing $@...
	@echo '[' > $@
	@find _build -name '*.json' | xargs cat >> $@
	@echo ']' >> $@

clean:
	$(RM) -r _build
	$(RM) compile_commands.json
	$(RM) $(patsubst %.c,%,$(wildcard *.c))

all: $(subst src,_build,$(basename $(shell rg src --files -g '*.c' -g '*.cpp'))) compile_commands.json

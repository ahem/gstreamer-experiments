CC := /usr/local/opt/llvm/bin/clang

.DEFAULT_GOAL := all

%: %.c
	mkdir -p _build
	$(CC) $(shell pkg-config --cflags --libs gstreamer-1.0) -MJ./_build/$@.json -o $@ $<

compile_commands.json: $(wildcard _build/*.json)
	@echo writing $@...
	@echo '[' > $@
	@find _build -name '*.json' | xargs cat >> $@
	@echo ']' >> $@

clean:
	$(RM) -r _build
	$(RM) compile_commands.json
	$(RM) $(patsubst %.c,%,$(wildcard *.c))

all: $(patsubst %.c,%,$(wildcard *.c)) compile_commands.json

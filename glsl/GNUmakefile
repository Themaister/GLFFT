SOURCES := $(wildcard *.comp)
OUTPUT := $(SOURCES:.comp=.inc)

all: $(OUTPUT)

%.inc: %.comp
	./shader_to_inc.sh $< $@

clean:
	rm -f $(OUTPUT)

.PHONY: clean

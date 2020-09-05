CC= gcc
GCCFLAGS= -std=c99 -Wall -pedantic -g -fsanitize=address
GCCLIBS= -lrt -pthread

SOURCES= $(wildcard *.c)
BINS=$(SOURCES:.c=.out)

all: $(BINS)
	for file in *.out; do mv $${file} $${file%.*} ;done 

%.out: %.c
	$(CC) $(GCCFLAGS) $^ -o $@ $(GCCLIBS)

clean:
	rm -rf slave solve vista output.txt

.PHONY: all clean

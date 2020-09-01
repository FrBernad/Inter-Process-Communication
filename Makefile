CC= gcc
GCCFLAGS= -std=c99 -Wall -pedantic -g

SOURCES= $(wildcard *.c)
BINS=$(SOURCES:.c=.out)

all: $(BINS)
	for file in *.out; do mv $${file} $${file%.*} ;done 

%.out: %.c
	$(CC) $(GCCFLAGS) $^ -o $@ 

clean:
	rm -rf slave solve vista

.PHONY: all clean

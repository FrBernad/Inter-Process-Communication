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

cleanTest:
	rm -rf report.tasks PVS-Studio.log strace_out cppoutput.txt *.valgrind

test: clean 
	pvs-studio-analyzer trace -- make  > /dev/null
	pvs-studio-analyzer analyze > /dev/null 2> /dev/null
	plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log  > /dev/null
	valgrind --leak-check=full -v ./solve files/*  2> solve.valgrind; valgrind --leak-check=full -v ./vista  2> vista.valgrind; cppcheck --quiet --enable=all --force --inconclusive . 2> cppoutput.txt

.PHONY: all clean test cleanTest

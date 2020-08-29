SOURCES= $(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)
SOURCES_APPLICATIONS=$(wildcard applications/*.c applications/shell/*.c applications/calculator/*.c)

OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
OBJECTS_APPLICATIONS=$(SOURCES_APPLICATIONS:.c=.o)

all: $(APP)

$(APP): $(OBJECTS) $(OBJECTS_ASM) $(OBJECTS_APPLICATIONS)
	$(GCC) $(GCCFLAGS) $< -o $@

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf 

.PHONY: all clean

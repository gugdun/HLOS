CFLAGS	:= -static -O2 -nostdlib -Ttext=0x7F0000000000
SOURCES	:= $(wildcard *.c)

all:
	@echo "Building test.elf..."
	@$(CC) $(CFLAGS) -o ../../out/test.elf $(SOURCES)
	@echo "Done!"

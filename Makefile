TC=riscv64-unknown-elf

all: bootloader

bootloader: linker.ld bin/main.o bin/entry.o
	$(TC)-ld -T linker.ld -nostdlib bin/main.o bin/entry.o -o bin/bootloader.elf

bin/main.o: src/main.c
	$(TC)-gcc -Wall -Wextra -c -mcmodel=medany src/main.c -o bin/main.o -ffreestanding

bin/entry.o: src/entry.s
	$(TC)-as -c src/entry.s -o bin/entry.o

clean:
	rm bin/*.*
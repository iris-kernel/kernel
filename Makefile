TC=riscv64-unknown-elf

# Create bin directory if it doesn't exist
$(shell mkdir -p bin)

all: bin/kernel.elf
# Explicit rule for the ELF file
bin/kernel.elf: linker.ld bin/main.o bin/entry.o bin/physical.o bin/dtb.o bin/opensbi.o
	$(TC)-ld -T linker.ld -nostdlib bin/main.o bin/entry.o bin/physical.o bin/opensbi.o bin/dtb.o -o bin/kernel.elf

bin/main.o: src/main.c
	$(TC)-gcc -Wall -Wextra -c -mcmodel=medany src/main.c -o bin/main.o -ffreestanding -nostdlib -I src

bin/entry.o: src/entry.s
	$(TC)-as -c src/entry.s -o bin/entry.o

bin/dtb.o: src/device/dtb.c
	$(TC)-gcc -Wall -Wextra -c -mcmodel=medany src/device/dtb.c -o bin/dtb.o -ffreestanding -nostdlib -I src

bin/opensbi.o: src/device/opensbi.c
	$(TC)-gcc -Wall -Wextra -c -mcmodel=medany src/device/opensbi.c -o bin/opensbi.o -ffreestanding -nostdlib -I src

bin/physical.o: src/memory/physical.c
	$(TC)-gcc -Wall -Wextra -c -mcmodel=medany src/memory/physical.c -o bin/physical.o -ffreestanding -nostdlib -I src

# Convert ELF to binary for easier loading
bin/kernel.bin: bin/kernel.elf
	$(TC)-objcopy -O binary bin/kernel.elf bin/kernel.bin

binary: bin/kernel.bin

# Test with QEMU (adjust memory size as needed)
qemu: binary
	qemu-system-riscv64 -machine virt -cpu rv64 -kernel bin/kernel.bin -m 512M -nographic -no-reboot

clean:
	rm -f bin/*.*

.PHONY: all binary qemu clean
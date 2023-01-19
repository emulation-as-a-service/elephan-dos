NAME = elephan-dos
all: $(NAME)

.NOTINTERMEDIATE: $(NAME).elf

%: %.c

%.elf: %.c
	$(CC) -g -Os -march=i386 -m16 -nostdlib -ffreestanding $< -o $@

%: %.elf
	objdump -S -mi8086 $<
	objcopy $< -O binary $@ -j .text
	printf "$(shell printf '\\%o' 0x55 0xaa)" | dd of=$@ conv=notrunc seek=510 bs=1
	hexdump -C $@
	if test "$$(wc -c < $@)" != "512"; then echo Image too large >&2; exit 1; fi

clean:
	rm -f $(NAME) $(NAME).elf

GNU_EFI	:= gnu-efi
EFI_INC := $(GNU_EFI)/inc
EFI_LIB := $(GNU_EFI)/lib
OVMF    := /usr/share/OVMF
PFLASH	:= $(OVMF)/OVMF_CODE_4M.fd

CPU		:= EPYC
CORES	:= 4
MEMORY	:= 8192

ARCH    := x86_64
CC      := $(ARCH)-w64-mingw32-gcc
LD		:= $(ARCH)-w64-mingw32-gcc

INCLUDE := -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol -Iinclude -Iinclude/lib
LIBRARY := -L$(GNU_EFI)/$(ARCH)/lib -L$(GNU_EFI)/$(ARCH)/gnuefi
CFLAGS  := -Wall -Wextra -O0 -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mcmodel=large -mno-red-zone $(INCLUDE)
LDFLAGS := -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main $(LIBRARY)

BOOT_SRC	:= $(wildcard boot/*.c)
BOOT_OBJ	:= $(patsubst boot/%.c, obj/boot/%.o, $(BOOT_SRC))
KERNEL_SRC	:= $(wildcard kernel/*.c kernel/memory/*.c kernel/io/*.c kernel/graphics/*.c)
KERNEL_OBJ	:= $(patsubst kernel/%.c, obj/kernel/%.o, $(KERNEL_SRC))
LIB_SRC		:= $(wildcard lib/*.c)
LIB_OBJ		:= $(patsubst lib/%.c, obj/lib/%.o, $(LIB_SRC))

OBJECTS		:= $(BOOT_OBJ) $(KERNEL_OBJ) $(LIB_OBJ)
EFI_OUTPUT	:= out/BOOTX64.EFI

all: $(EFI_OUTPUT)

$(EFI_OUTPUT): obj/gnu-efi/lib/data.o $(OBJECTS)
	mkdir -p out
	$(LD) $(LDFLAGS) -o out/BOOTX64.EFI $(OBJECTS) obj/gnu-efi/lib/data.o

obj/gnu-efi/lib/data.o:
	mkdir -p obj/gnu-efi/lib
	$(CC) $(CFLAGS) -c $(EFI_LIB)/data.c -o $@

obj/boot/%.o: boot/%.c
	mkdir -p obj/boot
	$(CC) $(CFLAGS) -c $< -o $@

obj/kernel/idt.o: kernel/idt.c
	mkdir -p obj/kernel
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@

obj/kernel/%.o: kernel/%.c
	mkdir -p obj/kernel
	mkdir -p obj/kernel/memory
	mkdir -p obj/kernel/graphics
	mkdir -p obj/kernel/io
	$(CC) $(CFLAGS) -c $< -o $@

obj/lib/%.o: lib/%.c
	mkdir -p obj/lib
	$(CC) $(CFLAGS) -c $< -o $@

usb: $(EFI_OUTPUT)
	dd if=/dev/zero of=out/usb.img bs=1k count=1440
	mformat -i out/usb.img -f 1440 ::
	mmd -i out/usb.img ::/EFI
	mmd -i out/usb.img ::/EFI/BOOT
	mcopy -i out/usb.img $(EFI_OUTPUT) ::/EFI/BOOT

qemu: usb
	qemu-system-$(ARCH) -L $(OVMF) -pflash $(PFLASH) \
		-cpu $(CPU) -enable-kvm -smp $(CORES) -m $(MEMORY) \
		-serial stdio \
		-usb -drive if=none,id=usbstick,format=raw,file=out/usb.img \
		-device usb-ehci,id=ehci \
		-device usb-storage,bus=ehci.0,drive=usbstick

clean:
	rm -rf obj/* out/*

.PHONY: all clean

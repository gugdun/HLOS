ARCH		:= x86_64
TARGET		:= pei-x86-64
EFI_OUTPUT	:= out/BOOTX64.EFI

GNU_EFI	:= gnu-efi
EFI_INC := $(GNU_EFI)/inc
EFI_LIB := $(GNU_EFI)/lib
OVMF    := /usr/share/OVMF
PFLASH	:= $(OVMF)/OVMF_CODE_4M.fd

CPU		:= EPYC
CORES	:= 2
MEMORY	:= 4096

TOOLCHAIN	:= $(HOME)/opt/$(ARCH)-hlos
CC      	:= $(TOOLCHAIN)/bin/$(ARCH)-elf-gcc
LD			:= $(TOOLCHAIN)/bin/$(ARCH)-elf-ld
OBJCOPY		:= $(TOOLCHAIN)/bin/$(ARCH)-pe-objcopy

DEBUG	:= -DHLOS_DEBUG
INCLUDE := -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol -Iinclude -Iinclude/lib
LIBRARY := -L$(GNU_EFI)/$(ARCH)/lib -L$(GNU_EFI)/$(ARCH)/gnuefi
CFLAGS  := $(DEBUG) $(INCLUDE) -Wall -Wextra -O2 -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mcmodel=large -mno-red-zone
LDFLAGS := -shared -Bsymbolic -z noexecstack -L$(GNU_EFI)/$(ARCH)/lib -L$(GNU_EFI)/$(ARCH)/gnuefi -T$(GNU_EFI)/gnuefi/elf_$(ARCH)_efi.lds $(LIBRARY)

BOOT_SRC	:= $(wildcard boot/*.c)
BOOT_OBJ	:= $(patsubst boot/%.c, obj/boot/%.o, $(BOOT_SRC))
KERNEL_SRC	:= $(wildcard kernel/*.c kernel/cpu/*.c kernel/memory/*.c kernel/io/*.c kernel/graphics/*.c kernel/interrupts/*.c kernel/timer/*.c kernel/fs/*.c)
KERNEL_OBJ	:= $(patsubst kernel/%.c, obj/kernel/%.o, $(KERNEL_SRC))
LIB_SRC		:= $(wildcard lib/*.c)
LIB_OBJ		:= $(patsubst lib/%.c, obj/lib/%.o, $(LIB_SRC))
DEMO_SRC	:= $(wildcard demo/*.c)
DEMO_OBJ	:= $(patsubst demo/%.c, obj/demo/%.o, $(DEMO_SRC))

OBJECTS		:= $(BOOT_OBJ) $(KERNEL_OBJ) $(LIB_OBJ) $(DEMO_OBJ)

all: $(EFI_OUTPUT)

$(EFI_OUTPUT): $(OBJECTS)
	@mkdir -p out
	@echo "Linking $(EFI_OUTPUT)..."
	@$(LD) $(LDFLAGS) -o out/kernel.so $(GNU_EFI)/$(ARCH)/gnuefi/crt0-efi-$(ARCH).o $(OBJECTS) -lgnuefi -lefi
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
		--target $(TARGET) --subsystem=10 out/kernel.so $(EFI_OUTPUT)
	@echo "Done!"

obj/boot/%.o: boot/%.c
	@mkdir -p obj/boot
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

obj/kernel/interrupts/isrs.o: kernel/interrupts/isrs.c
	@mkdir -p obj/kernel
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@

obj/kernel/%.o: kernel/%.c
	@mkdir -p obj/kernel
	@mkdir -p obj/kernel/cpu
	@mkdir -p obj/kernel/memory
	@mkdir -p obj/kernel/graphics
	@mkdir -p obj/kernel/io
	@mkdir -p obj/kernel/timer
	@mkdir -p obj/kernel/interrupts
	@mkdir -p obj/kernel/fs
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

obj/lib/%.o: lib/%.c
	@mkdir -p obj/lib
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

obj/demo/%.o: demo/%.c
	@mkdir -p obj/demo
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

gnu-efi:
	@echo "Compiling gnu-efi..."
	@make -C gnu-efi
	@echo "Done!"

initrd:
	@echo "Building initrd..."
	@tar --format=ustar -cf out/initrd.tar initrd
	@echo "Done!"

usb: $(EFI_OUTPUT) initrd
	@echo "Building USB image..."
	@dd if=/dev/zero of=out/usb.img bs=1k count=1440
	@mformat -i out/usb.img -f 1440 ::
	@mmd -i out/usb.img ::/EFI
	@mmd -i out/usb.img ::/EFI/BOOT
	@mcopy -i out/usb.img $(EFI_OUTPUT) ::/EFI/BOOT
	@mcopy -i out/usb.img out/initrd.tar ::/EFI/BOOT
	@echo "Done!"

qemu: usb
	@echo "Starting QEMU..."
	@qemu-system-$(ARCH) -L $(OVMF) -pflash $(PFLASH) \
		-cpu $(CPU) -enable-kvm -smp $(CORES) -m $(MEMORY) \
		-serial stdio \
		-usb -drive if=none,id=usbstick,format=raw,file=out/usb.img \
		-device usb-ehci,id=ehci \
		-device usb-storage,bus=ehci.0,drive=usbstick

clean:
	@echo "Cleaning..."
	@rm -rf obj out
	@echo "Done!"

.PHONY: all initrd clean

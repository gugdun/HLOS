GNU_EFI	:= gnu-efi
EFI_INC := $(GNU_EFI)/inc
EFI_LIB := $(GNU_EFI)/lib
OVMF    := /usr/share/OVMF
PFLASH	:= $(OVMF)/OVMF_CODE_4M.fd

CPU		:= EPYC
CORES	:= 2
MEMORY	:= 4096

CC      := x86_64-w64-mingw32-gcc
LD		:= x86_64-w64-mingw32-gcc
DEBUG	:= -DHLOS_DEBUG
INCLUDE := -I$(EFI_INC) -I$(EFI_INC)/x86_64 -I$(EFI_INC)/protocol -Iinclude -Iinclude/lib
LIBRARY := -L$(GNU_EFI)/x86_64/lib -L$(GNU_EFI)/x86_64/gnuefi
CFLAGS  := $(DEBUG) -Wall -Wextra -O2 -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mcmodel=large -mno-red-zone $(INCLUDE)
LDFLAGS := -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main $(LIBRARY)

BOOT_SRC	:= $(wildcard boot/*.c)
BOOT_OBJ	:= $(patsubst boot/%.c, obj/boot/%.o, $(BOOT_SRC))
KERNEL_SRC	:= $(wildcard kernel/*.c kernel/cpu/*.c kernel/memory/*.c kernel/io/*.c kernel/graphics/*.c kernel/interrupts/*.c kernel/timer/*.c kernel/fs/*.c)
KERNEL_OBJ	:= $(patsubst kernel/%.c, obj/kernel/%.o, $(KERNEL_SRC))
LIB_SRC		:= $(wildcard lib/*.c)
LIB_OBJ		:= $(patsubst lib/%.c, obj/lib/%.o, $(LIB_SRC))
DEMO_SRC	:= $(wildcard demo/*.c)
DEMO_OBJ	:= $(patsubst demo/%.c, obj/demo/%.o, $(DEMO_SRC))

OBJECTS		:= $(BOOT_OBJ) $(KERNEL_OBJ) $(LIB_OBJ) $(DEMO_OBJ)
EFI_OUTPUT	:= out/BOOTX64.EFI

all: $(EFI_OUTPUT)

$(EFI_OUTPUT): obj/gnu-efi/lib/data.o $(OBJECTS)
	@mkdir -p out
	@echo "Linking $(EFI_OUTPUT)..."
	@$(LD) $(LDFLAGS) -o $(EFI_OUTPUT) $(OBJECTS) obj/gnu-efi/lib/data.o
	@echo "Done!"

obj/gnu-efi/lib/data.o: $(EFI_LIB)/data.c
	@mkdir -p obj/gnu-efi/lib
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

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
	@qemu-system-x86_64 -L $(OVMF) -pflash $(PFLASH) \
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

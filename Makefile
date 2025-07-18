ARCH		:= x86_64
TARGET		:= pei-x86-64
EFI_OUTPUT	:= out/BOOTX64.EFI
SYSROOT		:= /opt/$(ARCH)-hlos

GNU_EFI	:= gnu-efi
EFI_INC := $(GNU_EFI)/inc
EFI_LIB := $(GNU_EFI)/lib
OVMF    := /usr/share/OVMF
PFLASH	:= $(OVMF)/OVMF_CODE_4M.fd

CPU		:= EPYC
CORES	:= 1
MEMORY	:= 512

CC      := $(SYSROOT)/bin/$(ARCH)-hlos-gcc
LD		:= $(SYSROOT)/bin/$(ARCH)-hlos-ld
OBJCOPY	:= $(ARCH)-w64-mingw32-objcopy

DEBUG	:= -DHLOS_DEBUG
DEFINES	:= $(DEBUG) -DARCH_$(ARCH)
INCLUDE := -I$(SYSROOT)/usr/$(ARCH)-hlos/include -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol -Iinclude
LIBRARY := -L$(SYSROOT)/usr/$(ARCH)-hlos/lib -L$(GNU_EFI)/$(ARCH)/lib -L$(GNU_EFI)/$(ARCH)/gnuefi
CFLAGS  := $(DEFINES) $(INCLUDE) -Wall -Wextra -O0 -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mcmodel=large -mno-red-zone
LDFLAGS := -shared -Bsymbolic -z noexecstack $(LIBRARY) -T$(GNU_EFI)/gnuefi/elf_$(ARCH)_efi.lds

ANOMALOUS_SRC	:= $(wildcard anomalous/*.c)
ANOMALOUS_OBJ	:= $(patsubst anomalous/%.c, obj/anomalous/%.o, $(ANOMALOUS_SRC))
XENCORE_SRC		:= $(wildcard xencore/*.c xencore/gman/*.c xencore/hazardous/*.c xencore/xenlib/*.c xencore/xenmem/*.c xencore/xenio/*.c xencore/graphics/*.c xencore/timer/*.c xencore/xenfs/*.c xencore/arch/$(ARCH)/*.c)
XENCORE_OBJ		:= $(patsubst xencore/%.c, obj/xencore/%.o, $(XENCORE_SRC))
DEMO_SRC		:= $(wildcard demo/*.c)
DEMO_OBJ		:= $(patsubst demo/%.c, obj/demo/%.o, $(DEMO_SRC))
OBJECTS			:= $(ANOMALOUS_OBJ) $(XENCORE_OBJ) $(DEMO_OBJ)

export ARCH SYSROOT CC

all: $(EFI_OUTPUT)

$(EFI_OUTPUT): $(OBJECTS)
	@mkdir -p out
	@echo "Linking $(EFI_OUTPUT)..."
	@$(LD) $(LDFLAGS) -o out/xencore.so $(GNU_EFI)/$(ARCH)/gnuefi/crt0-efi-$(ARCH).o $(OBJECTS) -lc -lg -lm -lnosys -lgnuefi -lefi
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
		--target $(TARGET) --subsystem=10 out/xencore.so $(EFI_OUTPUT)
	@echo "Done!"

obj/anomalous/%.o: anomalous/%.c
	@mkdir -p obj/anomalous
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

obj/xencore/arch/x86_64/isrs.o: xencore/arch/x86_64/isrs.c
	@mkdir -p obj/xencore
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@

obj/xencore/%.o: xencore/%.c
	@mkdir -p obj/xencore
	@mkdir -p obj/xencore/gman
	@mkdir -p obj/xencore/xenmem
	@mkdir -p obj/xencore/xenlib
	@mkdir -p obj/xencore/xenio
	@mkdir -p obj/xencore/xenfs
	@mkdir -p obj/xencore/timer
	@mkdir -p obj/xencore/graphics
	@mkdir -p obj/xencore/hazardous
	@mkdir -p obj/xencore/arch/$(ARCH)
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

hazardous:
	@echo "Building hazardous materials..."
	@mkdir -p out
	@make -C hazardous
	@echo "Done!"

test_sample: hazardous
	@echo "Building test_sample..."
	@mkdir -p out
	@tar --format=ustar -cf out/test_sample.tar test_sample
	@tar --append --file=out/test_sample.tar --transform='s|^|test_sample/|' \
		-C out $(patsubst out/%.elf, %.elf, $(wildcard out/*.elf))
	@echo "Done!"

usb: $(EFI_OUTPUT) test_sample
	@echo "Building USB image..."
	@dd if=/dev/zero of=out/usb.img bs=1K count=1440
	@mformat -i out/usb.img ::
	@mmd -i out/usb.img ::/EFI
	@mmd -i out/usb.img ::/EFI/BOOT
	@mcopy -i out/usb.img $(EFI_OUTPUT) ::/EFI/BOOT
	@mcopy -i out/usb.img out/test_sample.tar ::/EFI/BOOT
	@echo "Done!"

qemu: usb
	@echo "Starting QEMU..."
	@qemu-system-$(ARCH) -L $(OVMF) -pflash $(PFLASH) \
		-cpu $(CPU) -smp $(CORES) -m $(MEMORY) \
		-serial stdio \
		-usb -drive if=none,id=usbstick,format=raw,file=out/usb.img \
		-device usb-ehci,id=ehci \
		-device usb-storage,bus=ehci.0,drive=usbstick

clean:
	@echo "Cleaning..."
	@rm -rf obj out
	@echo "Done!"

.PHONY: all hazardous test_sample clean

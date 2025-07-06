# HLOS

**HLOS** is a modern, multitasking operating system designed for x86\_64 architecture. It’s built from the ground up to be minimal, fast, and educational.

## Prerequisites

Tested on **Ubuntu 24.04**. Make sure you have the following packages installed:

### Kernel Compilation

```bash
sudo apt-get install gcc-mingw-w64
```

### Building and Running via QEMU

```bash
sudo apt-get install qemu-system-x86 ovmf mtools
```

## Building

To compile the kernel:

```bash
make
```

## Running

To build the USB image and launch the OS in QEMU:

```bash
make qemu
```

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).
Copyright © 2025 **gugdun**

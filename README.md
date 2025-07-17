# HLOS - Half-Life OS

**HLOS** is a hobby operating system inspired by the atmosphere and aesthetics of **Half-Life 1**. It features a graphical interface styled after the original Half-Life main menu and is filled with subtle references, Easter eggs, and theming from the game.

## 🚀 Features

* UEFI-only boot support
* Currently targets `x86_64` architecture
* Future support planned for `aarch64`
* Graphical UI styled after Half-Life 1
* Custom build toolchain (`x86_64-hlos`)

## 🛠️ Toolchain Requirements

HLOS requires a cross-compilation toolchain targeting `x86_64-hlos`. You can either download prebuilt binaries or build the toolchain from source.

### 📦 Prebuilt Binaries

Precompiled versions of `binutils`, `gcc`, and `newlib` are available in Releases section:

* [hlos-binutils](https://github.com/gugdun/hlos-binutils)
* [hlos-gcc](https://github.com/gugdun/hlos-gcc)
* [hlos-newlib](https://github.com/gugdun/hlos-newlib)

Install these to `/opt`.

## 🔧 Building the Toolchain from Source

### 1. Clone Repositories

Clone all required repositories to e.g. `$HOME/src`:

```bash
cd $HOME/src
git clone https://github.com/gugdun/hlos-binutils.git
git clone https://github.com/gugdun/hlos-gcc.git
```

### 2. Install Sysroot

Download and extract the sysroot from `hlos-newlib` Releases into:

```bash
$HOME/opt/x86_64-hlos
```

### 3. Build Binutils

```bash
mkdir build-binutils && cd build-binutils
../hlos-binutils/configure \
  --target=x86_64-hlos \
  --prefix=$HOME/opt/x86_64-hlos \
  --with-sysroot=$HOME/opt/x86_64-hlos \
  --disable-werror
make -j$(nproc)
make install
```

### 4. Build GCC

```bash
mkdir ../build-gcc && cd ../build-gcc
../hlos-gcc/configure \
  --target=x86_64-hlos \
  --prefix=$HOME/opt/x86_64-hlos \
  --with-sysroot=$HOME/opt/x86_64-hlos \
  --enable-languages=c
make -j$(nproc) all-gcc all-target-libgcc
make install-gcc install-target-libgcc
ln -s $HOME/opt/x86_64-hlos/bin/x86_64-hlos-gcc \
      $HOME/opt/x86_64-hlos/bin/x86_64-hlos-cc
```

### 5. Update Makefile

Make sure the `Makefile` in HLOS points to the correct sysroot path.

## 🔁 Optional: Building newlib

If needed, rebuild `newlib` with:

```bash
export PATH="$HOME/opt/bin:$PATH"
mkdir build-newlib && cd build-newlib
../hlos-newlib/configure \
  --prefix=/usr \
  --target=x86_64-hlos \
  --enable-newlib-io-long-long \
  --enable-io-long-double \
  CFLAGS_FOR_TARGET="-g -O2 -fPIC"
make -j$(nproc)
make DESTDIR=$HOME/opt/x86_64-hlos install
ln -s $HOME/opt/x86_64-hlos/usr/x86_64-hlos/include \
      $HOME/opt/x86_64-hlos/usr/include
ln -s $HOME/opt/x86_64-hlos/usr/x86_64-hlos/lib \
      $HOME/opt/x86_64-hlos/usr/lib
```

## 🧱 Building HLOS

### 🔨 Prerequisites

Make sure the following packages are installed:

* `gcc-mingw-w64` – for generating PE files from ELF (UEFI bootloader)
* `tar` – for building `test_sample`
* `mtools` – for generating bootable USB images
* `qemu-system-x86`, `ovmf` – for QEMU testing

### 🔧 One-Time Setup

Run once to build GNU-EFI:

```bash
make gnu-efi
```

### 🧵 Build Targets

* Build the kernel:

```bash
make
```

* Build the `test_sample`:

```bash
make test_sample
```

* Run in QEMU:

```bash
make qemu
```

## 📄 License

MIT License
(c) 2025 **gugdun**

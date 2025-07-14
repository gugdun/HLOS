# HLOS - A x86_64 UEFI Operating System

**HLOS** is an operating system designed for the `x86_64` architecture with UEFI boot support. It is built from the ground up with a custom toolchain and a modern kernel.

## 🛠️ Requirements

Make sure the following packages are installed on your system:

```bash
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev
````

## 📦 Setup Toolchain

You'll need to build a custom `binutils` and `gcc` cross-compiler targeting `x86_64-elf` and `x86_64-pe`. Start by cloning the required repositories:

```bash
mkdir -p $HOME/src
cd $HOME/src
git clone https://github.com/gugdun/hlos-binutils
git clone https://github.com/gugdun/hlos-gcc
```

### Environment Variables

Before building, export these environment variables:

```bash
export PREFIX="$HOME/opt/x86_64-hlos"
export TARGET=x86_64-hlos
export PATH="$PREFIX/bin:$PATH"
```

## 🔧 Building the Toolchain

### 1. Build `binutils` (x86\_64-hlos)

```bash
cd $HOME/src
mkdir binutils-build
cd binutils-build
../binutils-hlos/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
```

### 2. Build `binutils` (x86\_64-pe)

Repeat the above process, replacing `--target=$TARGET` with `--target=x86_64-pe`.

### 3. Build `GCC` (x86\_64-hlos)

```bash
cd $HOME/src
mkdir gcc-build
cd gcc-build
../gcc-hlos/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
```

## 🧵 Building HLOS

### 1. Build the Kernel

```bash
make gnu-efi   # Run this only once
make -j$(nproc)
```

### 2. Build the Initrd

```bash
make initrd
```

## 🚀 Running in QEMU

You can run HLOS in a QEMU virtual machine using:

```bash
make qemu
```

## 📄 License

**MIT License**
© 2025 [gugdun](https://github.com/gugdun)

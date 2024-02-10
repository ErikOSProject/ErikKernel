# ErikKernel

ErikKernel is the OS kernel for ErikOS.

## Usage

You can build ErikKernel with CMake: 

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../x64.cmake ..
make
```

The CMake variable `CMAKE_TOOLCHAIN_FILE` needs to point to a toolchain specification. This repository includes aarch64.cmake and x64.cmake that use clang to cross-compile for their respective architectures.

The resulting file, `KERNEL.ERIK`, can be booted with ErikBoot.

## License

ErikKernel is licensed under [BSD-2-Clause](COPYING) license.

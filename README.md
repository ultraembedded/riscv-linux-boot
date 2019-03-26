# Trivial RISC-V Linux Binary Bootloader

Github: http://github.com/ultraembedded/riscv-linux-boot

A very simple bootstrap for starting the Linux kernel on RISC-V.  
This takes a vmlinux ELF and a device tree (DTS) file, converts them to binaries,
and then embeds these into the bootstrap ELF.

## Cloning
```
git clone https://github.com/ultraembedded/riscv-linux-boot.git
```

## Building
```
make LINUX_DIR=/path/to/riscv-linux DTS_FILE=/path/to/config.dts
```

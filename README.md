# Trivial RISC-V Linux Binary Bootloader

Github: http://github.com/ultraembedded/riscv-linux-boot

A very simple bootstrap for starting the Linux kernel on RISC-V.  
This takes a vmlinux ELF and a device tree (DTS) file, converts them to binaries,
and then embeds these into the bootstrap ELF.

If atomic instruction support is not present on the target platform, the atomic instruction set will be emulated in SW.

This bootloader implements the required SBI calls used by standard RISC-V Linux kernel builds.

Used by the biRISC-V core to boot Linux: [http://github.com/ultraembedded/biriscv](http://github.com/ultraembedded/biriscv)

## Hardware Dependencies
* UART: Xilinx UARTLite style UART @ 0x92000000
* Timer: Non-std RISC-V mtime, mtimecmp implementation.

## Cloning
```
git clone https://github.com/ultraembedded/riscv-linux-boot.git
```

## Building
```
make LINUX_DIR=/path/to/riscv-linux VMLINUX=/path/to/vmlinux[.elf] DTS_FILE=/path/to/config.dts
```

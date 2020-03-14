###############################################################################
# Variables: System specific options
###############################################################################
# Linux image
VMLINUX ?=
ifeq ($(VMLINUX),)
  ${error VMLINUX must be specified}
endif
DTS_FILE ?=
ifeq ($(DTS_FILE),)
  ${error DTS_FILE must be specified}
endif
LINUX_DIR ?=
ifeq ($(LINUX_DIR),)
  ${error LINUX_DIR must be specified (LINUX kernel source tree base)}
endif

# Toolchain path/prefix
TOOLCHAIN_PREFIX ?= riscv32-unknown-elf-

###############################################################################
# Variables: Safe defaults (no changes required)
###############################################################################
# Target name
ELF_NAME    ?= riscv-linux-boot.elf

# VMLINUX Binary
PAYLOAD     ?= vmlinux.bin

# DTB Binary
DTB         ?= config.dtb

# Source files
SRC_DIR      = .

# Directories
ELF_DIR     ?=
OBJ_DIR     ?= $(abspath ./obj)/

###############################################################################
# Variables: Source files
###############################################################################
SRC :=   boot.S \
		 $(foreach src,$(SRC_DIR),$(filter-out $(src)/boot.S, $(wildcard $(src)/*.S))) \
		 $(foreach src,$(SRC_DIR),$(wildcard $(src)/*.c))

###############################################################################
# Variables: Compiler Options
###############################################################################
EXTRA_CFLAGS = -DPAYLOAD_BINARY=\"$(PAYLOAD)\"
EXTRA_CFLAGS+= -DDTB_BINARY=\"$(DTB)\"
EXTRA_CFLAGS+= -Wno-unused-variable 

# Options
BASE_ADDRESS      = 0x80000000
PLATFORM_LDFLAGS  = -nostartfiles -nodefaultlibs -nostdlib -lgcc -T./custom.ld

OPT        ?= 2
CFLAGS	   := -Ttext $(BASE_ADDRESS) -O$(OPT) -g -Wall $(patsubst %,-I%,$(SRC_DIR)) $(EXTRA_CFLAGS)
ASFLAGS    := 
LDFLAGS    := $(PLATFORM_LDFLAGS) -Wl,--defsym=BASE_ADDRESS=$(BASE_ADDRESS)

###############################################################################
# Variables: Toolchain
###############################################################################
CC          = $(TOOLCHAIN_PREFIX)gcc
AS          = $(TOOLCHAIN_PREFIX)as
LD          = $(TOOLCHAIN_PREFIX)ld
OBJDUMP     = $(TOOLCHAIN_PREFIX)objdump
OBJCOPY     = $(TOOLCHAIN_PREFIX)objcopy

###############################################################################
# Variables: SRC / Object list
###############################################################################
src2obj = $(OBJ_DIR)$(patsubst %$(suffix $(1)),%.o,$(notdir $(1)))
OBJ    := $(foreach src,$(SRC),$(call src2obj,$(src)))

###############################################################################
# Rules
###############################################################################
all: $(ELF_DIR)$(ELF_NAME)

$(OBJ_DIR) $(ELF_DIR):
	@mkdir -p $@

clean:
	@echo "# Cleaning"
	@rm -rf $(OBJ_DIR) $(OBJ) $(ELF_DIR)$(ELF_NAME) $(PAYLOAD) $(DTB)

define template_c
$(call src2obj,$(1)): $(1) $(PAYLOAD) $(DTB) | $(OBJ_DIR)
	@echo "# CC $(notdir $$<)"
	@$(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(SRC),$(eval $(call template_c,$(src))))

$(ELF_DIR)$(ELF_NAME): $(OBJ) | $(ELF_DIR)
	@echo "# LD $(notdir $@)"
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Kernel to binary
$(PAYLOAD): $(VMLINUX)
	@$(OBJCOPY) -O binary $< $@

# Device tree to binary
$(DTB): $(DTS_FILE)
	$(LINUX_DIR)/scripts/dtc/dtc -I dts -O dtb -o $@ $<

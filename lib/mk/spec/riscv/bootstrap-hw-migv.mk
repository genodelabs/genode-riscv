INC_DIR += $(REP_DIR)/src/bootstrap/board/migv

SRC_CC  += bootstrap/platform_cpu_memory_area.cc
SRC_CC  += bootstrap/spec/riscv/platform.cc
SRC_S   += bootstrap/spec/riscv/crt0.s
SRC_CC  += lib/base/riscv/kernel/interface.cc

ARCH_WIDTH_PATH := spec/64bit

include $(call select_from_repositories,lib/mk/bootstrap-hw.inc)

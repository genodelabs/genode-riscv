INC_DIR += $(REP_DIR)/src/bootstrap/board/virt_qemu_riscv

SRC_CC  += bootstrap/spec/riscv/platform.cc
SRC_S   += bootstrap/spec/riscv/crt0.s
SRC_CC  += lib/base/riscv/kernel/interface.cc
SRC_CC  += spec/64bit/memory_map.cc

vpath spec/64bit/memory_map.cc $(call select_from_repositories,/src/lib/hw)

include $(call select_from_repositories,lib/mk/bootstrap-hw.inc)

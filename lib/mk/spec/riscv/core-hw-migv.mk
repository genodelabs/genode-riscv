REP_INC_DIR += src/core/spec/riscv src/core/board/migv

CC_OPT += -fno-delete-null-pointer-checks

# add C++ sources
SRC_CC += platform_services.cc
SRC_CC += board/migv/timer.cc
SRC_CC += kernel/vcpu_thread_off.cc
SRC_CC += kernel/cpu_up.cc
SRC_CC += kernel/mutex.cc
SRC_CC += spec/riscv/kernel/thread.cc
SRC_CC += spec/riscv/kernel/cpu.cc
SRC_CC += spec/riscv/kernel/interface.cc
SRC_CC += spec/riscv/kernel/pd.cc
SRC_CC += spec/riscv/cpu.cc
SRC_CC += spec/riscv/pic.cc
SRC_CC += spec/riscv/platform_support.cc

#add assembly sources
SRC_S += spec/riscv/exception_vector.s
SRC_S += spec/riscv/crt0.s

ARCH_WIDTH_PATH := spec/64bit
vpath board/migv/timer.cc $(REP_DIR)/src/core

# include less specific configuration
include $(call select_from_repositories,lib/mk/core-hw.inc)

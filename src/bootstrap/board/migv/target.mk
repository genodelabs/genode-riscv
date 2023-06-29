TARGET = bootstrap-sram-migv

LD_TEXT_ADDR = 0x1010000

LIBS += cxx

SRC_CC = bootstrap_sdram.cc
SRC_S  = crt0.s

SRC_CC += bootstrap/log.cc
SRC_CC += bootstrap/lock.cc
SRC_CC += bootstrap/thread.cc
SRC_CC += lib/base/log.cc
SRC_CC += lib/base/output.cc
SRC_CC += lib/base/raw_output.cc
SRC_CC += lib/base/avl_tree.cc
SRC_CC += lib/base/registry.cc
SRC_CC += lib/base/heap.cc
SRC_CC += lib/base/slab.cc
SRC_CC += lib/base/allocator_avl.cc
SRC_CC += lib/base/sleep.cc
SRC_CC += hw/capability.cc

INC_DIR += $(call select_from_repositories,src/include)
INC_DIR += $(BASE_DIR)/src/include
INC_DIR += $(REP_DIR)/src/include
INC_DIR += $(REP_DIR)/src/bootstrap/board/migv

vpath crt0.s $(call select_from_repositories,src/bootstrap/spec/riscv)

vpath bootstrap/%   $(call select_from_repositories,src)
vpath lib/base/%    $(BASE_DIR)/src
vpath hw/%          $(call select_from_repositories,src/lib)

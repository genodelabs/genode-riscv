/*
 * \brief  Board spcecification
 * \author Sebastian Sumpf
 * \date   2015-06-02
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__SPEC__RISCV_QEMU__BOARD_H_
#define _CORE__SPEC__RISCV_QEMU__BOARD_H_

/* base-hw internal includes */
#include <hw/spec/riscv/qemu_board.h>

/* base-hw Core includes */
#include <spec/riscv/pic.h>
#include <spec/riscv/cpu.h>
#include <spec/riscv/timer.h>

namespace Board { using namespace Hw::Riscv_board; }

#endif /* _CORE__SPEC__RISCV_QEMU__BOARD_H_ */

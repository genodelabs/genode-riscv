/*
 * \brief  Board spcecification
 * \author Sebastian Sumpf
 * \date   2021-02-09
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__BOARD__MIGV__BOARD_H_
#define _CORE__BOARD__MIGV__BOARD_H_

#include <hw/spec/riscv/migv_board.h>
#include <spec/riscv/cpu.h>
#include <spec/riscv/pic.h>
#include <no_vcpu_board.h>

namespace Board { using namespace Hw::Riscv_board; }

#include <spec/riscv/timer.h>

#endif /* _CORE__BOARD__MIGV__BOARD_H_ */

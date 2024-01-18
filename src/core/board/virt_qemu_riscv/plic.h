/**
 * \brief  Platform-level interrupt controller layout (PLIC)
 * \author Sebastian Sumpf
 * \date   2021-03-05
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__SPEC__RISCV_QEMU__PLIC_H_
#define _CORE__SPEC__RISCV_QEMU__PLIC_H_

namespace Board { class Plic; }

struct Board::Plic : Genode::Mmio<0x200000 + 0x1 + 0x1000 + 0x4 + 4>
{
		enum {
			/*
			 * VIRT_IRQCHIP_NUM_SOURCES from Qemu include/hw/riscv/virt.h.
			 */
			NR_OF_IRQ = 96,
		};

		/*
		 * Single core Qemu virt machine uses context 1 for
		 * supervisor mode interrupts and 0 for machine mode.
		 */
		static constexpr Genode::size_t CONTEXT        = 0x1;
		static constexpr Genode::addr_t ENABLE_BASE    = 0x2000;
		static constexpr Genode::size_t ENABLE_STRIDE  = 0x80;
		static constexpr Genode::addr_t CONTEXT_BASE   = 0x200000;
		static constexpr Genode::size_t CONTEXT_STRIDE = 0x1000;
		static constexpr Genode::addr_t ENABLE_ADDR    = ENABLE_BASE + CONTEXT * ENABLE_STRIDE;
		static constexpr Genode::addr_t PRI_THR_ADDR   = CONTEXT_BASE + CONTEXT * CONTEXT_STRIDE;
		static constexpr Genode::addr_t ID_ADDR        = CONTEXT_BASE + CONTEXT * CONTEXT_STRIDE + 0x4;

		struct Priority           : Register_array<0x4, 32, NR_OF_IRQ - 1, 32> { };
		struct Enable             : Register_array<ENABLE_ADDR, 32, NR_OF_IRQ, 1> { };
		struct Priority_threshold : Register<PRI_THR_ADDR, 32> { };
		struct Id                 : Register<ID_ADDR, 32> { };

		Plic(Genode::Byte_range_ptr const &range)
		:
			Mmio(range)
		{
			write<Priority_threshold>(0);
			for (unsigned i = 0; i < NR_OF_IRQ - 1; ++i)
				write<Priority>(1, i);
		}

		void enable(unsigned value, unsigned irq)
		{
			write<Enable>(value, irq);
		}

		void el(unsigned, unsigned) { }
};

#endif /* _CORE__SPEC__RISCV_QEMU__PLIC_H_ */

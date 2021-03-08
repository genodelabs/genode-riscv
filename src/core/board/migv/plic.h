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

#ifndef _CORE__SPEC__MIGV__PLIC_H_
#define _CORE__SPEC__MIGV__PLIC_H_

namespace Board { class Plic; }

struct Board::Plic : Genode::Mmio
{
		enum { NR_OF_IRQ = 24 };

		struct El       : Register_array<0x8, 32, NR_OF_IRQ, 1>  { };
		struct Priority : Register_array<0xc, 32, NR_OF_IRQ, 4>  { };
		struct Enable   : Register_array<0x1c, 32, NR_OF_IRQ, 1> { };
		struct Id       : Register<0x2c, 32> { };

		Plic(Genode::addr_t const base)
		:
			Mmio(base)
		{
			/* set all priorities to 1 */
			for (unsigned i = 0; i < NR_OF_IRQ; i++)
				write<Priority>(1, i);
		}

		void enable(unsigned value, unsigned irq)
		{
			write<Enable>(value, irq - 1);
		}

		void el(unsigned value, unsigned irq)
		{
			write<El>(value, irq - 1);
		}
};

#endif /* _CORE__SPEC__MIGV__PLIC_H_ */

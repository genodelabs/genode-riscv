/*
 * \brief   RISC-V second stage boot loader
 * \author  Sebastian Sumpf
 * \date    2021-02-09
 *
 * This mini-boot loader must be loaded into SRAM and excuted before using
 * SDRAM, it configures the SDRAM PLL and sets the SoC to supervisor mode as
 * expected by Genode's bootstrap component.
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <util/mmio.h>
#include <base/log.h>

struct Soc_configuration : Genode::Mmio
{
	Soc_configuration(Genode::addr_t const mmio_base)
		: Genode::Mmio(mmio_base) { }

	struct Pll_sdr : Register<0x68, 32>
	{
		struct En  : Bitfield<0, 1> { };
		struct Dr  : Bitfield<1, 4> { };
		struct Df  : Bitfield<5, 9> { };
		struct Dp  : Bitfield<14, 4> { };
		struct Hdu : Bitfield<18, 1> { };

	};

	bool check_values(Pll_sdr::access_t const value) const
	{
		return
			(Pll_sdr::En::get(value) == 1 &&
			 Pll_sdr::Dr::get(value) == 2 &&
			 Pll_sdr::Df::get(value) == 75 &&
			 Pll_sdr::Dp::get(value) == 4);
	}

	void configure_pll()
	{
		Pll_sdr::access_t value = read<Pll_sdr>();

		if (check_values(value)) return;

		Pll_sdr::En::set(value, 0);
		Pll_sdr::Dr::set(value, 2);
		Pll_sdr::Df::set(value, 75);
		Pll_sdr::Dp::set(value, 4);
		Pll_sdr::Hdu::set(value, 0);

		write<Pll_sdr>(value);
		write<Pll_sdr::En>(1);
	}
};


struct Mstatus : Genode::Register<64>
{
	enum {
		USER       = 0,
		SUPERVISOR = 1,
	};

	struct Uie    : Bitfield<0, 1> { };
	struct Sie    : Bitfield<1, 1> { };
	struct Mie    : Bitfield<3, 1> { };
	struct Upie   : Bitfield<4, 1> { };
	struct Spie   : Bitfield<5, 1> { };
	struct Spp    : Bitfield<8, 1> { };
	struct Mpp    : Bitfield<11, 2> { };

	struct Fs    : Bitfield<13, 2> { enum { INITIAL = 1 }; };
};


struct Medeleg : Genode::Register<64>
{
	struct Misaligned_fetch : Bitfield<0, 1>  { };
	struct User_ecall       : Bitfield<8, 1>  { };
	struct Fetch_page_fault : Bitfield<12, 1> { };
	struct Load_page_fault  : Bitfield<13, 1> { };
	struct Store_page_fault : Bitfield<15, 1> { };
};


struct Mideleg : Genode::Register<64>
{
	struct Sti  : Bitfield<5, 1> { }; /* supervisor timer interrupt */
	struct Seip : Bitfield<9, 1> { }; /* supervisor external interrupt */
};


extern "C" void init()
{
	Mstatus::access_t mstatus = 0;
	Mstatus::Fs::set(mstatus, Mstatus::Fs::INITIAL); /* enable FPU        */
	Mstatus::Upie::set(mstatus, 1);                  /* user mode interrupt */
	Mstatus::Spp::set(mstatus, Mstatus::USER);       /* set user mode       */
	Mstatus::Sie::set(mstatus, 0);                  /* disable interrupts  */
	Mstatus::Mpp::set(mstatus, Mstatus::SUPERVISOR); /* set supervisor mode */
	Mstatus::Mie::set(mstatus, 1);

	Medeleg::access_t medeleg = 0;
	Medeleg::Misaligned_fetch::set(medeleg, 1);
	Medeleg::User_ecall::set(medeleg, 1);
	Medeleg::Fetch_page_fault::set(medeleg, 1);
	Medeleg::Load_page_fault::set(medeleg, 1);
	Medeleg::Store_page_fault::set(medeleg, 1);

	Mideleg::access_t mideleg = 0;
	Mideleg::Sti::set(mideleg, 1);
	Mideleg::Seip::set(mideleg, 1);

	/* set ROM bootloader stack pointer (taken from MIG-V rom code) */
	constexpr Genode::addr_t rom_sp = 0x1001000ul;

	asm volatile ("la   t0,       1f\n"
	              "csrw mepc,     t0\n"
	              "csrw satp,     x0\n" /* disable paging               */
	              "csrw mstatus,  %0\n" /* change mode                  */
	              "csrw medeleg,  %1\n" /* forward page/access fault    */
	              "csrw mscratch, %2\n" /* set stack pointer for M-mode */
	              "csrw mideleg,  %3\n" /* forward interrupts */
	              "mret             \n" /* supverisor mode, jump to 1f  */
	              "1:               \n"
	              : : "r"(mstatus), "r"(medeleg), "r"(rom_sp), "r"(mideleg)
	              : "memory");

	Genode::log("\n\nswitched to supervisor mode");
	Genode::log("initializing SDRAM ...");

	Soc_configuration(0x40e000).configure_pll();

	Genode::log("initialization complete");
	Genode::log("\nbinaries can be loaded into SDRAM");
}

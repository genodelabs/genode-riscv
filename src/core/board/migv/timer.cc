/*
 * \brief  SBI-timer driver for MiG-V
 * \author Sebastian Sumpf
 * \date   2021-03-31
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Core includes */
#include <kernel/timer.h>
#include <platform.h>
#include <hw/spec/riscv/sbi.h>

using namespace Genode;
using namespace Kernel;


enum {
	US_PER_TICK  = 1000 / Board::Timer::TICKS_PER_MS
};


Board::Timer::Timer(unsigned)
{
	/* enable timer interrupt */
	enum { STIE = 0x20 };
	Hw::Riscv_cpu::Sie timer(STIE);
}


time_t Board::Timer::stime() const
{
	register time_t time asm("a0");
	asm volatile ("rdtime %0" : "=r"(time));

	return time;
}


void Timer::_start_one_shot(time_t const ticks)
{
	_device.last_time = _device.stime();
	Sbi::set_timer(_device.last_time + ticks);
}


time_t Timer::ticks_to_us(time_t const ticks) const
{
	time_t us = ticks * US_PER_TICK;
	return ticks  < US_PER_TICK ? 0 : us;
}


time_t Timer::us_to_ticks(time_t const us) const
{
	return us / US_PER_TICK;
}

time_t Timer::_max_value() const {
	return 0xffffffff; }


time_t Timer::_duration() const
{
	return _device.stime() - _device.last_time;
}


unsigned Timer::interrupt_id() const { return 5; }

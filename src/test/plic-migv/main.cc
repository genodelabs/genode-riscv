/*
 * \brief   MIG-V timer IRQ test
 * \author  Sebastian Sumpf
 * \date    2021-03-05
 *
 * Test IRQs using Timer0
 *
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>
#include <irq_session/connection.h>
#include <os/attached_mmio.h>

using namespace Genode;

namespace Genode { class Timer0; }

class Genode::Timer0 : Attached_mmio
{
	enum { MS = 32 * 1024 };

	struct Timer : Register<0x0, 32> { };
	struct Ctrl  : Register<0x4, 32>
	{
		struct Enable : Bitfield<0, 1> { };
	};
	struct Cmp   : Register<0x8, 32>{ } ;

	public:

		Timer0(Env &env, addr_t base)
		:
		  Attached_mmio(env, base, 0x1000)
		{ }

		void enable()  { write<Ctrl::Enable>(0x1); }
		void disable() { write<Ctrl::Enable>(0); }

		void program()
		{
			/* periodic timeout at 100ms */
			write<Cmp>(read<Timer>() + 100 * MS);
		}

		unsigned time_ms() { return read<Timer>() / MS; }
		unsigned next_ms() { return read<Cmp>() / MS; }
};


class Main
{
	private:

		Env                 &_env;
		Timer0               _timer       { _env, 0x409000 };
		Irq_connection       _irq         { _env, 16 };
		Signal_handler<Main> _irq_handler { _env.ep(), *this, &Main::handle_irq };
		unsigned             _irq_count   { 0 };

	public:

		Main(Env &env) : _env(env)
		{
			_irq.sigh(_irq_handler);
			_irq.ack_irq();
			_timer.program();
			_timer.enable();
		}

		void handle_irq()
		{
			Genode::log("[", ++_irq_count, "] Timer irq: now: ", _timer.time_ms(),
			            "ms next: ", _timer.next_ms(), "ms");

			if (_irq_count == 100) {
				Genode::log("Test successful");
				_timer.disable();
				return;
			}

			_irq.ack_irq();
		}
};


void Component::construct(Genode::Env &env)
{
	log("--- PLIC test --");

	static Main main(env);
}

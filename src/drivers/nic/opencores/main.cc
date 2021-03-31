/*
 * \brief   OpenCores Ethernet driver
 * \author  Sebastian Sumpf
 * \date    2021-03-05
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/attached_ram_dataspace.h>
#include <base/component.h>
#include <base/heap.h>
#include <dataspace/client.h>
#include <irq_session/connection.h>
#include <net/mac_address.h>
#include <os/attached_mmio.h>
#include <timer_session/connection.h>

#include <drivers/nic/uplink_client_base.h>

using namespace Genode;

namespace Genode {
	class Opencores;
	template<typename T> class Uplink_client;
}

class Genode::Opencores : Attached_mmio
{
	private:

		/* number of descriptors */
		enum { RX = 64, TX = 64 };

		Env           &_env;
		Mmio::Delayer &_delayer;

		Attached_ram_dataspace _dma_mem {
			_env.ram(), _env.rm(), (RX + TX) * 0x800, UNCACHED };

		//XXX: make configurable
		const unsigned phy_port { 0 };
		uint8_t m[6] { 0x2, 0x0, 0x0, 0x0, 0x0, 0x3 };
		Net::Mac_address _mac { m };

		unsigned _current_tx { 0 };
		unsigned _current_rx { 0 };

		struct Moder : Register<0x0, 32>
		{
			struct RxEn : Bitfield<0, 1> { }; /* rx enable */
			struct TxEn : Bitfield<1, 1> { }; /* tx enable */
			struct NoPre : Bitfield<2, 1> { }; /* no preamble */
			struct Bro : Bitfield<3, 1> { }; /* receive broadcast */
			struct Pro : Bitfield<5, 1> { }; /* promiscuous  mode */
			struct Ifg : Bitfield<6, 1> { }; /* Inter frame gap */
			struct Exdfren : Bitfield<9, 1> { }; /* Excess defer enabled */
			struct Fulld : Bitfield<10, 1> { }; /* Full/Half duplex */
		};

		struct Int_source : Register<0x4, 32>
		{
			struct Rxb : Bitfield<2, 1> { };
		};

		struct Int_mask : Register<0x8, 32>
		{
			struct Txb : Bitfield<0, 1> { };
			struct Rxb : Bitfield<2, 1> { }; /*  receive frame */
		};

		/* packet gap registers */
		struct Ipgt  : Register<0xc,  32> { };

		struct Tx_bd : Register<0x20, 32>
		{
			/*
			 * Number of TX buffer descriptors starting at 0x400 (total descriptors
			 * 128 a 8 byte), (0x7ff - Num * 8) / 8 = number of RX descriptors
			 */
			struct Num : Bitfield<0, 8> { };
		};

		struct Miimoder : Register<0x28, 32>
		{
			struct Miinopre : Bitfield<8, 1> { }; /* nor preamble */
			struct Clkdiv : Bitfield<0, 8> { };
		};

		struct Miicommand : Register<0x2c, 32>
		{
			struct Rstat    : Bitfield<1, 1> { };
			struct Wtrldata : Bitfield<2, 1> { };
		};

		struct Miiaddress : Register<0x30, 32>
		{
			struct Fiad : Bitfield<0, 5> { }; /* Phy address */
			struct Rgad : Bitfield<8, 5> { }; /* register address */
		};

		struct Miitx_data : Register<0x34, 32>
		{
			struct Ctrldata : Bitfield<0, 16> { }; /* control data to PHY */
		};

		struct Miirx_data : Register<0x38, 32>
		{
			struct Prsd : Bitfield<0, 16> { }; /* received data from PHY */
		};

		struct Miistatus : Register<0x3c, 32>
		{
			struct Busy : Bitfield<1, 1> { };
		};

		/* reverse order 0 = byte 5, 6 = byte 0 */
		struct Mac_addr : Register_array<0x40, 32, 6, 8> { };

		struct PacketL : Register<0x18, 32> { };

		void _configure_mac_address()
		{
			log("using MAC: ", _mac);
			for (unsigned i = 0; i < 6; i++) {
				write<Mac_addr>(_mac.addr[i], i);
			}
		}

		void _enable()
		{
			write<Moder::TxEn>(1);
			write<Moder::RxEn>(1);
		}


		/************************
		 ** Buffer descriptors **
		 ************************/

		struct Tx_descriptor : Register_array<0x400, 64, TX, 64>
		{
			struct Pad   : Bitfield<12, 1>  { }; /* PAD short packets */
			struct Wr    : Bitfield<13, 1>  { }; /* wrap */
			struct Irq   : Bitfield<14, 1>  { }; /* Raise IRQ */
			struct Rd    : Bitfield<15, 1>  { }; /* Descriptor is ready */
			struct Len   : Bitfield<16, 16> { }; /* length */
			struct Clear : Bitfield<0, 16>  { };
			struct Txpnt : Bitfield<32, 32> { }; /* buffer pointer */
		};

		struct Rx_descriptor : Register_array<0x400 + TX * 8, 64, RX, 64>
		{
			struct Wr    : Bitfield<13, 1>  { }; /* wrap */
			struct Irq   : Bitfield<14, 1>  { }; /* Raise IRQ */
			struct E     : Bitfield<15, 1>  { }; /* Empty (0 = data, 1 = empty) */
			struct Len   : Bitfield<16, 16> { }; /* length */
			struct Rxpnt : Bitfield<32, 32> { }; /* buffer pointer */
		};

		unsigned _tx_index() const { return _current_tx; }
		unsigned _rx_index() const { return _current_rx; }
		unsigned _tx_next()  const { return (_tx_index() + 1) % TX; }
		unsigned _rx_next()  const { return (_rx_index() + 1) % RX; }

		void *_transmit_buffer(unsigned index) const
		{
			addr_t begin = (addr_t)_dma_mem.local_addr<addr_t>();
			return (void *)(begin + index * 0x800);
		}

		void *_receive_buffer(unsigned index) const
		{
			addr_t begin = (addr_t)_dma_mem.local_addr<addr_t>() + TX * 0x800;
			return (void *)(begin + index * 0x800);
		}

		void _setup_transmit_buffer(uint32_t address, unsigned index)
		{
			write<Tx_descriptor>(0, index);
			write<Tx_descriptor::Txpnt>(address, index);
		}

		void _setup_receive_buffer(uint32_t address, unsigned index)
		{
			write<Rx_descriptor>(0, index);
			Rx_descriptor::access_t descr = read<Rx_descriptor>(index);
			Rx_descriptor::Irq::set(descr, 1);
			Rx_descriptor::Rxpnt::set(descr, address);
			write<Rx_descriptor>(descr, index);
			write<Rx_descriptor::E>(1, index);
		}


		/******************
		 ** PHY handling **
		 ******************/

		enum Phy_address {
			BMCR = 0x0,  /* basic mode control */
			BMSR = 0x1,  /* basic mode status  */
			MICR = 0x11  /* MII IRQ control    */
		};

		void _phy_write_transaction()
		{
			wait_for(_delayer, Miistatus::Busy::Equal(0));
			write<Miicommand::Wtrldata>(1);
			wait_for(_delayer, Miistatus::Busy::Equal(0));
			write<Miicommand::Wtrldata>(0);
		}

		void _phy_read_transaction()
		{
			wait_for(_delayer, Miistatus::Busy::Equal(0));
			write<Miicommand::Rstat>(1);
			wait_for(_delayer, Miistatus::Busy::Equal(0));
			write<Miicommand::Rstat>(0);
		}

		void _phy_reset()
		{
			enum { PHY_RESET = 0x8000 };

			write<Miiaddress::Rgad>(BMCR);          /* basic mode control (BCMR) */
			write<Miitx_data::Ctrldata>(PHY_RESET); /* BCMR phy reset */
			_phy_write_transaction();
			unsigned retry = 0;
			write<Miimoder::Miinopre>(0);
			_phy_read_transaction();
			write<Miimoder::Miinopre>(1);
			while (read<Miirx_data::Prsd>() & PHY_RESET && retry++ < 20)
			{
				_phy_read_transaction();
			}

			if (retry >= 20) {
				Genode::error("PHY reset failed");
				throw -1;
			}
			log("phy reset done");
		}

		void _phy_init()
		{
			enum {
				BMCR_DUPLEX_MODE      = 0x100,
				BMCR_SPEED_SELECTION  = 0x2000,
				BMSR_LINK_STATUS      = 0x4,
			};


			write<Miiaddress::Rgad>(MICR);

			/* assert interrupt output enable */
			Miirx_data::access_t micr = read<Miirx_data::Prsd>();
			micr |= 0x1;
			write<Miitx_data::Ctrldata>(micr);
			_phy_write_transaction();

			/* full duplex, no auto negotiation, 100 MBit/s */
			write<Miiaddress::Rgad>(BMCR);
			write<Miitx_data::Ctrldata>(BMCR_DUPLEX_MODE | BMCR_SPEED_SELECTION);

			_phy_write_transaction();

			write<Miiaddress::Rgad>(BMSR);
			unsigned retry = 0;
			do {
				_phy_read_transaction();
			} while(!(read<Miirx_data::Prsd>() & BMSR_LINK_STATUS) && retry < 200);
			if (retry >= 200) {
				Genode::error("PHY duplex/speed setup failed\n");
				throw -1;
			}
			log("Link is up: ", read<Miirx_data::Prsd>(), " (BMSR)");
		}

	public:

		Opencores(Env &env, addr_t base, Mmio::Delayer &delayer)
		:
			Attached_mmio(env, base, 0x1000),
			_env(env), _delayer(delayer)
		{
			Moder::access_t moder = 0;
			Moder::Bro::set(moder, 1);
			/* promiscuous needs to be set in order to receive broadcasts */
			Moder::Pro::set(moder, 1);
			Moder::Ifg::set(moder, 1);
			Moder::Exdfren::set(moder, 1);
			Moder::Fulld::set(moder, 1);
			Moder::NoPre::set(moder, 1);
			write<Moder>(moder);

			/* enable RX interrupts */
			write<Int_mask>(0);
			write<Int_mask::Rxb>(1);
//			write<Int_mask::Txb>(1);

			/* set packet gaps to recommented values (eth_speci.pdf) */
			write<Ipgt>(0x15);

			//XXX: PHY=1 for Qemu, PHY=0 for MiG-V
			write<Miiaddress::Fiad>(phy_port);
			_configure_mac_address();

			unsigned const div = 10;
			write<Miimoder::Clkdiv>(div);
			log("MII CLK DIV set to: ", div);
			_phy_reset();
			_phy_init();

			/* number of TX descriptors */
			write<Tx_bd::Num>(TX);
			/* set wrap bit for last descriptors */
			write<Tx_descriptor::Wr>(1, TX - 1);
			write<Rx_descriptor::Wr>(1, RX - 1);

			/* fill receive buffer descriptors */
			addr_t phys = Dataspace_client(_dma_mem.cap()).phys_addr();
			for (unsigned index = 0; index < TX; index++) {
				//log("tbuf[", index, "]: ", Hex(phys + 0x800 * index),
				//    " virt: ", _transmit_buffer(index));

				_setup_transmit_buffer(phys + 0x800 * index, index);
			}

			phys += TX * 0x800;
			for (unsigned index = 0; index < RX; index++) {
				//log("rbuf[", index, "]: ", Hex(phys + 0x800 * index),
				//    " virt: ", _receive_buffer(index));
				_setup_receive_buffer(phys + 0x800 * index, index);
			}

			log("TX/RX setup done");
			_enable();
		}

		Net::Mac_address &mac_address() { return _mac; }

		bool transmit(void const *address, uint16_t length)
		{
			Tx_descriptor::access_t descr = read<Tx_descriptor>(_tx_index());
			if (Tx_descriptor::Rd::get(descr)) return false;
			memcpy(_transmit_buffer(_tx_index()), address, length);

			Tx_descriptor::Clear::set(descr, 0);
			Tx_descriptor::Pad::set(descr, 1);
			Tx_descriptor::Len::set(descr, length);
			Tx_descriptor::Rd::set(descr, 1);
			Tx_descriptor::Irq::set(descr, 1 );

			log("[", _tx_index(), "] transmit desc: ", Hex(descr));

			write<Tx_descriptor>(descr, _tx_index());
			_current_tx = _tx_next();

			return true;
		}

		bool receive_ready() const
		{
			return read<Rx_descriptor::E>(_rx_index()) == 0;
		}

		void log_irq()
		{
			log("IRQ: ", Hex(read<Int_source>()));
		}

		size_t receive_length() const
		{
			return read<Rx_descriptor::Len>(_rx_index());
		}

		void receive(void *address, uint16_t length)
		{
			memcpy(address, _receive_buffer(_rx_index()), length);
			write<Rx_descriptor::E>(1, _rx_index());
			_current_rx = _rx_next();
		}

		void receive_ack()
		{
			write<Int_source::Rxb>(1);
		}
};

template<typename T>
class Genode::Uplink_client : public Signal_handler<Uplink_client<T>>,
                              public Uplink_client_base
{
	private:

		Opencores &_nic;
		T         &_obj;
		void      (T::*_ack_irq) ();

		void _handle_irq()
		{
			_nic.log_irq();
			while (_nic.receive_ready()) {

				_drv_rx_handle_pkt(
					_nic.receive_length(),
					[&] (void   *tx_pkt_base,
					     size_t  tx_pkt_size)
				{
					_nic.receive(tx_pkt_base, tx_pkt_size);
					return Write_result::WRITE_SUCCEEDED;
				});
			}

			/* ack IRQ device */
			_nic.receive_ack();
			/* ack IRQ controller */
			(_obj.*_ack_irq)();
		}

		Transmit_result
		_drv_transmit_pkt(const char *conn_rx_pkt_base,
		                  size_t conn_rx_pkt_size) override
		{
			if (_nic.transmit(conn_rx_pkt_base, conn_rx_pkt_size))
				return Transmit_result::ACCEPTED;

			return Transmit_result::RETRY;
		}

	public:

		Uplink_client(Env &env, Allocator &alloc, Opencores &nic,
		              T &obj, void (T::*ack_irq)())
		:
			Signal_handler<Uplink_client>(env.ep(), *this, &Uplink_client::_handle_irq),
			Uplink_client_base(env, alloc, nic.mac_address()),
			_nic(nic), _obj(obj), _ack_irq(ack_irq)
		{
			_drv_handle_link_state(true);
		}
};


class Main
{
	private:

		Env &_env;

		struct Timer_delayer : Mmio::Delayer, Timer::Connection
		{
			Timer_delayer(Env &env)
			:
				Timer::Connection(env) { }

			void usleep(uint64_t us) override { Timer::Connection::usleep(us); }
		} _delayer { _env };

		Irq_connection      _irq    { _env, 22 };
		Opencores           _nic    { _env, 0x600000, _delayer };
		Heap                _heap   { _env.ram(), _env.rm() };
		Uplink_client<Main> _uplink { _env, _heap, _nic, *this, &Main::ack_irq };

	public:

		Main(Env &env) : _env(env)
		{
			_irq.sigh(_uplink);
			_irq.ack_irq();
		}

		void ack_irq() { _irq.ack_irq(); }
};


void Component::construct(Genode::Env &env)
{
	log("--- OpenCores NIC driver --");

	static Main main(env);
}

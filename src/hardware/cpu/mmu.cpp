/*
 * Copyright (C) 2016  Marco Bortolin
 *
 * This file is part of IBMulator.
 *
 * IBMulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * IBMulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with IBMulator.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ibmulator.h"
#include "cpu.h"
#include "mmu.h"
#include "bus.h"

CPUMMU g_cpummu;

#define PF_NOT_PRESENT  0x00
#define PF_PROTECTION   0x01

enum PageProtection {
	PAGE_SUPER, PAGE_READ, PAGE_WRITE
};
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY    0x40

void CPUMMU::page_fault(unsigned _fault, uint32_t _linear, bool _user, bool _write)
{
	uint32_t error_code = _fault | (uint32_t(_user) << 2) | (uint32_t(_write) << 1);
	SET_CR2(_linear);
	PDEBUGF(LOG_V2, LOG_CPU, "page fault at %08X:%08X\n", _linear, REG_EIP);
	throw CPUException(CPU_PF_EXC, error_code);
}

static const unsigned g_page_protection[16] = {
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_READ,
	PAGE_READ,
	PAGE_SUPER,
	PAGE_SUPER,
	PAGE_READ,
	PAGE_WRITE
};

void CPUMMU::page_check(unsigned _protection, uint32_t _linear, bool _user, bool _write)
{
	if(_user) {
		if(_protection == PAGE_SUPER) {
			page_fault(PF_PROTECTION, _linear, true, _write);
		}
		if(_write && _protection == PAGE_READ) {
			page_fault(PF_PROTECTION, _linear, true, true);
		}
	}
}

void CPUMMU::TLB_miss(uint32_t _linear, TLBEntry *_tlbent, bool _user, bool _write)
{
	uint32_t ppf = PDBR;
	unsigned prot = 0;
	uint32_t entry[2];
	uint32_t entry_addr[2];
	static const int PDIR = 1;
	static const int PTBL = 0;

	/*
	Page Directory/Table Entry (PDE, PTE)
	31                                   12 11          6 5     2 1 0
	╔══════════════════════════════════════╤═══════╤═══╤═╤═╤═══╤═╤═╤═╗
	║                                      │       │   │ │ │   │U│R│ ║
	║      PAGE FRAME ADDRESS 31..12       │ AVAIL │0 0│D│A│0 0│/│/│P║
	║                                      │       │   │ │ │   │S│W│ ║
	╚══════════════════════════════════════╧═══════╧═══╧═╧═╧═══╧═╧═╧═╝
	P: PRESENT, R/W: READ/WRITE, U/S: USER/SUPERVISOR, D: DIRTY
	AVAIL: AVAILABLE FOR SYSTEMS PROGRAMMER USE, 0: reserved

	Page Translation
	                                                              PAGE FRAME
	              ╔═══════════╦═══════════╦══════════╗         ╔═══════════════╗
	              ║    DIR    ║   PAGE    ║  OFFSET  ║         ║               ║
	              ╚═════╤═════╩═════╤═════╩═════╤════╝         ║               ║
	                    │           │           │              ║               ║
	      ┌──────/──────┘           /10         └──────/──────▶║    PHYSICAL   ║
	      │     10                  │                 12       ║    ADDRESS    ║
	      *4                        *4                         ║               ║
	      │   PAGE DIRECTORY        │      PAGE TABLE          ║               ║
	      │  ╔═══════════════╗      │   ╔═══════════════╗      ║               ║
	      │  ║               ║      │   ║               ║      ╚═══════════════╝
	      │  ║               ║      │   ╠═══════════════╣              ▲
	      │  ║               ║      └──▶║      PTE      ╟──────────────┘
	      │  ╠═══════════════╣          ╠═══════════════╣   [31:12]
	      └─▶║      PDE      ╟──┐       ║               ║
	         ╠═══════════════╣  │       ║               ║
	         ║               ║  │       ║               ║
	         ╚═══════════════╝  │       ╚═══════════════╝
	                 ▲          │               ▲
	╔═══════╗ PDBR   │          └───────────────┘
	║  CR3  ╟────────┘               [31:12]
	╚═══════╝ [31:12]
	 */

	// Read tables from memory
	for(int table = PDIR; table>=PTBL; --table) {
		entry_addr[table] = ppf + ((_linear >> (10 + 10*table)) & 0xFFC);
		entry[table] = g_cpubus.mem_read<4>(entry_addr[table]);
		if(!(entry[table] & 0x1)) {
			page_fault(PF_NOT_PRESENT, _linear, _user, _write);
		}
		prot |= ((entry[table] & 0x6) >> 1) << table*2;
		ppf = entry[table] & 0xFFFFF000;
	}

	// Update TLB
	_tlbent->lpf = LPF_OF(_linear);
	_tlbent->ppf = ppf;
	_tlbent->protection = g_page_protection[prot];

	// Update PDE A bit
	if(!(entry[PDIR] & PAGE_ACCESSED)) {
		entry[PDIR] |= PAGE_ACCESSED;
		g_cpubus.mem_write<4>(entry_addr[PDIR], entry[PDIR]);
	}
	// Update PTE A and D bits
	if(!(entry[PTBL] & PAGE_ACCESSED) || (_write && !(entry[PTBL] & PAGE_DIRTY))) {
		entry[PTBL] |= (PAGE_ACCESSED | (_write << 6));
		g_cpubus.mem_write<4>(entry_addr[PTBL], entry[PTBL]);
	}
}

uint32_t CPUMMU::TLB_lookup(uint32_t _linear, unsigned _len, bool _user, bool _write)
{
	TLBEntry *tlbent = &m_TLB[TLB_index(_linear, _len-1)];
	if(tlbent->lpf != LPF_OF(_linear)) {
		TLB_miss(_linear, tlbent, _user, _write);
	}
	page_check(tlbent->protection, _linear, _user, _write);
	uint32_t phy = tlbent->ppf | PAGE_OFFSET(_linear);

	return phy;
}

void CPUMMU::TLB_check(uint32_t _linear, bool _user, bool _write)
{
	// do a byte lookup discarding the result
	TLB_lookup(_linear, 1, _user, _write);
}

void CPUMMU::TLB_flush()
{
	for(unsigned n=0; n<TLB_SIZE; n++) {
		m_TLB[n].lpf = -1;
	}
}

uint32_t CPUMMU::translate_linear(uint32_t _linear_addr, Memory *_memory) const
{
	uint32_t ppf = PDBR;
	for(int table = 1; table>=0; --table) {
		uint32_t entry_addr = ppf + ((_linear_addr >> (10 + 10*table)) & 0xffc);
		uint32_t entry = _memory->read_notraps<4>(entry_addr);
		if(!(entry & 0x1)) {
			throw CPUException(CPU_PF_EXC, PF_NOT_PRESENT);
		}
		ppf = entry & 0xfffff000;
	}
	return ppf | (_linear_addr & 0xfff);
}
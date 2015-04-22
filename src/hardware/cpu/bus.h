/*
 * 	Copyright (c) 2015  Marco Bortolin
 *
 *	This file is part of IBMulator
 *
 *  IBMulator is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	IBMulator is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with IBMulator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IBMULATOR_CPU_BUS_H
#define IBMULATOR_CPU_BUS_H

#include "core.h"
#include "../memory.h"
#ifndef NDEBUG
	#include "machine.h"
#endif

#define CPU_PQ_SIZE          6
#define DRAM_ACCESS_CYCLES   2
#define DRAM_WAIT_STATES     1
#define DRAM_TX_CYCLES (DRAM_ACCESS_CYCLES+DRAM_WAIT_STATES)
#define DRAM_REFRESH_CYCLES  5
/*TODO
 * The following is an oversimplification of the video memory access time.
 * Also, the video ram wait states depend on the CPU frequency.
 * For an in depth explanation of the Display Adapter Cycle-Eater read Michael
 * Abrash's Graphics Programming Black Book, Ch.4
 */
#define VRAM_WAIT_STATES     20
#define VRAM_TX_CYCLES (DRAM_ACCESS_CYCLES+VRAM_WAIT_STATES)


class CPUBus;
extern CPUBus g_cpubus;


class CPUBus
{
private:
	struct {
		uint32_t csip;
		uint16_t ip;
		uint8_t pq[CPU_PQ_SIZE];
		bool pq_valid;
		uint32_t pq_head;
		uint32_t pq_tail;
		uint32_t pq_headpos;
	} m_s;

	uint m_dram_tx;
	uint m_vram_tx;
	uint m_pq_fetches;
	uint m_cycles_penalty;
	uint m_cycles_surplus;

	void pq_fill(uint free_space, uint toread);
	GCC_ATTRIBUTE(always_inline)
	inline uint get_pq_free_space() {
		return CPU_PQ_SIZE - (m_s.pq_tail-m_s.pq_head) + (m_s.csip - m_s.pq_head);
	}
	GCC_ATTRIBUTE(always_inline)
	inline uint get_pq_cur_index() {
		return (m_s.pq_headpos + (m_s.csip - m_s.pq_head)) % CPU_PQ_SIZE;
	}
	inline uint get_pq_cur_size() {
		return CPU_PQ_SIZE - get_pq_free_space();
	}
	inline bool is_pq_empty() {
		return (m_s.csip == m_s.pq_tail);
	}

public:

	CPUBus();

	void init();
	void config_changed();

	inline void reset_counters() {
		m_dram_tx = 0;
		m_vram_tx = 0;
		m_cycles_penalty = 0;
		m_pq_fetches = 0;
	}

	inline uint get_dram_tx() { return m_dram_tx; }
	inline uint get_vram_tx() { return m_vram_tx; }
	inline uint get_cycles_penalty() { return m_cycles_penalty; }
	inline uint get_pq_fetches() { return m_pq_fetches; }
	inline bool is_pq_valid() { return m_s.pq_valid; }
	void update_pq(uint _cycles);

	//instruction fetching
	uint8_t fetchb();
	uint16_t fetchw();

	inline uint32_t get_ip() { return m_s.ip; }
	inline uint32_t get_csip() { return m_s.csip; }

	inline void invalidate_pq() {
		m_s.pq_valid = false;
		m_s.ip = REG_IP;
		m_s.csip = GET_PHYADDR(CS, m_s.ip);
	}

	inline uint8_t mem_read_byte(uint32_t _addr) {
		//TODO video ram range is a special case; also, this check is already
		//done in Memory::read* and Memory::write*. Move into the Memory class.
		if(_addr >= 0xA0000 && _addr <= 0xBFFFF) {
			m_vram_tx += 1;
		} else {
			m_dram_tx += 1;
		}
		return g_memory.read_byte(_addr);
	}
	inline uint16_t mem_read_word(uint32_t _addr) {
		/* When the 286 is asked to perform a word-sized access
		 * starting at an odd address, it actually performs two separate
		 * accesses, each of which fetches 1 byte, just as the 8088 does for
		 * all word-sized accesses.
		 */
		if(_addr >= 0xA0000 && _addr <= 0xBFFFF) {
			m_vram_tx += 1 + (_addr & 1);
		} else {
			m_dram_tx += 1 + (_addr & 1);
		}
		return g_memory.read_word(_addr);
	}
	inline uint32_t mem_read_dword(uint32_t _addr) {
		uint32_t w0 = mem_read_word(_addr);
		uint32_t w1 = mem_read_word(_addr+2);
		return w1<<16 | w0;
	}
	inline uint64_t mem_read_qword(uint32_t _addr) {
		uint64_t w0 = mem_read_word(_addr);
		uint64_t w1 = mem_read_word(_addr+2);
		uint64_t w2 = mem_read_word(_addr+4);
		uint64_t w3 = mem_read_word(_addr+6);
		return w3<<48 | w2<<32 | w1<<16 | w0;

	}

	inline void mem_write_byte(uint32_t _addr, uint8_t _data) {
		if(_addr >= 0xA0000 && _addr <= 0xBFFFF) {
			m_vram_tx += 1;
		} else {
			m_dram_tx += 1;
		}
		g_memory.write_byte(_addr, _data);
	}
	inline void mem_write_word(uint32_t _addr, uint16_t _data) {
		if(_addr >= 0xA0000 && _addr <= 0xBFFFF) {
			m_vram_tx += 1 + (_addr & 1);
		} else {
			m_dram_tx += 1 + (_addr & 1);
		}
		g_memory.write_word(_addr, _data);
	}

	void save_state(StateBuf &_state);
	void restore_state(StateBuf &_state);

	void write_pq_to_logfile(FILE *_dest);
};

#endif

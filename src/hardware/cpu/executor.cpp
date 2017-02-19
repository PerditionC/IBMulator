/*
 * Copyright (C) 2015, 2016  Marco Bortolin
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
#include "machine.h"
#include "cpu.h"
#include "core.h"
#include "executor.h"
#include "bus.h"
#include "mmu.h"
#include "hardware/devices.h"
#include "debugger.h"
#include <cstring>
#include <cmath>

CPUExecutor g_cpuexecutor;

/* the parity flag (PF) indicates whether the modulo 2 sum of the low-order
 * eight bits of the operation is even (PF=O) or odd (PF= 1) parity.
 */
#define PARITY(x) (!(popcnt(x & 0xFF) & 1))

#ifdef __MSC_VER
#  include <intrin.h>
#  define __builtin_popcount __popcnt
#endif

GCC_ATTRIBUTE(always_inline)
inline uint popcnt(uint _value)
{
#if 0
	uint count;
    __asm__ ("popcnt %1,%0" : "=r"(count) : "rm"(_value) : "cc");
    return count;
#else
    // the builtin is translated with the POPCNT instruction only with -mpopcnt
    // or -msse4.2 flags
    return __builtin_popcount(_value);
#endif
}

#include "executor/access.cpp"
#include "executor/ctrlxfer.cpp"
#include "executor/flags.cpp"
#include "executor/interrupts.cpp"
#include "executor/memory.cpp"
#include "executor/modrm.cpp"
#include "executor/stack.cpp"
#include "executor/tasks.cpp"
#include "executor/opcodes.cpp"


CPUExecutor::CPUExecutor()
:
m_dos_prg_int_exit(0)
{
	//register_INT_trap(0x00, 0xFF, &CPUExecutor::INT_debug);
	register_INT_trap(0x13, 0x13, &CPUExecutor::INT_debug);
	register_INT_trap(0x21, 0x21, &CPUExecutor::INT_debug);
}

void CPUExecutor::reset(uint _signal)
{
	m_instr = nullptr;
	m_base_ds = REGI_DS;
	m_base_ss = REGI_SS;

	if(_signal == MACHINE_HARD_RESET || _signal == MACHINE_POWER_ON) {
		m_inttraps_ret.clear();
		while(!m_dos_prg.empty()) {
			m_dos_prg.pop();
		}
	}
}

void CPUExecutor::config_changed()
{
	if(CPU_FAMILY <= CPU_286) {
		m_max_instr_size = 10;
	} else {
		m_max_instr_size = 15;
	}
}

void CPUExecutor::execute(Instruction * _instr)
{
	m_instr = _instr;

	uint32_t old_eip = REG_EIP;

	SET_EIP(REG_EIP + m_instr->size);

	if(INT_TRAPS) {
		auto ret = m_inttraps_ret.find(m_instr->cseip);
		if(ret != m_inttraps_ret.end()) {
			for(auto fn : ret->second) {
				fn();
			}
			m_inttraps_ret.erase(ret);
		}
	}

	if(CPULOG && m_dos_prg_int_exit) {
		if(m_instr->cseip == m_dos_prg_int_exit) {
			//logging starts at the next instruction
			g_machine.DOS_program_start(m_dos_prg.top().second);
		}
	}

	if(!m_instr->valid) {
		illegal_opcode();
	}

	static CPUExecutor_fun exec_fn;

	if(!m_instr->rep || m_instr->rep_first) {
		if(m_instr->size > m_max_instr_size) {
			/*
			 * When the CPU detects an instruction that is illegal due to being
			 * greater than 10 bytes in length, it generates an exception
			 * #13 (General Protection Violation)
			 * [80286 ARPL and Overlength Instructions, 15 October 1984]
			 */
			throw CPUException(CPU_GP_EXC, 0);
		}

		if(old_eip + m_instr->size > GET_LIMIT(CS)) {
			PERRF(LOG_CPU, "CS limit violation!\n");
			throw CPUException(CPU_GP_EXC, 0);
		}

		if(m_instr->seg != REGI_NONE) {
			m_base_ds = m_instr->seg;
			m_base_ss = m_instr->seg;
		} else {
			m_base_ds = REGI_DS;
			m_base_ss = REGI_SS;
		}

		exec_fn = m_instr->fn;

		if(m_instr->addr32) {
			EA_get_segreg = &CPUExecutor::EA_get_segreg_32;
			EA_get_offset = &CPUExecutor::EA_get_offset_32;
			m_addr_mask = 0xFFFFFFFF;
			if(m_instr->rep) {
				exec_fn = &CPUExecutor::rep_32;
			}
		} else {
			EA_get_segreg = &CPUExecutor::EA_get_segreg_16;
			EA_get_offset = &CPUExecutor::EA_get_offset_16;
			m_addr_mask = 0xFFFF;
			if(m_instr->rep) {
				exec_fn = &CPUExecutor::rep_16;
			}
		}
	}

	(this->*exec_fn)();
}

void CPUExecutor::rep_16()
{
	/* 1. Check the CX register. If it is zero, exit the iteration and move
	 * to the next instruction.
	 */
	if(REG_CX == 0) {
		//REP finished and IP points to the next instr.
		return;
	}
	/* 2. Acknowledge any pending interrupts.
	 * this is done in the CPU::step()
	 * TODO so it checks (E)CX after interrupts; is it really a relevant difference?
	 */
	try {
		/* 3. Perform the string operation once.
		 */
		(this->*(m_instr->fn))();
	} catch(CPUException &e) {
		//TODO an exception occurred during the instr execution. what should i do?
		RESTORE_EIP();
		throw;
	}

	/* 4. Decrement CX by 1; no flags are modified. */
	REG_CX -= 1;

	/* 5. If the string operation is SCAS or CMPS, check the zero flag.
	 * If the repeat condition does not hold, then exit the iteration and
	 * move to the next instruction. Exit if the prefix is REPE and ZF=O
	 * (the last comparison was not equal), or if the prefix is REPNE and
	 * ZF=1 (the last comparison was equal).
	 */
	if(m_instr->rep_zf) {
		if((m_instr->rep_equal && !FLAG_ZF) || (!m_instr->rep_equal && FLAG_ZF))
		{
			//REP finished and IP points to the next instr.
			return;
		}
	}
	/* 6. Go to step 1 for the next iteration. */
	//REP not finished so back up
	RESTORE_EIP();

	m_instr->rep_first = false;
}

void CPUExecutor::rep_32()
{
	if(REG_ECX == 0) {
		return;
	}

	try {
		(this->*(m_instr->fn))();
	} catch(CPUException &e) {
		//TODO an exception occurred during the instr execution. what should i do?
		RESTORE_EIP();
		throw;
	}

	REG_ECX -= 1;

	if(m_instr->rep_zf) {
		if((m_instr->rep_equal && !FLAG_ZF) || (!m_instr->rep_equal && FLAG_ZF)) {
			return;
		}
	}

	RESTORE_EIP();

	m_instr->rep_first = false;
}

void CPUExecutor::illegal_opcode()
{
	char buf[CPU_MAX_INSTR_SIZE * 2 + 1];
	char * writecode = buf;
	uint i=0;
	while(i<(m_instr->size) && i<CPU_MAX_INSTR_SIZE) {
		sprintf(writecode, "%02X", m_instr->bytes[i++]);
		writecode += 2;
	}
	PDEBUGF(LOG_V2, LOG_CPU, "Illegal opcode: %s\n", buf);
	throw CPUException(CPU_UD_EXC, 0);
}

uint64_t CPUExecutor::fetch_descriptor(Selector & _selector, uint8_t _exc_vec)
{
	uint32_t linaddr = _selector.index * 8;
	if(_selector.ti == 0) {
		//from GDT
		if((_selector.index*8 + 7u) > SEG_REG(REGI_GDTR).desc.limit) {
			PDEBUGF(LOG_V2, LOG_CPU,"fetch_descriptor: GDT: index (%x) %x > limit (%x)\n",
					_selector.index*8 + 7, _selector.index, SEG_REG(REGI_GDTR).desc.limit);
			throw CPUException(_exc_vec, _selector.value & SELECTOR_RPL_MASK);
		}
		linaddr += SEG_REG(REGI_GDTR).desc.base;
	} else {
		// from LDT
		if(!SEG_REG(REGI_LDTR).desc.valid) {
			PDEBUGF(LOG_V2, LOG_CPU, "fetch_descriptor: LDTR not valid\n");
			throw CPUException(_exc_vec, _selector.value & SELECTOR_RPL_MASK);
		}
		if((_selector.index*8 + 7u) > SEG_REG(REGI_LDTR).desc.limit) {
			PDEBUGF(LOG_V2, LOG_CPU,"fetch_descriptor: LDT: index (%x) %x > limit (%x)\n",
					_selector.index*8 + 7, _selector.index, SEG_REG(REGI_LDTR).desc.limit);
			throw CPUException(_exc_vec, _selector.value & SELECTOR_RPL_MASK);
		}
		linaddr += SEG_REG(REGI_LDTR).desc.base;
	}
	return read_qword(linaddr);
}

void CPUExecutor::touch_segment(Selector &_selector, Descriptor &_descriptor)
{
	/*
	 * Whenever a segment descriptor is loaded into a segment register, the
	 * accessed bit in the descriptor table is set to 1. This bit is useful for
	 * determining the usage profile of the segment.
	 * (cfr. 7-11)
	 */
	if(!_descriptor.accessed) {
		_descriptor.accessed = true;
		uint8_t ar = _descriptor.get_AR();
		unsigned reg;
		if(_selector.ti == false) {
			// from GDT
			reg = REGI_GDTR;
		} else {
			// from LDT
			reg = REGI_LDTR;
		}
		write_byte(SEG_REG(reg).desc.base + _selector.index*8 + 5, ar);
	}
}

void CPUExecutor::register_INT_trap(uint8_t _lo_vec, uint8_t _hi_vec, inttrap_fun_t _fn)
{
	m_inttraps_intervals.push_back(inttrap_interval_t(_lo_vec, _hi_vec, _fn));
	m_inttraps_tree = inttrap_intervalTree_t(m_inttraps_intervals);
}

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

#include "ibmulator.h"
#include "core.h"
#include "hardware/cpu.h"
#include "hardware/memory.h"
#include <cstring>

CPUCore g_cpucore;


void CPUCore::reset()
{
	/* The RESET signal initializes the CPU in Real-Address mode, with the CS
	 * base register containing FFOOOOH and IP containing FFFOH. The first
	 * instruction fetch cycle following reset will be from the physical address
	 * formed by these two registers, i.e., from address FFFFFOH. This location
	 * will normally contain a JMP instruction to the actual beginning of the
	 * system bootstrap program.
	 */

	memset(m_genregs, 0, sizeof(GenReg)*8);

	m_f = 0x0002;
	m_msw = 0xFFF0;

	m_ip = 0xFFF0;

	load_segment_real(REG_CS, 0xF000, true);
	REG_CS.desc.base = 0xFF0000;
	load_segment_real(REG_DS, 0x0000, true);
	load_segment_real(REG_SS, 0x0000, true);
	load_segment_real(REG_ES, 0x0000, true);

	load_segment_real(m_ldtr, 0x0000, true);
	load_segment_real(m_tr, 0x0000, true);

	m_idtr_base = 0x000000;
	m_idtr_limit = 0x03FF;

	m_gdtr_base = 0x000000;
	m_gdtr_limit = 0x0000;
}

#define CPUCORE_STATE_NAME "CPUCore"

void CPUCore::save_state(StateBuf &_state)
{
	static_assert(std::is_pod<CPUCore>::value, "CPUCore must be POD");
	StateHeader h;
	h.name = CPUCORE_STATE_NAME;
	h.data_size = sizeof(CPUCore);
	_state.write(this,h);
}

void CPUCore::restore_state(StateBuf &_state)
{
	StateHeader h;
	h.name = CPUCORE_STATE_NAME;
	h.data_size = sizeof(CPUCore);
	_state.read(this,h);
}

void CPUCore::load_segment_real(SegReg & _segreg, uint16_t _value, bool _defaults)
{
	/* According to Intel, each time any segment register is loaded in real mode,
	 * the base address is calculated as 16 times the segment value, while the
	 * access rights and size limit attributes are given fixed, "real-mode
	 * compatible" values. This is not true. In fact, only the CS descriptor caches
	 * for the 286, 386, and 486 get loaded with fixed values each time the segment
	 * register is loaded.
	 * (http://www.rcollins.org/ddj/Aug98/Aug98.html)
	 */

	_segreg.sel.value = _value;
	_segreg.sel.cpl = 0; // in real mode the current privilege level is always 0
	_segreg.desc.base = uint32_t(_value) << 4;
	if(_defaults) {
		_segreg.desc.limit = 0xFFFF;
		_segreg.desc.set_AR(
			SEG_ACCESSED |
			SEG_READWRITE |
			SEG_EXECUTABLE |
			SEG_SEGMENT |
			SEG_PRESENT
		);
	}
}

void CPUCore::load_segment_protected(SegReg & _segreg, uint16_t _value)
{
	/*chapters 6,7*/

	Selector selector;
	Descriptor descriptor;

	if(_segreg.is(REG_SS)) {

		if ((_value & SELECTOR_RPL_MASK) == 0) {
			PDEBUGF(LOG_V2, LOG_CPU, "load_segment_protected(SS): null selector\n");
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}

		selector.set(_value);

		/* selector's RPL must be equal to CPL, else #GP(selector) */
		if(selector.rpl != CPL) {
			PDEBUGF(LOG_V2, LOG_CPU, "load_segment_protected(SS): rpl != CPL\n");
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}

		uint64_t descdata = fetch_descriptor(selector, CPU_GP_EXC);
		descriptor.set(descdata);
		if(!descriptor.valid) {
			PDEBUGF(LOG_V2, LOG_CPU,"load_segment_protected(SS): not valid\n");
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}
		/* AR byte must indicate a writable data segment else #GP(selector) */
		if(  descriptor.segment == false ||
		    (descriptor.is_code_segment()) ||
		   !(descriptor.is_data_segment_writeable())
		) {
			PDEBUGF(LOG_V2, LOG_CPU, "load_segment_protected(SS): not writable data segment\n");
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}
		/* DPL in the AR byte must equal CPL else #GP(selector) */
		if(descriptor.dpl != CPL) {
			PDEBUGF(LOG_V2, LOG_CPU,"load_segment_protected(SS): dpl != CPL\n");
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}
		/* segment must be marked PRESENT else #SS(selector) */
		if(!descriptor.present) {
			PDEBUGF(LOG_V2, LOG_CPU,"load_segment_protected(SS): not present\n");
			throw CPUException(CPU_SS_EXC, _value & SELECTOR_RPL_MASK);
		}

		/* set accessed bit */
		touch_segment(selector, descriptor);

		/* all done and well, load the register */
		REG_SS.desc = descriptor;
		REG_SS.sel = selector;

	} else if(_segreg.is(REG_DS) || _segreg.is(REG_ES)) {

		if((_value & SELECTOR_RPL_MASK) == 0) {
			/* null selector */
			_segreg.sel.set(_value);
			_segreg.desc.set(0);
			_segreg.desc.set_AR(SEG_SEGMENT); /* data/code segment */
			_segreg.desc.valid = false; /* invalidate null selector */
			return;
		}

		selector.set(_value);
		uint64_t descdata = fetch_descriptor(selector, CPU_GP_EXC);
		descriptor.set(descdata);

		if(descriptor.valid==0) {
			PDEBUGF(LOG_V2, LOG_CPU,"load_segment_protected(%s, 0x%04x): invalid segment\n",
					_segreg.to_string(), _value);
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}

		/* AR byte must indicate a writable data segment else #GP(selector) */
		if(  descriptor.segment == false ||
			(descriptor.is_code_segment() && !(descriptor.is_code_segment_readable())
		  )
		) {
			PDEBUGF(LOG_V2, LOG_CPU, "load_segment_protected(%s, 0x%04x): not data or readable code (AR=0x%02X)\n",
					_segreg.to_string(), _value, descriptor.ar);
			throw CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
		}

		/* If data or non-conforming code, then both the RPL and the CPL
		 * must be less than or equal to DPL in AR byte else #GP(selector) */
		if(descriptor.is_data_segment() || descriptor.is_code_segment_non_conforming()) {
			if((selector.rpl > descriptor.dpl) || (CPL > descriptor.dpl)) {
				PDEBUGF(LOG_V2, LOG_CPU, "load_segment_protected(%s, 0x%04x): RPL & CPL must be <= DPL\n",
						_segreg.to_string(), _value);
				CPUException(CPU_GP_EXC, _value & SELECTOR_RPL_MASK);
			}
		}

		/* segment must be marked PRESENT else #NP(selector) */
		if(!descriptor.present) {
			PDEBUGF(LOG_V2, LOG_CPU,"load_segment_protected(%s, 0x%04x): segment not present\n",
					_segreg.to_string(), _value);
			throw CPUException(CPU_NP_EXC, _value & SELECTOR_RPL_MASK);
		}

		/* set accessed bit */
		touch_segment(selector, descriptor);

		/* all done and well, load the register */
		_segreg.desc = descriptor;
		_segreg.sel = selector;

	} else {

		PERRF_ABORT(LOG_CPU, "load_segment_protected(): invalid register!\n");

	}
}

void CPUCore::check_CS(uint16_t selector, Descriptor &descriptor, uint8_t rpl, uint8_t cpl)
{
	// descriptor AR byte must indicate code segment else #GP(selector)
	if(descriptor.valid==false || descriptor.segment==false ||
			!(descriptor.type & SEG_TYPE_EXECUTABLE))
	{
		PDEBUGF(LOG_V2, LOG_CPU,"check_CS(0x%04x): not a valid code segment\n", selector);
		throw CPUException(CPU_GP_EXC, selector & 0xfffc);
	}

	// if non-conforming, code segment descriptor DPL must = CPL else #GP(selector)
	if(!(descriptor.is_code_segment_conforming())) {
		if(descriptor.dpl != cpl) {
			PDEBUGF(LOG_V2, LOG_CPU,"check_CS(0x%04x): non-conforming code seg descriptor dpl != cpl, dpl=%d, cpl=%d\n",
					selector, descriptor.dpl, cpl);
			throw CPUException(CPU_GP_EXC, selector & 0xfffc);
		}

		/* RPL of destination selector must be <= CPL else #GP(selector) */
		if(rpl > cpl) {
			PDEBUGF(LOG_V2, LOG_CPU,"check_CS(0x%04x): non-conforming code seg selector rpl > cpl, rpl=%d, cpl=%d\n",
					selector, rpl, cpl);
			throw CPUException(CPU_GP_EXC, selector & 0xfffc);
		}
	}
	// if conforming, then code segment descriptor DPL must <= CPL else #GP(selector)
	else {
		if(descriptor.dpl > cpl) {
			PDEBUGF(LOG_V2, LOG_CPU,"check_CS(0x%04x): conforming code seg descriptor dpl > cpl, dpl=%d, cpl=%d\n",
					selector, descriptor.dpl, cpl);
			throw CPUException(CPU_GP_EXC, selector & 0xfffc);
		}
	}

	// code segment must be present else #NP(selector)
	if(!descriptor.present) {
		PDEBUGF(LOG_V2, LOG_CPU,"check_CS(0x%04x): code segment not present\n", selector);
		throw CPUException(CPU_NP_EXC, selector & 0xfffc);
	}
}


void CPUCore::set_CS(Selector &selector, Descriptor &descriptor, uint8_t cpl)
{
	// Add cpl to the selector value.
	selector.value = (selector.value & SELECTOR_RPL_MASK) | cpl;

	touch_segment(selector, descriptor);

	REG_CS.sel = selector;
	REG_CS.desc = descriptor;
	REG_CS.sel.cpl = cpl;

	//the pq must be invalidated by the caller!
}

void CPUCore::set_SS(Selector &selector, Descriptor &descriptor, uint8_t cpl)
{
	// Add cpl to the selector value.
	selector.value = (selector.value & SELECTOR_RPL_MASK) | cpl;

	if((selector.value & SELECTOR_RPL_MASK) != 0)
		touch_segment(selector, descriptor);

	REG_SS.sel = selector;
	REG_SS.desc = descriptor;
	REG_SS.sel.cpl = cpl;
}


void CPUCore::touch_segment(Selector &_selector, Descriptor &_descriptor)
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
		if(_selector.ti == false) {
			// from GDT
			g_cpubus.mem_write_byte(m_gdtr_base + _selector.index*8 + 5, ar);
		} else {
			// from LDT
			g_cpubus.mem_write_byte(m_ldtr.desc.base + _selector.index*8 + 5, ar);
		}
	}
}

uint64_t CPUCore::fetch_descriptor(Selector & _selector, uint8_t _exc_vec)
{
	uint32_t addr = _selector.index * 8;
	if(_selector.ti == 0) {
		//from GDT
		if((_selector.index*8 + 7) > m_gdtr_limit) {
			PDEBUGF(LOG_V2, LOG_CPU,"fetch_descriptor: GDT: index (%x) %x > limit (%x)\n",
					_selector.index*8 + 7, _selector.index, m_gdtr_limit);
			throw CPUException(_exc_vec, _selector.value & 0xfffc);
		}
		addr += m_gdtr_base;
	} else {
		// from LDT
		if(m_ldtr.desc.valid == false) {
			PDEBUGF(LOG_V2, LOG_CPU, "fetch_descriptor: LDTR not valid\n");
			throw CPUException(_exc_vec, _selector.value & 0xfffc);
		}
		if((_selector.index*8 + 7) > m_ldtr.desc.limit) {
			PDEBUGF(LOG_V2, LOG_CPU,"fetch_descriptor: LDT: index (%x) %x > limit (%x)\n",
					_selector.index*8 + 7, _selector.index, m_ldtr.desc.limit);
			throw CPUException(_exc_vec, _selector.value & 0xfffc);
		}
		addr += m_ldtr.desc.base;
	}
	return g_cpubus.mem_read_qword(addr);
}

void CPUCore::set_F(uint16_t _val)
{
	uint16_t f = m_f;
	m_f = _val & FMASK_VALID;
	if(m_f & FMASK_TF) {
		g_cpu.set_async_event();
	}
	if((f ^ _val) & FMASK_IF) {
		g_cpu.interrupt_mask_change();
	}
}

void CPUCore::set_TF(bool _val)
{
	if(_val) {
		m_f |= FMASK_TF;
		g_cpu.set_async_event();
	} else {
		m_f &= ~FMASK_TF;
	}
}

void CPUCore::set_IF(bool _val)
{
	set_F(FBITN_IF,_val);
	g_cpu.interrupt_mask_change();
}

const char *SegReg::to_string()
{
	if (is(REG_ES)) return("ES");
	else if (is(REG_CS)) return("CS");
	else if (is(REG_SS)) return("SS");
	else if (is(REG_DS)) return("DS");
	else { return "??"; }
}

void SegReg::validate()
{
	if(desc.dpl < CPL) {
		// invalidate if data or non-conforming code segment
		if(desc.valid==0 || desc.segment==0 ||
        desc.is_data_segment() || desc.is_code_segment_non_conforming())
		{
			sel.value = 0;
			desc.valid = false;
		}
	}
}
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

void CPUExecutor::mem_access(uint32_t _linear, unsigned _len, bool _user, bool _write)
{
	if(IS_PAGING()) {
		if((PAGE_OFFSET(_linear) + _len) <= 4096) {
			m_cached_phy.phy1 = TLB_lookup(_linear, _len, _user, _write);
			m_cached_phy.len1 = _len;
			m_cached_phy.pages = 1;
		} else {
			uint32_t page_offset = PAGE_OFFSET(_linear);
			m_cached_phy.len1 = 4096 - page_offset;
			m_cached_phy.len2 = _len - m_cached_phy.len1;
			m_cached_phy.phy1 = TLB_lookup(_linear, m_cached_phy.len1, _user, _write);
			m_cached_phy.phy2 = TLB_lookup(_linear + m_cached_phy.len1, m_cached_phy.len2, _user, _write);
			m_cached_phy.pages = 2;
		}
	} else {
		m_cached_phy.phy1 = _linear;
		m_cached_phy.len1 = _len;
		m_cached_phy.pages = 1;
	}
}


/*******************************************************************************
 * Reads
 */

uint32_t CPUExecutor::read_xpages()
{
	unsigned len = m_cached_phy.len1 + m_cached_phy.len2;
	uint32_t b, data = 0;
	for(b=0; b<m_cached_phy.len1; b++) {
		data |= g_cpubus.mem_read<1>(m_cached_phy.phy1 + b, len) << (b*8);
	}
	for(; b<len; b++) {
		data |= g_cpubus.mem_read<1>(m_cached_phy.phy2 + (b-m_cached_phy.len1), len) << (b*8);
	}
	if(MEMORY_TRAPS) {
		g_memory.check_trap(m_cached_phy.phy1, false, data, len);
	}
	return data;
}

uint8_t CPUExecutor::read_byte()
{
	return g_cpubus.mem_read<1>(m_cached_phy.phy1);
}

uint16_t CPUExecutor::read_word()
{
	if(!IS_PAGING() || m_cached_phy.pages == 1) {
		return g_cpubus.mem_read<2>(m_cached_phy.phy1);
	} else {
		return read_xpages();
	}
}

uint32_t CPUExecutor::read_dword()
{
	if(!IS_PAGING() || m_cached_phy.pages == 1) {
		return g_cpubus.mem_read<4>(m_cached_phy.phy1);
	} else {
		return read_xpages();
	}
}

uint8_t CPUExecutor::read_byte(SegReg &_seg, uint32_t _offset, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 1, false, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 1, IS_USER_PL, false);
	return read_byte();
}

uint16_t CPUExecutor::read_word(SegReg &_seg, uint32_t _offset, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 2, false, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 2, IS_USER_PL, false);
	return read_word();
}

uint32_t CPUExecutor::read_dword(SegReg &_seg, uint32_t _offset, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 4, false, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 4, IS_USER_PL, false);
	return read_dword();
}

uint8_t  CPUExecutor::read_byte(uint32_t _linear)
{
	mem_access(_linear, 1, false, false);
	return read_byte();
}

uint16_t CPUExecutor::read_word(uint32_t _linear)
{
	mem_access(_linear, 2, false, false);
	return read_word();
}

uint32_t CPUExecutor::read_dword(uint32_t _linear)
{
	mem_access(_linear, 4, false, false);
	return read_dword();
}


/*******************************************************************************
 * Writes
 */

void CPUExecutor::write_xpages(uint32_t _data)
{
	unsigned len = m_cached_phy.len1 + m_cached_phy.len2;
	unsigned b;
	for(b=0; b<m_cached_phy.len1; b++) {
		g_cpubus.mem_write<1>(m_cached_phy.phy1 + b, len, (_data>>(b*8)) & 0xFF);
	}
	for(; b<len; b++) {
		g_cpubus.mem_write<1>(m_cached_phy.phy2 + (b-m_cached_phy.len1), len, (_data>>(b*8)) & 0xFF);
	}
	if(MEMORY_TRAPS) {
		g_memory.check_trap(m_cached_phy.phy1, true, _data, len);
	}
}

void CPUExecutor::write_byte(uint8_t _data)
{
	g_cpubus.mem_write<1>(m_cached_phy.phy1, _data);
}

void CPUExecutor::write_word(uint16_t _data)
{
	if(!IS_PAGING() || m_cached_phy.pages == 1) {
		g_cpubus.mem_write<2>(m_cached_phy.phy1, _data);
	} else {
		write_xpages(_data);
	}
}

void CPUExecutor::write_dword(uint32_t _data)
{
	if(!IS_PAGING() || m_cached_phy.pages == 1) {
		g_cpubus.mem_write<4>(m_cached_phy.phy1, _data);
	} else {
		write_xpages(_data);
	}
}

void CPUExecutor::write_byte(SegReg &_seg, uint32_t _offset, uint8_t _data, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 1, true, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 1, IS_USER_PL, true);
	write_byte(_data);
}

void CPUExecutor::write_word(SegReg &_seg, uint32_t _offset, uint16_t _data, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 2, true, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 2, IS_USER_PL, true);
	write_word(_data);
}

void CPUExecutor::write_dword(SegReg &_seg, uint32_t _offset, uint32_t _data, uint8_t _vector, uint16_t _errcode)
{
	seg_check(_seg, _offset, 4, true, _vector, _errcode);
	mem_access(_seg.desc.base + _offset, 4, IS_USER_PL, true);
	write_dword(_data);
}

void CPUExecutor::write_byte(uint32_t _linear, uint8_t _data)
{
	mem_access(_linear, 1, false, true);
	write_byte(_data);
}

void CPUExecutor::write_word(uint32_t _linear, uint16_t _data)
{
	mem_access(_linear, 2, false, true);
	write_word(_data);
}

void CPUExecutor::write_dword(uint32_t _linear, uint32_t _data)
{
	mem_access(_linear, 4, false, true);
	write_dword(_data);
}

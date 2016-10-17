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
#include "gui.h"
#include "program.h"
#include "hardware/cpu.h"
#include "hardware/cpu/core.h"
#include "hardware/memory.h"
#include "machine.h"
#include "hardware/cpu/debugger.h"
#include "sysdebugger386.h"
#include <cstdio>
#include <Rocket/Core.h>
#include <Rocket/Controls.h>

#include "format.h"


event_map_t SysDebugger386::ms_evt_map = {
	GUI_EVT( "cmd_switch_power", "click", SysDebugger::on_cmd_switch_power ),
	GUI_EVT( "cmd_pause",        "click", SysDebugger::on_cmd_pause ),
	GUI_EVT( "cmd_save_state",   "click", SysDebugger::on_cmd_save_state ),
	GUI_EVT( "cmd_restore_state","click", SysDebugger::on_cmd_restore_state ),
	GUI_EVT( "CPU_step",         "click", SysDebugger::on_CPU_step ),
	GUI_EVT( "CPU_skip",         "click", SysDebugger386::on_CPU_skip ),
	GUI_EVT( "CPU_bp_btn",       "click", SysDebugger::on_CPU_bp_btn ),
	GUI_EVT( "log_prg_toggle",   "click", SysDebugger::on_log_prg_toggle ),
	GUI_EVT( "log_write",        "click", SysDebugger::on_log_write ),
	GUI_EVT( "mem_dump",         "click", SysDebugger::on_mem_dump ),
	GUI_EVT( "cs_dump",          "click", SysDebugger::on_cs_dump ),
	GUI_EVT( "ds_dump",          "click", SysDebugger::on_ds_dump ),
	GUI_EVT( "ss_dump",          "click", SysDebugger::on_ss_dump ),
	GUI_EVT( "es_dump",          "click", SysDebugger::on_es_dump ),
	GUI_EVT( "fs_dump",          "click", SysDebugger386::on_fs_dump ),
	GUI_EVT( "gs_dump",          "click", SysDebugger386::on_gs_dump ),
	GUI_EVT( "idt_dump",         "click", SysDebugger::on_idt_dump ),
	GUI_EVT( "ldt_dump",         "click", SysDebugger::on_ldt_dump ),
	GUI_EVT( "gdt_dump",         "click", SysDebugger::on_gdt_dump ),
};

SysDebugger386::SysDebugger386(GUI *_gui, Machine *_machine)
: SysDebugger(_gui, "debugger386.rml", _machine)
{
	assert(m_wnd);

	m_386core.rf = get_element("RF");
	m_386core.vm = get_element("VM");
	m_386core.pe = get_element("PE");
	m_386core.ts = get_element("TS");
	m_386core.pg = get_element("PG");

	m_386core.cr2 = get_element("CR2");
	m_386core.cr3 = get_element("CR3");

	m_386core.fs = get_element("FS");
	m_386core.gs = get_element("GS");
	m_386core.fsbase = get_element("FSbase");
	m_386core.gsbase = get_element("GSbase");
	m_386core.fslimit = get_element("FSlimit");
	m_386core.gslimit = get_element("GSlimit");

	m_tools.eip_bp->SetValue(format_hex32(0));
}

SysDebugger386::~SysDebugger386()
{
}

const RC::String & SysDebugger386::disasm(uint32_t _eip, bool _analyze, uint * _size)
{
	CPUDebugger debugger;

	static RC::String str;

	str = "";

	static char empty = 0;

	//throws CPUException when #PF
	uint32_t start = GET_PHYADDR(CS, _eip);
	char dline[200];
	uint size = debugger.disasm(dline, 200, start, _eip, &g_memory, nullptr, 0, REG_CS.desc.def);
	if(_size!=nullptr) {
		*_size = size;
	}

	char *res = &empty;

	if(_analyze) {
		//analyze_instruction throws CPUException when #PF
		res = debugger.analyze_instruction(dline, &g_cpucore, &g_memory,
				debugger.last_disasm_opsize());
		if(!res || !(*res)) {
			res = &empty;
		}
	}

	dline[30] = 0;

	str.FormatString(100, "%04X:%08X &nbsp; %s &nbsp; %s", REG_CS.sel.value, _eip, dline, res);

	return str;
};

void SysDebugger386::update()
{
	SysDebugger::update();

	m_core.eax->SetInnerRML(format_hex32(REG_EAX));
	m_core.ebx->SetInnerRML(format_hex32(REG_EBX));
	m_core.ecx->SetInnerRML(format_hex32(REG_ECX));
	m_core.edx->SetInnerRML(format_hex32(REG_EDX));

	m_core.ebp->SetInnerRML(format_hex32(REG_EBP));
	m_core.esi->SetInnerRML(format_hex32(REG_ESI));
	m_core.edi->SetInnerRML(format_hex32(REG_EDI));
	m_core.esp->SetInnerRML(format_hex32(REG_ESP));

	m_core.eip->SetInnerRML(format_hex32(REG_EIP));
	m_core.eflags->SetInnerRML(format_hex32(GET_FLAGS()));
	m_386core.rf->SetInnerRML(format_bit(FLAG_RF));
	m_386core.vm->SetInnerRML(format_bit(FLAG_VM));
	m_386core.pe->SetInnerRML(format_bit(CR0_PE));
	m_386core.ts->SetInnerRML(format_bit(CR0_TS));
	m_386core.pg->SetInnerRML(format_bit(CR0_PG));

	m_386core.fs->SetInnerRML(format_hex16(REG_FS.sel.value));
	m_386core.gs->SetInnerRML(format_hex16(REG_GS.sel.value));

	m_core.csbase->SetInnerRML(format_hex32(GET_BASE(CS)));
	m_core.dsbase->SetInnerRML(format_hex32(GET_BASE(DS)));
	m_core.esbase->SetInnerRML(format_hex32(GET_BASE(ES)));
	m_core.ssbase->SetInnerRML(format_hex32(GET_BASE(SS)));
	m_core.trbase->SetInnerRML(format_hex32(GET_BASE(TR)));
	m_386core.fsbase->SetInnerRML(format_hex32(GET_BASE(FS)));
	m_386core.gsbase->SetInnerRML(format_hex32(GET_BASE(GS)));

	m_core.cslimit->SetInnerRML(format_hex24(GET_LIMIT(CS)));
	m_core.dslimit->SetInnerRML(format_hex24(GET_LIMIT(DS)));
	m_core.eslimit->SetInnerRML(format_hex24(GET_LIMIT(ES)));
	m_core.sslimit->SetInnerRML(format_hex24(GET_LIMIT(SS)));
	m_core.trlimit->SetInnerRML(format_hex24(GET_LIMIT(TR)));
	m_386core.fslimit->SetInnerRML(format_hex24(GET_LIMIT(TR)));
	m_386core.gslimit->SetInnerRML(format_hex24(GET_LIMIT(TR)));

	m_386core.cr2->SetInnerRML(format_hex32(REG_CR2));
	m_386core.cr3->SetInnerRML(format_hex32(REG_CR3));

	m_core.ldtbase->SetInnerRML(format_hex32(GET_BASE(LDTR)));
	m_core.idtbase->SetInnerRML(format_hex32(GET_BASE(IDTR)));
	m_core.gdtbase->SetInnerRML(format_hex32(GET_BASE(GDTR)));

	const uint len = 12;
	uint8_t buf[len];

	try {
		read_memory(GET_PHYADDR(CS, REG_EIP), buf, len);
		m_memory.cs_eip->SetInnerRML(format_words(buf, len));
		m_memory.cs_eip_str->SetInnerRML(format_words_string(buf, len));
	} catch(...) {
		// catch any #PF
		m_memory.cs_eip->SetInnerRML("#PF");
		m_memory.cs_eip_str->SetInnerRML("#PF");
	}

	try {
		read_memory(GET_PHYADDR(DS, REG_ESI), buf, len);
		m_memory.ds_esi->SetInnerRML(format_words(buf, len));
		m_memory.ds_esi_str->SetInnerRML(format_words_string(buf, len));
	} catch(...) {
		// catch any #PF
		m_memory.ds_esi->SetInnerRML("#PF");
		m_memory.ds_esi_str->SetInnerRML("#PF");
	}

	try {
		read_memory(GET_PHYADDR(ES, REG_EDI), buf, len);
		m_memory.es_edi->SetInnerRML(format_words(buf, len));
		m_memory.es_edi_str->SetInnerRML(format_words_string(buf, len));
	} catch(...) {
		// catch any #PF
		m_memory.es_edi->SetInnerRML("#PF");
		m_memory.es_edi_str->SetInnerRML("#PF");
	}

	try {
		read_memory(GET_PHYADDR(SS, REG_ESP), buf, len);
		m_memory.ss_esp->SetInnerRML(format_words(buf, len));
		m_memory.ss_esp_str->SetInnerRML(format_words_string(buf, len));
	} catch(...) {
		// catch any #PF
		m_memory.ss_esp->SetInnerRML("#PF");
		m_memory.ss_esp_str->SetInnerRML("#PF");
	}

	RC::String str;
	uint size;
	uint32_t nextip = REG_EIP;
	try {
		str = disasm(nextip, true, &size) + "<br />";
		nextip += size;
		str += disasm(nextip, false, &size) + "<br />";
		nextip += size;
		str += disasm(nextip, false, nullptr);
	} catch(CPUException &) {
		// catch any #PF
		str = "[invalid]";
	}
	m_disasm.line0->SetInnerRML(str);
}

void SysDebugger386::on_CPU_skip(RC::Event &)
{
	if(m_machine->is_paused()) {
		uint size;
		try {
			disasm(REG_EIP, false, &size);
			uint32_t curaddr = GET_LINADDR(CS, REG_EIP);
			m_machine->cmd_cpu_breakpoint(curaddr+size, [](){});
			m_tools.btn_bp->SetClass("on", false);
			m_machine->cmd_resume();
		} catch(CPUException &) {
			m_gui->show_dbg_message("CPU exception trying to disassemble current instruction");
		}
	}
}

void SysDebugger386::on_fs_dump(RC::Event &)
{
	m_machine->cmd_memdump(REG_FS.desc.base, REG_FS.desc.limit);
}

void SysDebugger386::on_gs_dump(RC::Event &)
{
	m_machine->cmd_memdump(REG_GS.desc.base, REG_GS.desc.limit);
}

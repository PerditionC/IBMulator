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

/*******************************************************************************
 * AAA-ASCII Adjust AL After Addition
 */

void CPUExecutor::AAA()
{
	/* according to the original Intel's 286 manual, only AF and CF are modified
	 * but it seems OF,SF,ZF,PF are also updated in a specific way (they are not
	 * undefined).
	 * used the dosbox algo.
	 */
	SET_FLAG(SF, ((REG_AL >= 0x7a) && (REG_AL <= 0xf9)));
	if(((REG_AL & 0x0f) > 9)) {
		SET_FLAG(OF,(REG_AL & 0xf0) == 0x70);
		REG_AX += 0x106;
		SET_FLAG(CF, true);
		SET_FLAG(ZF, REG_AL == 0);
		SET_FLAG(AF, true);
	} else if(FLAG_AF) {
		REG_AX += 0x106;
		SET_FLAG(CF, true);
		SET_FLAG(AF, true);
		SET_FLAG(ZF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, false);
		SET_FLAG(AF, false);
		SET_FLAG(ZF, REG_AL == 0);
		SET_FLAG(OF, false);
	}
	SET_FLAG(PF, PARITY(REG_AL));
	REG_AL &= 0x0f;
}


/*******************************************************************************
 * AAD-ASCII Adjust AX Before Division
 */

void CPUExecutor::AAD()
{
	//according to the Intel's 286 manual, the immediate value is always 0x0A.
	//in reality it can be anything.
	//see http://www.rcollins.org/secrets/opcodes/AAD.html
	uint16_t tmp = REG_AL + (m_instr->ib * REG_AH);
	REG_AX = (tmp & 0xff);

	SET_FLAG(SF, REG_AL & 0x80);
	SET_FLAG(ZF, REG_AL == 0);
	SET_FLAG(PF, PARITY(REG_AL));
	SET_FLAG(CF, false);
	SET_FLAG(OF, false);
	SET_FLAG(AF, false);
}


/*******************************************************************************
 * AAM-ASCII Adjust AX After Multiply
 */

void CPUExecutor::AAM()
{
	//according to the Intel's 286 manual the immediate value is always 0x0A.
	//in reality it can be anything.
	//see http://www.rcollins.org/secrets/opcodes/AAM.html
	if(m_instr->ib == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}
	uint8_t al = REG_AL;
	REG_AH = al / m_instr->ib;
	REG_AL = al % m_instr->ib;

	SET_FLAG(SF, REG_AL & 0x80);
	SET_FLAG(ZF, REG_AL == 0);
	SET_FLAG(PF, PARITY(REG_AL));
	SET_FLAG(CF, false);
	SET_FLAG(OF, false);
	SET_FLAG(AF, false);
}


/*******************************************************************************
 * AAS-ASCII Adjust AL After Subtraction
 */

void CPUExecutor::AAS()
{
	if((REG_AL & 0x0f) > 9) {
		SET_FLAG(SF, REG_AL > 0x85);
		REG_AX -= 0x106;
		SET_FLAG(OF, false);
		SET_FLAG(CF, true);
		SET_FLAG(AF, true);
	} else if(FLAG_AF) {
		SET_FLAG(OF, (REG_AL >= 0x80) && (REG_AL <= 0x85));
		SET_FLAG(SF, (REG_AL < 0x06) || (REG_AL > 0x85));
		REG_AX -= 0x106;
		SET_FLAG(CF, true);
		SET_FLAG(AF, true);
	} else {
		SET_FLAG(SF, REG_AL >= 0x80);
		SET_FLAG(OF, false);
		SET_FLAG(CF, false);
		SET_FLAG(AF, false);
	}
	SET_FLAG(ZF, REG_AL == 0);
	SET_FLAG(PF, PARITY(REG_AL));
	REG_AL &= 0x0F;
}


/*******************************************************************************
 * ADC/ADD-Integer Addition
 */

uint8_t CPUExecutor::ADC_b(uint8_t op1, uint8_t op2)
{
	uint8_t cf = FLAG_CF;
	uint8_t res = op1 + op2 + cf;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x80) & (res ^ op2)) & 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, (res < op1) || (cf && (res == op1)));

	return res;
}

uint16_t CPUExecutor::ADC_w(uint16_t op1, uint16_t op2)
{
	uint16_t cf = FLAG_CF;
	uint16_t res = op1 + op2 + cf;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x8000) & (res ^ op2)) & 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, (res < op1) || (cf && (res == op1)));

	return res;
}

uint32_t CPUExecutor::ADC_d(uint32_t op1, uint32_t op2)
{
	uint32_t cf = FLAG_CF;
	uint32_t res = op1 + op2 + cf;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x80000000) & (res ^ op2)) & 0x80000000);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, (res < op1) || (cf && (res == op1)));

	return res;
}

void CPUExecutor::ADC_eb_rb() { store_eb(ADC_b(load_eb(), load_rb())); }
void CPUExecutor::ADC_ew_rw() { store_ew(ADC_w(load_ew(), load_rw())); }
void CPUExecutor::ADC_ed_rd() { store_ed(ADC_d(load_ed(), load_rd())); }
void CPUExecutor::ADC_rb_eb() { store_rb(ADC_b(load_rb(), load_eb())); }
void CPUExecutor::ADC_rw_ew() { store_rw(ADC_w(load_rw(), load_ew())); }
void CPUExecutor::ADC_rd_ed() { store_rd(ADC_d(load_rd(), load_ed())); }
void CPUExecutor::ADC_AL_ib() { REG_AL = ADC_b(REG_AL, m_instr->ib); }
void CPUExecutor::ADC_AX_iw() { REG_AX = ADC_w(REG_AX, m_instr->iw1); }
void CPUExecutor::ADC_EAX_id(){ REG_EAX = ADC_d(REG_EAX, m_instr->id1); }
void CPUExecutor::ADC_eb_ib() { store_eb(ADC_b(load_eb(), m_instr->ib)); }
void CPUExecutor::ADC_ew_iw() { store_ew(ADC_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::ADC_ed_id() { store_ed(ADC_d(load_ed(), m_instr->id1)); }
void CPUExecutor::ADC_ew_ib() { store_ew(ADC_w(load_ew(), int8_t(m_instr->ib))); }
void CPUExecutor::ADC_ed_ib() { store_ed(ADC_d(load_ed(), int8_t(m_instr->ib))); }

uint8_t CPUExecutor::ADD_b(uint8_t op1, uint8_t op2)
{
	uint8_t res = op1 + op2;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x80) & (res ^ op2)) & 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, res < op1);

	return res;
}

uint16_t CPUExecutor::ADD_w(uint16_t op1, uint16_t op2)
{
	uint16_t res = op1 + op2;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x8000) & (res ^ op2)) & 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, res < op1);

	return res;
}

uint32_t CPUExecutor::ADD_d(uint32_t op1, uint32_t op2)
{
	uint32_t res = op1 + op2;

	SET_FLAG(OF, ((op1 ^ op2 ^ 0x80000000) & (res ^ op2)) & 0x80000000);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, res < op1);

	return res;
}

void CPUExecutor::ADD_eb_rb() { store_eb(ADD_b(load_eb(), load_rb())); }
void CPUExecutor::ADD_ew_rw() { store_ew(ADD_w(load_ew(), load_rw())); }
void CPUExecutor::ADD_ed_rd() { store_ed(ADD_d(load_ed(), load_rd())); }
void CPUExecutor::ADD_rb_eb() { store_rb(ADD_b(load_rb(), load_eb())); }
void CPUExecutor::ADD_rw_ew() { store_rw(ADD_w(load_rw(), load_ew())); }
void CPUExecutor::ADD_rd_ed() { store_rd(ADD_d(load_rd(), load_ed())); }
void CPUExecutor::ADD_AL_ib() { REG_AL = ADD_b(REG_AL, m_instr->ib); }
void CPUExecutor::ADD_AX_iw() { REG_AX = ADD_w(REG_AX, m_instr->iw1); }
void CPUExecutor::ADD_EAX_id(){ REG_EAX = ADD_d(REG_EAX, m_instr->id1); }
void CPUExecutor::ADD_eb_ib() { store_eb(ADD_b(load_eb(), m_instr->ib)); }
void CPUExecutor::ADD_ew_iw() { store_ew(ADD_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::ADD_ed_id() { store_ed(ADD_d(load_ed(), m_instr->id1)); }
void CPUExecutor::ADD_ew_ib() { store_ew(ADD_w(load_ew(), int8_t(m_instr->ib))); }
void CPUExecutor::ADD_ed_ib() { store_ed(ADD_d(load_ed(), int8_t(m_instr->ib))); }


/*******************************************************************************
 * AND-Logical AND
 */

uint8_t CPUExecutor::AND_b(uint8_t op1, uint8_t op2)
{
	uint8_t res = op1 & op2;

	SET_FLAG(OF, false);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, false);
	SET_FLAG(AF, false); // unknown

	return res;
}

uint16_t CPUExecutor::AND_w(uint16_t op1, uint16_t op2)
{
	uint16_t res = op1 & op2;

	SET_FLAG(OF, false);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, false);
	SET_FLAG(AF, false); // unknown

	return res;
}

uint32_t CPUExecutor::AND_d(uint32_t op1, uint32_t op2)
{
	uint32_t res = op1 & op2;

	SET_FLAG(OF, false);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, false);
	SET_FLAG(AF, false); // unknown

	return res;
}

void CPUExecutor::AND_eb_rb() { store_eb(AND_b(load_eb(),load_rb())); }
void CPUExecutor::AND_ew_rw() { store_ew(AND_w(load_ew(),load_rw())); }
void CPUExecutor::AND_ed_rd() { store_ed(AND_d(load_ed(),load_rd())); }
void CPUExecutor::AND_rb_eb() { store_rb(AND_b(load_rb(),load_eb())); }
void CPUExecutor::AND_rw_ew() { store_rw(AND_w(load_rw(),load_ew())); }
void CPUExecutor::AND_rd_ed() { store_rd(AND_d(load_rd(),load_ed())); }
void CPUExecutor::AND_AL_ib() { REG_AL = AND_b(REG_AL, m_instr->ib); }
void CPUExecutor::AND_AX_iw() { REG_AX = AND_w(REG_AX, m_instr->iw1); }
void CPUExecutor::AND_EAX_id(){ REG_EAX = AND_d(REG_EAX, m_instr->id1); }
void CPUExecutor::AND_eb_ib() { store_eb(AND_b(load_eb(),m_instr->ib)); }
void CPUExecutor::AND_ew_iw() { store_ew(AND_w(load_ew(),m_instr->iw1)); }
void CPUExecutor::AND_ed_id() { store_ed(AND_d(load_ed(),m_instr->id1)); }
void CPUExecutor::AND_ew_ib() { store_ew(AND_w(load_ew(), int8_t(m_instr->ib))); }
void CPUExecutor::AND_ed_ib() { store_ed(AND_d(load_ed(), int8_t(m_instr->ib))); }


/*******************************************************************************
 * ARPL-Adjust RPL Field of Selector
 */

void CPUExecutor::ARPL_ew_rw()
{
	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "ARPL: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	uint16_t op1 = load_ew();
	uint16_t op2 = load_rw();

	if((op1 & 0x03) < (op2 & 0x03)) {
		op1 = (op1 & SELECTOR_RPL_MASK) | (op2 & 0x03);
		store_ew(op1);
		SET_FLAG(ZF, true);
	} else {
		SET_FLAG(ZF, false);
	}
}


/*******************************************************************************
 * BOUND-Check Array Index Against Bounds
 */

void CPUExecutor::BOUND_rw_md()
{
	int16_t op1 = int16_t(load_rw());
	uint16_t bound_min, bound_max;
	load_ed_mem(bound_min, bound_max);

	if(op1 < int16_t(bound_min) || op1 > int16_t(bound_max)) {
		PDEBUGF(LOG_V2,LOG_CPU, "BOUND: fails bounds test\n");
		throw CPUException(CPU_BOUND_EXC, 0);
	}
}

void CPUExecutor::BOUND_rd_mq()
{
	int32_t op1 = int32_t(load_rd());
	uint32_t bound_min, bound_max;
	load_eq_mem(bound_min, bound_max);

	if(op1 < int16_t(bound_min) || op1 > int16_t(bound_max)) {
		PDEBUGF(LOG_V2,LOG_CPU, "BOUND: fails bounds test\n");
		throw CPUException(CPU_BOUND_EXC, 0);
	}
}


/*******************************************************************************
 * CALL-Call Procedure
 */

void CPUExecutor::CALL_cw()
{
	/* push 16 bit EA of next instruction */
	stack_push_word(REG_IP);

	uint16_t new_IP = REG_IP + m_instr->iw1;
	branch_near(new_IP);
}

void CPUExecutor::CALL_ew()
{
	/* push 16 bit EA of next instruction */
	stack_push_word(REG_IP);

	uint16_t new_IP = load_ew();
	branch_near(new_IP);
}

void CPUExecutor::CALL_cd(uint16_t newip, uint16_t newcs)
{
	if(IS_PMODE()) {
		call_pmode(newcs, newip);
		return;
	}

	//REAL mode
	stack_push_word(REG_CS.sel.value);
	stack_push_word(REG_IP);

	// CS LIMIT can't change when in real mode
	if(newip > GET_LIMIT(CS)) {
		PDEBUGF(LOG_V2, LOG_CPU, "CALL_cd: instruction pointer not within code segment limits\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SET_CS(newcs);
	SET_IP(newip);
	g_cpubus.invalidate_pq();
}

void CPUExecutor::CALL_cd()
{
	CALL_cd(m_instr->iw1, m_instr->iw2);
}

void CPUExecutor::CALL_ed()
{
	uint16_t newip, newcs;
	load_ed_mem(newip, newcs);

	CALL_cd(newip, newcs);
}


/*******************************************************************************
 * CBW/CWD/CWDE/CDQ-Convert Byte/Word/DWord to Word/DWord/QWord
 */

void CPUExecutor::CBW()
{
	/* CBW: no flags are effected */
	REG_AX = int8_t(REG_AL);
}

void CPUExecutor::CWD()
{
	if(REG_AX & 0x8000) {
		REG_DX = 0xFFFF;
	} else {
		REG_DX = 0;
	}
}

void CPUExecutor::CWDE()
{
	REG_EAX = int16_t(REG_AX);
}

void CPUExecutor::CDQ()
{
	if(REG_EAX & 0x80000000) {
		REG_EDX = 0xFFFFFFFF;
	} else {
		REG_EDX = 0;
	}
}


/*******************************************************************************
 * CLC/CLD/CLI/CLTS-Clear Flags
 */

void CPUExecutor::CLC()
{
	SET_FLAG(CF, false);
}

void CPUExecutor::CLD()
{
	SET_FLAG(DF, false);
}

void CPUExecutor::CLI()
{
	if(IS_PMODE() && (FLAG_IOPL < CPL)) {
		PDEBUGF(LOG_V2, LOG_CPU, "CLI: IOPL < CPL in protected mode\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SET_FLAG(IF, false);
}

void CPUExecutor::CLTS()
{
	// CPL is always 0 in real mode
	if(IS_PMODE() && CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "CLTS: priveledge check failed\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SET_CR0(TS, false);
}


/*******************************************************************************
 * CMC-Complement Carry Flag
 */

void CPUExecutor::CMC()
{
	SET_FLAG(CF, !FLAG_CF);
}


/*******************************************************************************
 * CMP-Compare Two Operands
 */

void CPUExecutor::CMP_b(uint8_t op1, uint8_t op2)
{
	uint8_t res = op1 - op2;

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, op1<op2);
}

void CPUExecutor::CMP_w(uint16_t op1, uint16_t op2)
{
	uint16_t res = op1 - op2;

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, op1<op2);
}

void CPUExecutor::CMP_d(uint32_t op1, uint32_t op2)
{
	uint32_t res = op1 - op2;

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x80000000);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, op1<op2);
}

void CPUExecutor::CMP_eb_rb() { CMP_b(load_eb(), load_rb()); }
void CPUExecutor::CMP_ew_rw() { CMP_w(load_ew(), load_rw()); }
void CPUExecutor::CMP_ed_rd() { CMP_d(load_ed(), load_rd()); }
void CPUExecutor::CMP_rb_eb() { CMP_b(load_rb(), load_eb()); }
void CPUExecutor::CMP_rw_ew() { CMP_w(load_rw(), load_ew()); }
void CPUExecutor::CMP_rd_ed() { CMP_d(load_rd(), load_ed()); }
void CPUExecutor::CMP_AL_ib() { CMP_b(REG_AL, m_instr->ib); }
void CPUExecutor::CMP_AX_iw() { CMP_w(REG_AX, m_instr->iw1); }
void CPUExecutor::CMP_EAX_id(){ CMP_d(REG_EAX, m_instr->id1); }
void CPUExecutor::CMP_eb_ib() { CMP_b(load_eb(), m_instr->ib); }
void CPUExecutor::CMP_ew_iw() { CMP_w(load_ew(), m_instr->iw1); }
void CPUExecutor::CMP_ed_id() { CMP_d(load_ed(), m_instr->id1); }
void CPUExecutor::CMP_ew_ib() { CMP_w(load_ew(), int8_t(m_instr->ib)); }
void CPUExecutor::CMP_ed_ib() { CMP_d(load_ed(), int8_t(m_instr->ib)); }


/*******************************************************************************
 * CMPS/CMPSB/CMPSW/CMPSWD-Compare string operands
 */

void CPUExecutor::CMPSB_16()
{
	uint8_t op1 = read_byte(SEG_REG(m_base_ds), REG_SI);
	uint8_t op2 = read_byte(REG_ES, REG_DI);

	CMP_b(op1, op2);

	if(FLAG_DF) {
		REG_SI -= 1;
		REG_DI -= 1;
	} else {
		REG_SI += 1;
		REG_DI += 1;
	}
}

void CPUExecutor::CMPSW_16()
{
	uint16_t op1 = read_word(SEG_REG(m_base_ds), REG_SI);
	uint16_t op2 = read_word(REG_ES, REG_DI);

	CMP_w(op1, op2);

	if(FLAG_DF) {
		REG_SI -= 2;
		REG_DI -= 2;
	} else {
		REG_SI += 2;
		REG_DI += 2;
	}
}

void CPUExecutor::CMPSD_16()
{
	uint32_t op1 = read_dword(SEG_REG(m_base_ds), REG_SI);
	uint32_t op2 = read_dword(REG_ES, REG_DI);

	CMP_d(op1, op2);

	if(FLAG_DF) {
		REG_SI -= 4;
		REG_DI -= 4;
	} else {
		REG_SI += 4;
		REG_DI += 4;
	}
}

void CPUExecutor::CMPSB_32()
{
	uint8_t op1 = read_byte(SEG_REG(m_base_ds), REG_ESI);
	uint8_t op2 = read_byte(REG_ES, REG_EDI);

	CMP_b(op1, op2);

	if(FLAG_DF) {
		REG_ESI -= 1;
		REG_EDI -= 1;
	} else {
		REG_ESI += 1;
		REG_EDI += 1;
	}
}

void CPUExecutor::CMPSW_32()
{
	uint16_t op1 = read_word(SEG_REG(m_base_ds), REG_ESI);
	uint16_t op2 = read_word(REG_ES, REG_EDI);

	CMP_w(op1, op2);

	if(FLAG_DF) {
		REG_ESI -= 2;
		REG_EDI -= 2;
	} else {
		REG_ESI += 2;
		REG_EDI += 2;
	}
}

void CPUExecutor::CMPSD_32()
{
	uint32_t op1 = read_dword(SEG_REG(m_base_ds), REG_ESI);
	uint32_t op2 = read_dword(REG_ES, REG_EDI);

	CMP_d(op1, op2);

	if(FLAG_DF) {
		REG_ESI -= 4;
		REG_EDI -= 4;
	} else {
		REG_ESI += 4;
		REG_EDI += 4;
	}
}

/*******************************************************************************
 * DAA/DAS-Decimal Adjust AL after addition/subtraction
 */

void CPUExecutor::DAA()
{
	if(((REG_AL & 0x0F) > 9) || FLAG_AF) {
		REG_AL += 6;
		SET_FLAG(AF, true);
	} else {
		SET_FLAG(AF, false);
	}
	if((REG_AL > 0x9F) || FLAG_CF) {
		REG_AL += 0x60;
		SET_FLAG(CF, true);
	} else {
		SET_FLAG(CF, false);
	}
	SET_FLAG(SF, REG_AL & 0x80);
	SET_FLAG(ZF, REG_AL == 0);
	SET_FLAG(PF, PARITY(REG_AL));
}

void CPUExecutor::DAS()
{
	if(((REG_AL & 0x0F) > 9) || FLAG_AF) {
		REG_AL -= 6;
		SET_FLAG(AF, true);
	} else {
		SET_FLAG(AF, false);
	}
	if((REG_AL > 0x9F) || FLAG_CF) {
		REG_AL -= 0x60;
		SET_FLAG(CF, true);
	} else {
		SET_FLAG(CF, false);
	}
	SET_FLAG(SF, REG_AL & 0x80);
	SET_FLAG(ZF, REG_AL == 0);
	SET_FLAG(PF, PARITY(REG_AL));
}


/*******************************************************************************
 * DEC-Decrement by 1
 */

void CPUExecutor::DEC_eb()
{
	uint8_t op1 = load_eb();
	uint8_t res = op1 - 1;
	store_eb(res);

	SET_FLAG(OF, res == 0x7f);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0x0f);
	SET_FLAG(PF, PARITY(res));
}

uint16_t CPUExecutor::DEC_w(uint16_t _op1)
{
	uint16_t res = _op1 - 1;

	SET_FLAG(OF, res == 0x7fff);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0x0f);
	SET_FLAG(PF, PARITY(res));

	return res;
}

uint32_t CPUExecutor::DEC_d(uint32_t _op1)
{
	uint32_t res = _op1 - 1;

	SET_FLAG(OF, res == 0x7fffffff);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0x0f);
	SET_FLAG(PF, PARITY(res));

	return res;
}

void CPUExecutor::DEC_ew() { store_ew(DEC_w(load_ew())); }
void CPUExecutor::DEC_ed() { store_ed(DEC_d(load_ed())); }
void CPUExecutor::DEC_rw_op() { store_rw_op(DEC_w(load_rw_op())); }
void CPUExecutor::DEC_rd_op() { store_rd_op(DEC_d(load_rd_op())); }


/*******************************************************************************
 * DIV-Unsigned Divide
 */

void CPUExecutor::DIV_eb()
{
	uint8_t op2 = load_eb();
	if(op2 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	uint16_t op1 = REG_AX;

	uint16_t quotient_16 = op1 / op2;
	uint8_t remainder_8 = op1 % op2;
	uint8_t quotient_8l = quotient_16 & 0xFF;

	if(quotient_16 != quotient_8l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_AL = quotient_8l;
	REG_AH = remainder_8;
}

void CPUExecutor::DIV_ew()
{
	uint16_t op2_16 = load_ew();
	if(op2_16 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	uint32_t op1_32 = (uint32_t(REG_DX) << 16) | uint32_t(REG_AX);

	uint32_t quotient_32  = op1_32 / op2_16;
	uint16_t remainder_16 = op1_32 % op2_16;
	uint16_t quotient_16l = quotient_32 & 0xFFFF;

	if(quotient_32 != quotient_16l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_AX = quotient_16l;
	REG_DX = remainder_16;
}

void CPUExecutor::DIV_ed()
{
	uint32_t op2_32 = load_ew();
	if(op2_32 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	uint64_t op1_64 = (uint64_t(REG_EDX) << 32) | uint64_t(REG_EAX);

	uint64_t quotient_64  = op1_64 / op2_32;
	uint32_t remainder_32 = op1_64 % op2_32;
	uint32_t quotient_32l = quotient_64 & 0xFFFFFFFF;

	if(quotient_64 != quotient_32l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_EAX = quotient_32l;
	REG_EDX = remainder_32;
}


/*******************************************************************************
 * ENTER-Make Stack Frame for Procedure Parameters
 */

void CPUExecutor::ENTER()
{
	uint8_t level = m_instr->ib & 0x1F;

	stack_push_word(REG_BP);

	uint16_t frame_ptr16 = REG_SP;
	uint16_t bp = REG_BP;

	if (level > 0) {
		/* do level-1 times */
		while(--level) {
			bp -= 2;
			uint16_t temp16 = read_word(REG_SS, bp);
			stack_push_word(temp16);
		}

		/* push(frame pointer) */
		stack_push_word(frame_ptr16);
	}

	REG_SP -= m_instr->iw1; // bytes

	// ENTER finishes with memory write check on the final stack pointer
	// the memory is touched but no write actually occurs
	// emulate it by doing RMW read access from SS:SP
	//read_RMW_virtual_word_32(REG_SS, SP); TODO?
	//according to the intel docs the only exc is #SS(0) if SP were to go outside
	//of the stack limit (already checked in stack_push())

	REG_BP = frame_ptr16;
}


/*******************************************************************************
 * FPU ESC
 * this function should be used only if there's no FPU installed (TODO?)
 */

void CPUExecutor::FPU_ESC()
{
	if(CR0_EM || CR0_TS) {
		throw CPUException(CPU_NM_EXC, 0);
	}
}


/*******************************************************************************
 * HLT-Halt
 */

void CPUExecutor::HLT()
{
	// CPL is always 0 in real mode
	if(IS_PMODE() && CPL != 0) { //pmode
		PDEBUGF(LOG_V2,LOG_CPU,
			"HLT: pmode priveledge check failed, CPL=%d\n", CPL);
		throw CPUException(CPU_GP_EXC, 0);
	}

	if(!FLAG_IF) {
		PWARNF(LOG_CPU, "HLT instruction with IF=0!");
		PWARNF(LOG_CPU, " CS:IP=%04X:%04X\n", REG_CS.sel.value, REG_IP);
	}

	// stops instruction execution and places the processor in a
	// HALT state. An enabled interrupt, NMI, or reset will resume
	// execution. If interrupt (including NMI) is used to resume
	// execution after HLT, the saved CS:IP points to instruction
	// following HLT.
	g_cpu.enter_sleep_state(CPU_STATE_HALT);
}


/*******************************************************************************
 * IDIV-Signed Divide
 */

void CPUExecutor::IDIV_eb()
{
	int16_t op1 = int16_t(REG_AX);

	/* check MIN_INT case */
	if(op1 == int16_t(0x8000)) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int8_t op2 = int8_t(load_eb());

	if(op2 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int16_t quotient_16 = op1 / op2;
	int8_t remainder_8 = op1 % op2;
	int8_t quotient_8l = quotient_16 & 0xFF;

	if (quotient_16 != quotient_8l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_AL = quotient_8l;
	REG_AH = remainder_8;
}

void CPUExecutor::IDIV_ew()
{
	int32_t op1_32 = int32_t((uint32_t(REG_DX) << 16) | uint32_t(REG_AX));

	/* check MIN_INT case */
	if(op1_32 == int32_t(0x80000000)) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int16_t op2_16 = int16_t(load_ew());

	if(op2_16 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int32_t quotient_32  = op1_32 / op2_16;
	int16_t remainder_16 = op1_32 % op2_16;
	int16_t quotient_16l = quotient_32 & 0xFFFF;

	if(quotient_32 != quotient_16l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_AX = quotient_16l;
	REG_DX = remainder_16;
}

void CPUExecutor::IDIV_ed()
{
	int64_t op1_64 = int64_t((uint64_t(REG_EDX) << 32) | uint64_t(REG_EAX));

	/* check MIN_INT case */
	if(op1_64 == int64_t(0x8000000000000000LL)) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int32_t op2_32 = int32_t(load_ed());

	if(op2_32 == 0) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	int64_t quotient_64  = op1_64 / op2_32;
	int32_t remainder_32 = op1_64 % op2_32;
	int32_t quotient_32l = quotient_64 & 0xFFFFFFFF;

	if(quotient_64 != quotient_32l) {
		throw CPUException(CPU_DIV_ER_EXC, 0);
	}

	/* now write quotient back to destination */
	REG_EAX = quotient_32l;
	REG_EDX = remainder_32;
}


/*******************************************************************************
 * IMUL-Signed Multiply
 */

inline static int mul_cycles_386(int _m)
{
	/* The 80386 uses an early-out multiply algorithm. The actual number of
	clocks depends on the position of the most significant bit in the
	optimizing multiplier. The optimization occurs for positive and negative
	values. To calculate the actual clocks, use	the following formula:
	Clock = if m <> 0 then max(ceiling(log₂│m│), 3) + 6 clocks
	Clock = if m = 0 then 9 clocks
	(where m is the multiplier)
	*/
	if(_m != 0) {
		return std::max(int(std::ceil(std::log2(std::abs(_m)))), 3);
	} else {
		return 3;
	}
}

void CPUExecutor::IMUL_eb()
{
	int8_t op1 = int8_t(REG_AL);
	int8_t op2 = int8_t(load_eb());

	int16_t product_16 = op1 * op2;

	/* now write product back to destination */
	REG_AX = product_16;

	/* IMUL r/m8: condition for clearing CF & OF:
	 *   AX = sign-extend of AL to 16 bits
	 */
	if((product_16 & 0xff80)==0xff80 || (product_16 & 0xff80)==0) {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2);
	}
}

void CPUExecutor::IMUL_ew()
{
	int16_t op1_16 = int16_t(REG_AX);
	int16_t op2_16 = int16_t(load_ew());

	int32_t product_32 = int32_t(op1_16) * int32_t(op2_16);
	uint16_t product_16l = (product_32 & 0xFFFF);
	uint16_t product_16h = product_32 >> 16;

	/* now write product back to destination */
	REG_AX = product_16l;
	REG_DX = product_16h;

	/* IMUL r/m16: condition for clearing CF & OF:
	 *   DX:AX = sign-extend of AX
	 */
	if(((product_32 & 0xffff8000)==0xffff8000 || (product_32 & 0xffff8000)==0)) {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2_16);
	}
}

void CPUExecutor::IMUL_ed()
{
	int32_t op1_32 = int32_t(REG_EAX);
	int32_t op2_32 = int32_t(load_ed());

	int64_t product_64 = int64_t(op1_32) * int64_t(op2_32);
	uint32_t product_32l = (product_64 & 0xFFFFFFFF);
	uint32_t product_32h = product_64 >> 32;

	/* now write product back to destination */
	REG_EAX = product_32l;
	REG_EDX = product_32h;

	/* IMUL r/m32: condition for clearing CF & OF:
	 *   EDX:EAX = sign-extend of EAX
	 */
	uint64_t sign = product_64 & 0xFFFFFFFF80000000LL;
	if((sign==0xFFFFFFFF80000000LL || sign==0)) {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2_32);
	}
}

int16_t CPUExecutor::IMUL_w(int16_t _op1, int16_t _op2)
{
	int32_t product_32  = _op1 * _op2;

	/* Carry and overflow are set to 0 if the result fits in a signed word
	 * (between -32768 and +32767, inclusive); they are set to 1 otherwise.
	 */
	if((product_32 >= -32768) && (product_32 <= 32767)) {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(_op2);
	}

	return (product_32 & 0xFFFF);
}

int32_t CPUExecutor::IMUL_d(int32_t _op1, int32_t _op2)
{
	int64_t product_64  = _op1 * _op2;

	/* Carry and overflow are set to 0 if the result fits in a signed dword
	 * (between -2147483648 and +2147483647, inclusive); they are set to 1 otherwise.
	 */
	if((product_64 >= -2147483648LL) && (product_64 <= 2147483647LL)) {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	} else {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(_op2);
	}

	return (product_64 & 0xFFFFFFFF);
}

void CPUExecutor::IMUL_rw_ew()    { store_rw(IMUL_w(load_rw(), load_ew())); }
void CPUExecutor::IMUL_rd_ed()    { store_rd(IMUL_d(load_rd(), load_ed())); }
void CPUExecutor::IMUL_rw_ew_ib() {	store_rw(IMUL_w(load_ew(), int8_t(m_instr->ib))); }
void CPUExecutor::IMUL_rd_ed_ib() {	store_rd(IMUL_d(load_ed(), int8_t(m_instr->ib))); }
void CPUExecutor::IMUL_rw_ew_iw() {	store_rw(IMUL_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::IMUL_rd_ed_id() {	store_rd(IMUL_d(load_ed(), m_instr->id1)); }


/*******************************************************************************
 * Input from Port
 */

void CPUExecutor::IN_AL_ib()
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
		/* #GP(O) if the current privilege level is bigger (has less privilege)
		 * than IOPL; which is the privilege level found in the flags register.
		 */
		PDEBUGF(LOG_V2, LOG_CPU, "IN_AL_ib: I/O access not allowed!\n");
		throw CPUException(CPU_GP_EXC, 0);
	}
	REG_AL = g_devices.read_byte(m_instr->ib);
}

void CPUExecutor::IN_AL_DX()
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
	    PDEBUGF(LOG_V2, LOG_CPU, "IN_AL_DX: I/O access not allowed!\n");
	    throw CPUException(CPU_GP_EXC, 0);
	}
	REG_AL = g_devices.read_byte(REG_DX);
}

void CPUExecutor::IN_AX_ib()
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
	    PDEBUGF(LOG_V2, LOG_CPU, "IN_AX_ib: I/O access not allowed!\n");
	    throw CPUException(CPU_GP_EXC, 0);
	}
	REG_AX = g_devices.read_word(m_instr->ib);
}

void CPUExecutor::IN_AX_DX()
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
	    PDEBUGF(LOG_V2, LOG_CPU, "IN_AX_DX: I/O access not allowed!\n");
	    throw CPUException(CPU_GP_EXC, 0);
	}
	REG_AX = g_devices.read_word(REG_DX);
}


/*******************************************************************************
 * INC-Increment by 1
 */

void CPUExecutor::INC_eb()
{
	uint8_t op1 = load_eb();
	uint8_t res = op1 + 1;
	store_eb(res);

	SET_FLAG(OF, res == 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0);
	SET_FLAG(PF, PARITY(res));
}

uint16_t CPUExecutor::INC_w(uint16_t _op1)
{
	uint16_t res = _op1 + 1;

	SET_FLAG(OF, res == 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0);
	SET_FLAG(PF, PARITY(res));

	return res;
}

uint32_t CPUExecutor::INC_d(uint32_t _op1)
{
	uint32_t res = _op1 + 1;

	SET_FLAG(OF, res == 0x80000000);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, (res & 0x0f) == 0);
	SET_FLAG(PF, PARITY(res));

	return res;
}

void CPUExecutor::INC_ew() { store_ew(INC_w(load_ew())); }
void CPUExecutor::INC_ed() { store_ed(INC_d(load_ed())); }
void CPUExecutor::INC_rw_op() { store_rw_op(INC_w(load_rw_op())); }
void CPUExecutor::INC_rd_op() { store_rd_op(INC_d(load_rd_op())); }


/*******************************************************************************
 * INSB/INSW-Input from Port to String
 */

void CPUExecutor::INSB()
{
	// trigger any segment faults before reading from IO port
	if(IS_PMODE()) {
		if(CPL > FLAG_IOPL) {
			PDEBUGF(LOG_V2, LOG_CPU, "INSB: I/O access not allowed!\n");
			throw CPUException(CPU_GP_EXC, 0);
		}
	}
	/*
	The memory operand must be addressable from the ES register; no segment override is
	possible.
	*/
	mem_access_check(REG_ES, REG_DI, 1, IS_USER_PL, true);

	uint8_t value = g_devices.read_byte(REG_DX);
	write_byte(value);

	if(FLAG_DF) {
		REG_DI -= 1;
	} else {
		REG_DI += 1;
	}
}

void CPUExecutor::INSW()
{
	// trigger any segment faults before reading from IO port
	if(IS_PMODE()) {
		if(CPL > FLAG_IOPL) {
			PDEBUGF(LOG_V2, LOG_CPU, "INSW: I/O access not allowed!\n");
			throw CPUException(CPU_GP_EXC, 0);
		}
	}
	seg_check_write(REG_ES, REG_DI, 2);

	uint16_t value = g_devices.read_word(REG_DX);
	write_word(value);

	if(FLAG_DF) {
		REG_DI -= 2;
	} else {
		REG_DI += 2;
	}
}


/*******************************************************************************
 * INT/INTO-Call to Interrupt Procedure
 */

bool CPUExecutor::INT_debug(bool call, uint8_t vector, uint16_t ax, CPUCore *core, Memory *mem)
{
	const char * str = CPUDebugger::INT_decode(call, vector, ax, core, mem);
	if(str != nullptr) {
		PDEBUGF(LOG_V1, LOG_CPU, "%s\n", str);
	}
	return true;
}

void CPUExecutor::INT(uint8_t _vector, unsigned _type)
{
	uint8_t ah = REG_AH;
	uint32_t retaddr = GET_PHYADDR(CS, REG_IP);

	if(INT_TRAPS) {
		std::vector<inttrap_interval_t> results;
		m_inttraps_tree.findOverlapping(_vector, _vector, results);
		if(!results.empty()) {
			bool res = false;
			auto retinfo = m_inttraps_ret.insert(
				std::pair<uint32_t, std::vector<std::function<bool()>>>(
					retaddr, std::vector<std::function<bool()>>()
				)
			).first;
			for(auto t : results) {
				res |= t.value(true, _vector, REG_AX, &g_cpucore, &g_memory);

				auto retfunc = std::bind(t.value, false, _vector, REG_AX, &g_cpucore, &g_memory);
				retinfo->second.push_back(retfunc);
			}
			if(!res) {
				return;
			}
		}
	}

	//DOS 2+ - EXEC - LOAD AND/OR EXECUTE PROGRAM
	if(_vector == 0x21 && ah == 0x4B) {
		char * pname = (char*)g_memory.get_phy_ptr(GET_PHYADDR(DS, REG_DX));
		PDEBUGF(LOG_V1, LOG_CPU, "exec %s\n", pname);
		g_machine.DOS_program_launch(pname);
		m_dos_prg.push(std::pair<uint32_t,std::string>(retaddr,pname));
		//can the cpu be in pmode?
		if(!CPULOG || CPULOG_INT21_EXIT_IP==-1 || IS_PMODE()) {
			g_machine.DOS_program_start(pname);
		} else {
			//find the INT exit point
			uint32_t cs = g_cpubus.mem_read<2>(0x21*4 + 2);
			m_dos_prg_int_exit = (cs<<4) + CPULOG_INT21_EXIT_IP;
		}
	}
	else if((_vector == 0x21 && (
			ah==0x31 || //DOS 2+ - TERMINATE AND STAY RESIDENT
			ah==0x4C    //DOS 2+ - EXIT - TERMINATE WITH RETURN CODE
		)) ||
			_vector == 0x27 //DOS 1+ - TERMINATE AND STAY RESIDENT
	)
	{
		std::string oldprg,newprg;
		if(!m_dos_prg.empty()) {
			oldprg = m_dos_prg.top().second;
			m_dos_prg.pop();
			if(!m_dos_prg.empty()) {
				newprg = m_dos_prg.top().second;
			}
		}
		g_machine.DOS_program_finish(oldprg,newprg);
		m_dos_prg_int_exit = 0;
	}

	g_cpu.interrupt(_vector, _type, false, 0);
}

void CPUExecutor::INT3()   { INT(3, CPU_SOFTWARE_EXCEPTION); }
void CPUExecutor::INT_ib() { INT(m_instr->ib, CPU_SOFTWARE_INTERRUPT); }
void CPUExecutor::INTO()   { if(FLAG_OF) INT(4, CPU_SOFTWARE_EXCEPTION); }


/*******************************************************************************
 * IRET-Interrupt Return
 */

void CPUExecutor::IRET()
{
	g_cpu.unmask_event(CPU_EVENT_NMI);

	if(IS_PMODE()) {
		iret_pmode();
	} else {
		uint16_t ip     = stack_pop_word();
		uint16_t cs_raw = stack_pop_word(); // #SS has higher priority
		uint16_t flags  = stack_pop_word();

		// CS LIMIT can't change when in real mode
		if(ip > REG_CS.desc.limit) {
			PDEBUGF(LOG_V2, LOG_CPU,
				"IRET: instruction pointer not within code segment limits\n");
			throw CPUException(CPU_GP_EXC, 0);
		}
		SET_CS(cs_raw);
		SET_IP(ip);
		write_flags(flags,false,true,false);
	}
	g_cpubus.invalidate_pq();
}


/*******************************************************************************
 * Jcond-Jump Short If Condition Met
 */

void CPUExecutor::JA_cb()  { if(!FLAG_CF && !FLAG_ZF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JBE_cb() { if(FLAG_CF || FLAG_ZF)  {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JC_cb()  { if(FLAG_CF) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNC_cb() { if(!FLAG_CF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JE_cb()  { if(FLAG_ZF) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNE_cb() { if(!FLAG_ZF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JO_cb()  { if(FLAG_OF) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNO_cb() { if(!FLAG_OF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JPE_cb() { if(FLAG_PF) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JPO_cb() { if(!FLAG_PF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JS_cb()  { if(FLAG_SF) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNS_cb() { if(!FLAG_SF){branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JL_cb()  { if((FLAG_SF!=0) != (FLAG_OF!=0)) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNL_cb() { if((FLAG_SF!=0) == (FLAG_OF!=0)) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JLE_cb() { if(FLAG_ZF || ((FLAG_SF!=0) != (FLAG_OF!=0))) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JNLE_cb(){ if(!FLAG_ZF && ((FLAG_SF!=0) == (FLAG_OF!=0))) {branch_near(REG_IP + int8_t(m_instr->ib));} }
void CPUExecutor::JCXZ_cb(){ if(REG_CX==0) {branch_near(REG_IP + int8_t(m_instr->ib));} }


/*******************************************************************************
 * JMP-Jump
 */

void CPUExecutor::JMP_ew()
{
	uint16_t newip = load_ew();
	branch_near(newip);
}

void CPUExecutor::JMP_ed()
{
	uint16_t disp, cs;
	load_ed_mem(disp, cs);

	if(!IS_PMODE()) {
		branch_far(cs, disp);
	} else {
		branch_far_pmode(cs, disp);
	}
}

void CPUExecutor::JMP_cb()
{
	uint16_t new_IP = REG_IP + int8_t(m_instr->ib);
	branch_near(new_IP);
}

void CPUExecutor::JMP_cw()
{
	branch_near(REG_IP + m_instr->iw1);
}

void CPUExecutor::JMP_cd()
{
	if(!IS_PMODE()) {
		branch_far(m_instr->iw2, m_instr->iw1);
	} else {
		branch_far_pmode(m_instr->iw2, m_instr->iw1);
	}
}


/*******************************************************************************
 * Load Flags into AH register
 */

void CPUExecutor::LAHF()
{
	REG_AH = uint8_t(GET_FLAGS());
}


/*******************************************************************************
 * LAR-Load Access Rights Byte
 */

void CPUExecutor::LAR_rw_ew()
{
	Descriptor descriptor;
	Selector   selector;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "LAR: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	selector = load_ew();

	/* if selector null, clear ZF and done */
	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		SET_FLAG(ZF, false);
		return;
	}

	try {
		descriptor = g_cpucore.fetch_descriptor(selector,0);
	} catch(CPUException &e) {
		//this fetch does not throw an exception
		PDEBUGF(LOG_V2, LOG_CPU, "LAR: failed to fetch descriptor\n");
		SET_FLAG(ZF, false);
		return;
	}

	if(!descriptor.valid) {
		PDEBUGF(LOG_V2, LOG_CPU, "LAR: descriptor not valid\n");
		SET_FLAG(ZF, false);
		return;
	}

	/* if source selector is visible at CPL & RPL,
	 * within the descriptor table, and of type accepted by LAR instruction,
	 * then load register with segment limit and set ZF
	 */

	if(descriptor.segment) { /* normal segment */
		if(descriptor.is_code_segment() && descriptor.is_code_segment_conforming()) {
			/* ignore DPL for conforming segments */
		} else {
			if(descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
				SET_FLAG(ZF, false);
				return;
			}
		}
	} else { /* system or gate segment */
		switch(descriptor.type) {
			case DESC_TYPE_AVAIL_286_TSS:
			case DESC_TYPE_BUSY_286_TSS:
			case DESC_TYPE_286_CALL_GATE:
			case DESC_TYPE_TASK_GATE:
			case DESC_TYPE_LDT_DESC:
				break;
			default: /* rest not accepted types to LAR */
				PDEBUGF(LOG_V2, LOG_CPU, "LAR: not accepted descriptor type\n");
				SET_FLAG(ZF, false);
				return;
		}

		if(descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
			SET_FLAG(ZF, false);
			return;
		}
	}

	SET_FLAG(ZF, true);
	uint16_t value = uint16_t(descriptor.ar)<<8;
	store_rw(value);
}


/*******************************************************************************
 * LDS/LES-Load Doubleword Pointer
 */

void CPUExecutor::LDS_rw_ed()
{
	uint16_t reg, ds;
	load_ed_mem(reg, ds);

	SET_DS(ds);
	store_rw(reg);
}

void CPUExecutor::LES_rw_ed()
{
	uint16_t reg, es;
	load_ed_mem(reg, es);

	SET_ES(es);
	store_rw(reg);
}


/*******************************************************************************
 * LEA-Load Effective Address Offset
 */

void CPUExecutor::LEA_rw_m()
{
	if(m_instr->modrm.mod == 3) {
		PDEBUGF(LOG_V2, LOG_CPU, "LEA second operand is a register\n");
		throw CPUException(CPU_UD_EXC, 0);
	}
	uint16_t offset = (this->*EA_get_offset)();
	store_rw(offset);
}


/*******************************************************************************
 * LEAVE-High Level Procedure Exit
 */

void CPUExecutor::LEAVE()
{
	REG_SP = REG_BP;
	REG_BP = stack_pop_word();
}


/*******************************************************************************
 * LGDT/LIDT/LLDT-Load Global/Interrupt/Local Descriptor Table Register
 */

void CPUExecutor::LGDT()
{
	// CPL is always 0 is real mode
	if(IS_PMODE() && CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LGDT: CPL != 0 causes #GP\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SegReg & sr = (this->*EA_get_segreg)();
	uint16_t off = (this->*EA_get_offset)();

	uint16_t limit = read_word(sr, off);
	uint32_t base = read_dword(sr, off+2) & 0x00ffffff;

	SET_GDTR(base, limit);
}

void CPUExecutor::LIDT()
{
	// CPL is always 0 is real mode
	if(IS_PMODE() && CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LIDT: CPL != 0 causes #GP\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SegReg & sr = (this->*EA_get_segreg)();
	uint16_t off = (this->*EA_get_offset)();

	uint16_t limit = read_word(sr, off);
	uint32_t base = read_dword(sr, off+2) & 0x00ffffff;

	if(limit==0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LIDT: base 0x%06X, limit 0x%04X\n", base, limit);
	}

	SET_IDTR(base, limit);
}

void CPUExecutor::LLDT_ew()
{
	/* protected mode */
	Descriptor  descriptor;
	Selector    selector;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU,"LLDT: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	if(CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU,"LLDT: The current priveledge level is not 0\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	selector = load_ew();

	/* if selector is NULL, invalidate and done */
	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		REG_LDTR.sel = selector;
		REG_LDTR.desc.valid = false;
		return;
	}

	// #GP(selector) if the selector operand does not point into GDT
	if(selector.ti != 0) {
		PDEBUGF(LOG_V2, LOG_CPU,"LLDT: selector.ti != 0\n");
		throw CPUException(CPU_GP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	/* fetch descriptor; call handles out of limits checks */
	descriptor = g_cpucore.fetch_descriptor(selector, CPU_GP_EXC);

	/* if selector doesn't point to an LDT descriptor #GP(selector) */
	if(!descriptor.valid || descriptor.segment ||
         descriptor.type != DESC_TYPE_LDT_DESC)
	{
		PDEBUGF(LOG_V2, LOG_CPU,"LLDT: doesn't point to an LDT descriptor!\n");
		throw CPUException(CPU_GP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	/* #NP(selector) if LDT descriptor is not present */
	if(!descriptor.present) {
		PDEBUGF(LOG_V2, LOG_CPU,"LLDT: LDT descriptor not present!\n");
		throw CPUException(CPU_NP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	REG_LDTR.sel = selector;
	REG_LDTR.desc = descriptor;
}


/*******************************************************************************
 * LMSW-Load Machine Status Word
 */

void CPUExecutor::LMSW_ew()
{
	uint16_t msw;

	// CPL is always 0 in real mode
	if(IS_PMODE() && CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LMSW: CPL!=0\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	msw = load_ew();

	// LMSW cannot clear PE
	if(CR0_PE) {
		msw |= CR0MASK_PE; // adjust PE to current value of 1
	} else if(msw & CR0MASK_PE) {
		PDEBUGF(LOG_V2, LOG_CPU, "now in Protected Mode\n");
	}

	SET_MSW(msw);
}


/*******************************************************************************
 * LOADALL-Load registers from memory
 */

void CPUExecutor::LOADALL()
{
	/* Undocumented
	 * From 15-page Intel document titled "Undocumented iAPX 286 Test Instruction"
	 * http://www.rcollins.org/articles/loadall/tspec_a3_doc.html
	 */

	uint16_t word_reg;
	uint16_t desc_cache[3];
	uint32_t base,limit;

	if(IS_PMODE() && (CPL != 0)) {
		PDEBUGF(LOG_V2, LOG_CPU, "LOADALL: CPL != 0 causes #GP\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	PDEBUGF(LOG_V2, LOG_CPU, "LOADALL\n");

	word_reg = g_cpubus.mem_read<2>(0x806);
	if(CR0_PE) {
		word_reg |= CR0MASK_PE; // adjust PE to current value of 1
	}
	SET_MSW(word_reg);

	REG_TR.sel = g_cpubus.mem_read<2>(0x816);
	SET_FLAGS(g_cpubus.mem_read<2>(0x818));
	SET_IP(g_cpubus.mem_read<2>(0x81A));
	REG_LDTR.sel = g_cpubus.mem_read<2>(0x81C);
	REG_DS.sel = g_cpubus.mem_read<2>(0x81E);
	REG_SS.sel = g_cpubus.mem_read<2>(0x820);
	REG_CS.sel = g_cpubus.mem_read<2>(0x822);
	REG_ES.sel = g_cpubus.mem_read<2>(0x824);
	REG_DI = g_cpubus.mem_read<2>(0x826);
	REG_SI = g_cpubus.mem_read<2>(0x828);
	REG_BP = g_cpubus.mem_read<2>(0x82A);
	REG_SP = g_cpubus.mem_read<2>(0x82C);
	REG_BX = g_cpubus.mem_read<2>(0x82E);
	REG_DX = g_cpubus.mem_read<2>(0x830);
	REG_CX = g_cpubus.mem_read<2>(0x832);
	REG_AX = g_cpubus.mem_read<2>(0x834);

	desc_cache[0] = g_cpubus.mem_read<2>(0x836);
	desc_cache[1] = g_cpubus.mem_read<2>(0x838);
	desc_cache[2] = g_cpubus.mem_read<2>(0x83A);
	REG_ES.desc.set_from_286_cache(desc_cache);

	desc_cache[0] = g_cpubus.mem_read<2>(0x83C);
	desc_cache[1] = g_cpubus.mem_read<2>(0x83E);
	desc_cache[2] = g_cpubus.mem_read<2>(0x840);
	REG_CS.desc.set_from_286_cache(desc_cache);

	desc_cache[0] = g_cpubus.mem_read<2>(0x842);
	desc_cache[1] = g_cpubus.mem_read<2>(0x844);
	desc_cache[2] = g_cpubus.mem_read<2>(0x846);
	REG_SS.desc.set_from_286_cache(desc_cache);

	desc_cache[0] = g_cpubus.mem_read<2>(0x848);
	desc_cache[1] = g_cpubus.mem_read<2>(0x84A);
	desc_cache[2] = g_cpubus.mem_read<2>(0x84C);
	REG_DS.desc.set_from_286_cache(desc_cache);

	base  = g_cpubus.mem_read<4>(0x84E);
	limit = g_cpubus.mem_read<2>(0x852);
	SET_GDTR(base, limit);

	desc_cache[0] = g_cpubus.mem_read<2>(0x854);
	desc_cache[1] = g_cpubus.mem_read<2>(0x856);
	desc_cache[2] = g_cpubus.mem_read<2>(0x858);
	REG_LDTR.desc.set_from_286_cache(desc_cache);

	base  = g_cpubus.mem_read<4>(0x85A);
	limit = g_cpubus.mem_read<2>(0x85E);
	SET_IDTR(base, limit);

	desc_cache[0] = g_cpubus.mem_read<2>(0x860);
	desc_cache[1] = g_cpubus.mem_read<2>(0x862);
	desc_cache[2] = g_cpubus.mem_read<2>(0x864);
	REG_TR.desc.set_from_286_cache(desc_cache);

	g_cpubus.invalidate_pq();
}


/*******************************************************************************
 * LODSB/LODSW-Load String Operand
 */

void CPUExecutor::LODSB()
{
	REG_AL = read_byte(SEG_REG(m_base_ds), REG_SI);

	if(FLAG_DF) {
		REG_SI -= 1;
	} else {
		REG_SI += 1;
	}
}

void CPUExecutor::LODSW()
{
	REG_AX = read_word(SEG_REG(m_base_ds), REG_SI);

	if(FLAG_DF) {
		REG_SI -= 2;
	} else {
		REG_SI += 2;
	}
}


/*******************************************************************************
 * LOOP/LOOPcond-Loop Control with CX Counter
 */

void CPUExecutor::LOOP()
{
	uint16_t count = REG_CX;

	count--;
	if(count != 0) {
		uint16_t new_IP = REG_IP + int8_t(m_instr->ib);
		branch_near(new_IP);
	}

    REG_CX = count;
}

void CPUExecutor::LOOPZ()
{
	uint16_t count = REG_CX;

	count--;
	if(count != 0 && FLAG_ZF) {
		uint16_t new_IP = REG_IP + int8_t(m_instr->ib);
		branch_near(new_IP);
	}

	REG_CX = count;
}

void CPUExecutor::LOOPNZ()
{
	uint16_t count = REG_CX;

	count--;
	if(count != 0 && (FLAG_ZF==false)) {
		uint16_t new_IP = REG_IP + int8_t(m_instr->ib);
		branch_near(new_IP);
	}

	REG_CX = count;
}


/*******************************************************************************
 * LSL-Load Segment Limit
 */

void CPUExecutor::LSL_rw_ew()
{
	Selector   selector;
	Descriptor descriptor;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "LSL: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	selector = load_ew();

	/* if selector null, clear ZF and done */
	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		SET_FLAG(ZF, false);
	    return;
	}

	try {
		descriptor = g_cpucore.fetch_descriptor(selector, CPU_GP_EXC);
	} catch(CPUException &e) {
		PDEBUGF(LOG_V2, LOG_CPU, "LSL: failed to fetch descriptor\n");
	    SET_FLAG(ZF, false);
	    return;
	}

	if(!descriptor.segment) { // system segment
		switch (descriptor.type) {
			case DESC_TYPE_AVAIL_286_TSS:
			case DESC_TYPE_BUSY_286_TSS:
			case DESC_TYPE_LDT_DESC:
				if(descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
					SET_FLAG(ZF, false);
					return;
				}
				break;
			default: /* rest not accepted types to LSL */
				SET_FLAG(ZF, false);
				return;
		}
	} else { // data & code segment
		if(descriptor.is_code_segment_non_conforming()) {
			// non-conforming code segment
			if(descriptor.dpl < CPL || descriptor.dpl < selector.rpl) {
				SET_FLAG(ZF, false);
				return;
			}
		}
	}

	/* all checks pass */
	SET_FLAG(ZF, true);
	store_rw(descriptor.limit);
}


/*******************************************************************************
 * LTR-Load Task Register
 */

void CPUExecutor::LTR_ew()
{
	Descriptor descriptor;
	Selector   selector;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	if(CPL != 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: The current priveledge level is not 0\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	selector = load_ew();

	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: loading with NULL selector!\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	if(selector.ti) {
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: selector.ti != 0\n");
		throw CPUException(CPU_GP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	/* fetch descriptor; call handles out of limits checks */
	descriptor = g_cpucore.fetch_descriptor(selector, CPU_GP_EXC);

	/* #GP(selector) if object is not a TSS or is already busy */
	if(!descriptor.valid || descriptor.segment || descriptor.type != DESC_TYPE_AVAIL_286_TSS)
	{
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: doesn't point to an available TSS descriptor!\n");
		throw CPUException(CPU_GP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	/* #NP(selector) if TSS descriptor is not present */
	if(!descriptor.present) {
		PDEBUGF(LOG_V2, LOG_CPU, "LTR: TSS descriptor not present!\n");
		throw CPUException(CPU_NP_EXC, selector.value & SELECTOR_RPL_MASK);
	}

	REG_TR.sel  = selector;
	REG_TR.desc = descriptor;

	/* mark as busy */
	REG_TR.desc.type = DESC_TYPE_BUSY_286_TSS;
	g_cpubus.mem_write<1>(GET_BASE(GDTR) + selector.index*8 + 5, REG_TR.desc.get_AR());
}


/*******************************************************************************
 * MOV-Move Data
 */

void CPUExecutor::MOV_eb_rb() { store_eb(load_rb()); }
void CPUExecutor::MOV_ew_rw() { store_ew(load_rw()); }
void CPUExecutor::MOV_ed_rd() { store_ed(load_rd()); }
void CPUExecutor::MOV_rb_eb() { store_rb(load_eb()); }
void CPUExecutor::MOV_rw_ew() { store_rw(load_ew()); }
void CPUExecutor::MOV_rd_ed() { store_rd(load_ed()); }
void CPUExecutor::MOV_ew_SR() { store_ew(load_sr()); }
void CPUExecutor::MOV_SR_ew() {	store_sr(load_ew()); }
void CPUExecutor::MOV_AL_xb() { REG_AL = read_byte(SEG_REG(m_base_ds), m_instr->offset); }
void CPUExecutor::MOV_AX_xw() { REG_AX = read_word(SEG_REG(m_base_ds), m_instr->offset); }
void CPUExecutor::MOV_EAX_xd(){ REG_EAX = read_dword(SEG_REG(m_base_ds), m_instr->offset); }
void CPUExecutor::MOV_xb_AL() { write_byte(SEG_REG(m_base_ds), m_instr->offset, REG_AL); }
void CPUExecutor::MOV_xw_AX() { write_word(SEG_REG(m_base_ds), m_instr->offset, REG_AX); }
void CPUExecutor::MOV_xd_EAX(){ write_dword(SEG_REG(m_base_ds), m_instr->offset, REG_EAX); }
void CPUExecutor::MOV_rb_ib() { store_rb_op(m_instr->ib); }
void CPUExecutor::MOV_rw_iw() { store_rw_op(m_instr->iw1); }
void CPUExecutor::MOV_rd_id() { store_rd_op(m_instr->id1); }
void CPUExecutor::MOV_eb_ib() { store_eb(m_instr->ib); }
void CPUExecutor::MOV_ew_iw() { store_ew(m_instr->iw1); }
void CPUExecutor::MOV_ed_id() { store_ed(m_instr->id1); }


/*******************************************************************************
 * MOVSB/MOVSW/MOVSD-Move Data from String to String
 */

void CPUExecutor::MOVSB_16()
{
	uint8_t temp = read_byte(SEG_REG(m_base_ds), REG_SI);
	write_byte(REG_ES, REG_DI, temp);

	if(FLAG_DF) {
		/* decrement SI, DI */
		REG_SI -= 1;
		REG_DI -= 1;
	} else {
		/* increment SI, DI */
		REG_SI += 1;
		REG_DI += 1;
	}
}

void CPUExecutor::MOVSW_16()
{
	uint16_t temp = read_word(SEG_REG(m_base_ds), REG_SI);
	write_word(REG_ES, REG_DI, temp);

	if(FLAG_DF) {
		/* decrement SI, DI */
		REG_SI -= 2;
		REG_DI -= 2;
	} else {
		/* increment SI, DI */
		REG_SI += 2;
		REG_DI += 2;
	}
}

void CPUExecutor::MOVSD_16()
{
	uint32_t temp = read_dword(SEG_REG(m_base_ds), REG_SI);
	write_dword(REG_ES, REG_DI, temp);

	if(FLAG_DF) {
		/* decrement SI, DI */
		REG_SI -= 4;
		REG_DI -= 4;
	} else {
		/* increment SI, DI */
		REG_SI += 4;
		REG_DI += 4;
	}
}

void CPUExecutor::MOVSB_32()
{
	uint8_t temp = read_byte(SEG_REG(m_base_ds), REG_ESI);
	write_byte(REG_ES, REG_EDI, temp);

	if(FLAG_DF) {
		/* decrement ESI, EDI */
		REG_ESI -= 1;
		REG_EDI -= 1;
	} else {
		/* increment ESI, EDI */
		REG_ESI += 1;
		REG_EDI += 1;
	}
}

void CPUExecutor::MOVSW_32()
{
	uint16_t temp = read_word(SEG_REG(m_base_ds), REG_ESI);
	write_word(REG_ES, REG_EDI, temp);

	if(FLAG_DF) {
		/* decrement ESI, EDI */
		REG_ESI -= 2;
		REG_EDI -= 2;
	} else {
		/* increment ESI, EDI */
		REG_ESI += 2;
		REG_EDI += 2;
	}
}

void CPUExecutor::MOVSD_32()
{
	uint32_t temp = read_dword(SEG_REG(m_base_ds), REG_ESI);
	write_dword(REG_ES, REG_EDI, temp);

	if(FLAG_DF) {
		/* decrement ESI, EDI */
		REG_ESI -= 4;
		REG_EDI -= 4;
	} else {
		/* increment ESI, EDI */
		REG_ESI += 4;
		REG_EDI += 4;
	}
}


/*******************************************************************************
 * MUL-Unsigned Multiplication of AL / AX / EAX
 */

void CPUExecutor::MUL_eb()
{
	uint8_t op1_8 = REG_AL;
	uint8_t op2_8 = load_eb();

	uint16_t product_16 = uint16_t(op1_8) * uint16_t(op2_8);

	uint8_t product_8h = product_16 >> 8;

	/* now write product back to destination */
	REG_AX = product_16;

	if(product_8h) {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	} else {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2_8);
	}
}

void CPUExecutor::MUL_ew()
{
	uint16_t op1_16 = REG_AX;
	uint16_t op2_16 = load_ew();

	uint32_t product_32  = uint32_t(op1_16) * uint32_t(op2_16);

	uint16_t product_16l = product_32 & 0xFFFF;
	uint16_t product_16h = product_32 >> 16;

	/* now write product back to destination */
	REG_AX = product_16l;
	REG_DX = product_16h;

	if(product_16h) {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	} else {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2_16);
	}
}

void CPUExecutor::MUL_ed()
{
	uint32_t op1_32 = REG_EAX;
	uint32_t op2_32 = load_ed();

	uint64_t product_64  = uint64_t(op1_32) * uint64_t(op2_32);

	uint32_t product_32l = product_64 & 0xFFFFFFFF;
	uint32_t product_32h = product_64 >> 32;

	/* now write product back to destination */
	REG_EAX = product_32l;
	REG_EDX = product_32h;

	if(product_32h) {
		SET_FLAG(CF, true);
		SET_FLAG(OF, true);
	} else {
		SET_FLAG(CF, false);
		SET_FLAG(OF, false);
	}

	if(CPU_TYPE == CPU_386) {
		m_instr->cycles.extra = mul_cycles_386(op2_32);
	}
}


/*******************************************************************************
 * NEG-Two's Complement Negation
 */

void CPUExecutor::NEG_eb()
{
	uint8_t op1 = load_eb();
	uint8_t res = -(int8_t)(op1);
	store_eb(res);

	SET_FLAG(CF, op1);
	SET_FLAG(AF, op1 & 0x0f);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(OF, op1 == 0x80);
	SET_FLAG(PF, PARITY(res));
}

void CPUExecutor::NEG_ew()
{
	uint16_t op1 = load_ew();
	uint16_t res = -(int16_t)(op1);
	store_ew(res);

	SET_FLAG(CF, op1);
	SET_FLAG(AF, op1 & 0x0f);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(OF, op1 == 0x8000);
	SET_FLAG(PF, PARITY(res));
}

void CPUExecutor::NEG_ed()
{
	uint32_t op1 = load_ed();
	uint32_t res = -(int32_t)(op1);
	store_ed(res);

	SET_FLAG(CF, op1);
	SET_FLAG(AF, op1 & 0x0f);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(OF, op1 == 0x80000000);
	SET_FLAG(PF, PARITY(res));
}


/*******************************************************************************
 * NOP-No OPERATION
 */

void CPUExecutor::NOP() {}


/*******************************************************************************
 * NOT-One's Complement Negation
 */

void CPUExecutor::NOT_eb()
{
	uint8_t op1 = load_eb();
	op1 = ~op1;
	store_eb(op1);
}

void CPUExecutor::NOT_ew()
{
	uint16_t op1 = load_ew();
	op1 = ~op1;
	store_ew(op1);
}

void CPUExecutor::NOT_ed()
{
	uint32_t op1 = load_ed();
	op1 = ~op1;
	store_ed(op1);
}


/*******************************************************************************
 * OR-Logical Inclusive OR
 */

uint8_t CPUExecutor::OR_b(uint8_t op1, uint8_t op2)
{
	uint8_t res = op1 | op2;

	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(OF, false);
	SET_FLAG(CF, false);

	return res;
}

uint16_t CPUExecutor::OR_w(uint16_t op1, uint16_t op2)
{
	uint16_t res = op1 | op2;

	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(OF, false);
	SET_FLAG(CF, false);

	return res;
}

uint32_t CPUExecutor::OR_d(uint32_t op1, uint32_t op2)
{
	uint32_t res = op1 | op2;

	SET_FLAG(SF, res & 0x80000000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(OF, false);
	SET_FLAG(CF, false);

	return res;
}

void CPUExecutor::OR_eb_rb() { store_eb(OR_b(load_eb(), load_rb())); }
void CPUExecutor::OR_ew_rw() { store_ew(OR_w(load_ew(), load_rw())); }
void CPUExecutor::OR_ed_rd() { store_ed(OR_d(load_ed(), load_rd())); }
void CPUExecutor::OR_rb_eb() { store_rb(OR_b(load_rb(), load_eb())); }
void CPUExecutor::OR_rw_ew() { store_rw(OR_w(load_rw(), load_ew())); }
void CPUExecutor::OR_rd_ed() { store_rd(OR_d(load_rd(), load_ed())); }
void CPUExecutor::OR_AL_ib() { REG_AL = OR_b(REG_AL, m_instr->ib); }
void CPUExecutor::OR_AX_iw() { REG_AX = OR_w(REG_AX, m_instr->iw1); }
void CPUExecutor::OR_EAX_id(){ REG_EAX = OR_d(REG_EAX, m_instr->id1); }
void CPUExecutor::OR_eb_ib() { store_eb(OR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::OR_ew_iw() { store_ew(OR_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::OR_ed_id() { store_ed(OR_d(load_ed(), m_instr->id1)); }
void CPUExecutor::OR_ew_ib() { store_ew(OR_w(load_ew(), int8_t(m_instr->ib))); }
void CPUExecutor::OR_ed_ib() { store_ed(OR_d(load_ed(), int8_t(m_instr->ib))); }


/*******************************************************************************
 * OUT-Output to port
 */

void CPUExecutor::OUT_b(uint16_t _port, uint8_t _value)
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
	    PDEBUGF(LOG_V2, LOG_CPU, "OUT_b: I/O access not allowed!\n");
	    throw CPUException(CPU_GP_EXC, 0);
	}

	g_devices.write_byte(_port, _value);
}

void CPUExecutor::OUT_w(uint16_t _port, uint16_t _value)
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
	    PDEBUGF(LOG_V2, LOG_CPU, "OUT_w: I/O access not allowed!\n");
	    throw CPUException(CPU_GP_EXC, 0);
	}

	g_devices.write_word(_port, _value);
}

void CPUExecutor::OUT_ib_AL() { OUT_b(m_instr->ib, REG_AL); }
void CPUExecutor::OUT_ib_AX() { OUT_w(m_instr->ib, REG_AX); }
void CPUExecutor::OUT_DX_AL() { OUT_b(REG_DX, REG_AL); }
void CPUExecutor::OUT_DX_AX() { OUT_w(REG_DX, REG_AX); }


/*******************************************************************************
 * OUTSB/OUTSW-Output String to Port
 */

void CPUExecutor::OUTSB()
{
	uint8_t value = read_byte(SEG_REG(m_base_ds), REG_SI);

	OUT_b(REG_DX, value);

	if(FLAG_DF) {
		REG_SI -= 1;
	} else {
		REG_SI += 1;
	}
}

void CPUExecutor::OUTSW()
{
	uint16_t value = read_word(SEG_REG(m_base_ds), REG_SI);

	OUT_w(REG_DX, value);

	if(FLAG_DF) {
		REG_SI -= 2;
	} else {
		REG_SI += 2;
	}
}


/*******************************************************************************
 * POP-Pop Operand from the Stack
 */

void CPUExecutor::POP_DS()    { SET_DS(stack_pop_word()); }
void CPUExecutor::POP_ES()    { SET_ES(stack_pop_word()); }
void CPUExecutor::POP_FS()    { SET_FS(stack_pop_word()); }
void CPUExecutor::POP_GS()    { SET_GS(stack_pop_word()); }
void CPUExecutor::POP_DS_32() { SET_DS(stack_pop_dword()); }
void CPUExecutor::POP_ES_32() {	SET_ES(stack_pop_dword()); }
void CPUExecutor::POP_FS_32() { SET_FS(stack_pop_dword()); }
void CPUExecutor::POP_GS_32() {	SET_GS(stack_pop_dword()); }
void CPUExecutor::POP_mw()    { store_ew(stack_pop_word()); }
void CPUExecutor::POP_md()    { store_ed(stack_pop_dword()); }
void CPUExecutor::POP_rw_op() { store_rw_op(stack_pop_word()); }
void CPUExecutor::POP_rd_op() { store_rd_op(stack_pop_dword()); }

void CPUExecutor::POP_SS()
{
	SET_SS(stack_pop_word());
	/* A POP SS instruction will inhibit all interrupts, including NMI, until
	 * after the execution of the next instruction. This permits a POP SP
	 * instruction to be performed first. (cf. B-83)
	 */
	g_cpu.inhibit_interrupts(CPU_INHIBIT_INTERRUPTS_BY_MOVSS);
}

void CPUExecutor::POP_SS_32()
{
	SET_SS(stack_pop_dword());
	g_cpu.inhibit_interrupts(CPU_INHIBIT_INTERRUPTS_BY_MOVSS);
}

/*******************************************************************************
 * POPA/POPAD-Pop All General Registers
 */

void CPUExecutor::POPA()
{
	REG_DI = stack_pop_word();
	REG_SI = stack_pop_word();
	REG_BP = stack_pop_word();
	         stack_pop_word(); //skip SP
	REG_BX = stack_pop_word();
	REG_DX = stack_pop_word();
	REG_CX = stack_pop_word();
	REG_AX = stack_pop_word();
}

void CPUExecutor::POPAD()
{
	REG_EDI = stack_pop_dword();
	REG_ESI = stack_pop_dword();
	REG_EBP = stack_pop_dword();
	          stack_pop_dword(); //skip ESP
	REG_EBX = stack_pop_dword();
	REG_EDX = stack_pop_dword();
	REG_ECX = stack_pop_dword();
	REG_EAX = stack_pop_dword();
}


/*******************************************************************************
 * POPF/POPFD-Pop from Stack into the FLAGS or EFLAGS Register
 */

void CPUExecutor::POPF()
{
	uint16_t flags = stack_pop_word();
	write_flags(flags);
}

void CPUExecutor::POPFD()
{
	/* POPF and POPFD don't affect bit 16 & 17 of EFLAGS, so use the
	 * same write_flags as POPF
	 * TODO this works only for the 386
	 */
	uint16_t flags = uint16_t(stack_pop_dword());
	write_flags(flags);
}


/*******************************************************************************
 * PUSH-Push Operand onto the Stack
 */

void CPUExecutor::PUSH_ES()    { stack_push_word(REG_ES.sel.value); }
void CPUExecutor::PUSH_CS()    { stack_push_word(REG_CS.sel.value); }
void CPUExecutor::PUSH_SS()    { stack_push_word(REG_SS.sel.value); }
void CPUExecutor::PUSH_DS()    { stack_push_word(REG_DS.sel.value); }
void CPUExecutor::PUSH_FS()    { stack_push_word(REG_FS.sel.value); }
void CPUExecutor::PUSH_GS()    { stack_push_word(REG_GS.sel.value); }
void CPUExecutor::PUSH_ES_32() { stack_push_dword(REG_ES.sel.value); }
void CPUExecutor::PUSH_CS_32() { stack_push_dword(REG_CS.sel.value); }
void CPUExecutor::PUSH_SS_32() { stack_push_dword(REG_SS.sel.value); }
void CPUExecutor::PUSH_DS_32() { stack_push_dword(REG_DS.sel.value); }
void CPUExecutor::PUSH_FS_32() { stack_push_dword(REG_FS.sel.value); }
void CPUExecutor::PUSH_GS_32() { stack_push_dword(REG_GS.sel.value); }
void CPUExecutor::PUSH_rw_op() { stack_push_word(load_rw_op()); }
void CPUExecutor::PUSH_rd_op() { stack_push_dword(load_rd_op()); }
void CPUExecutor::PUSH_mw()    { stack_push_word(load_ew()); }
void CPUExecutor::PUSH_md()    { stack_push_dword(load_ed()); }
void CPUExecutor::PUSH_ib()    { stack_push_word(int8_t(m_instr->ib)); }
void CPUExecutor::PUSH_iw()    { stack_push_word(m_instr->iw1); }
void CPUExecutor::PUSH_id()    { stack_push_word(m_instr->id1); }


/*******************************************************************************
 * PUSHA-Push All General Registers
 */

void CPUExecutor::PUSHA()
{
	if(!IS_PMODE()) {
		uint32_t sp = (REG_SS.desc.big)?REG_ESP:REG_SP;
		if(sp == 7 || sp == 9 || sp == 11 || sp == 13 || sp == 15) {
			throw CPUException(CPU_SEG_OVR_EXC,0);
		}
		if(sp == 1 || sp == 3 || sp == 5) {
			throw CPUShutdown("SP=1,3,5 on stack push (PUSHA)");
		}
	}
	uint16_t temp_SP  = REG_SP;
	stack_push_word(REG_AX);
	stack_push_word(REG_CX);
	stack_push_word(REG_DX);
	stack_push_word(REG_BX);
	stack_push_word(temp_SP);
	stack_push_word(REG_BP);
	stack_push_word(REG_SI);
	stack_push_word(REG_DI);
}

void CPUExecutor::PUSHAD()
{
	if(!IS_PMODE()) {
		uint32_t sp = (REG_SS.desc.big)?REG_ESP:REG_SP;
		if(sp == 7 || sp == 9 || sp == 11 || sp == 13 || sp == 15) {
			throw CPUException(CPU_SEG_OVR_EXC,0);
		}
		if(sp == 1 || sp == 3 || sp == 5) {
			throw CPUShutdown("SP=1,3,5 on stack push (PUSHAD)");
		}
	}
	uint32_t temp_ESP  = REG_ESP;
	stack_push_dword(REG_EAX);
	stack_push_dword(REG_ECX);
	stack_push_dword(REG_EDX);
	stack_push_dword(REG_EBX);
	stack_push_dword(temp_ESP);
	stack_push_dword(REG_EBP);
	stack_push_dword(REG_ESI);
	stack_push_dword(REG_EDI);
}


/*******************************************************************************
 * PUSHF/PUSHFD-Push FLAGS or EFLAGS Register onto the Stack
 */

void CPUExecutor::PUSHF()
{
	if(IS_V8086() && FLAG_IOPL < 3) {
		PDEBUGF(LOG_V2, LOG_CPU, "Push Flags: general protection in v8086 mode\n");
		throw CPUException(CPU_GP_EXC, 0);
	}
	uint16_t flags = GET_FLAGS();
	stack_push_word(flags);
}

void CPUExecutor::PUSHFD()
{
	if(IS_V8086() && FLAG_IOPL < 3) {
		PDEBUGF(LOG_V2, LOG_CPU, "Push Flags: general protection in v8086 mode\n");
		throw CPUException(CPU_GP_EXC, 0);
	}
	// VM & RF flags cleared when pushed onto stack
	uint32_t eflags = GET_EFLAGS() & ~(FMASK_RF | FMASK_VM);
	stack_push_dword(eflags);
}


/*******************************************************************************
 * RCL/RCR/ROL/ROR-Rotate Instructions
 */

uint8_t CPUExecutor::ROL_b(uint8_t _value, uint8_t _times)
{
	if(!(_times & 0x7)) { //if _times==0 || _times>=8
		if(_times & 0x18) { //if times==8 || _times==16 || _times==24
			SET_FLAG(CF, _value & 1);
			SET_FLAG(OF, (_value & 1) ^ (_value >> 7));
		}
		return _value;
	}
	_times %= 8;

	m_instr->cycles.extra = _times;

	_value = (_value << _times) | (_value >> (8 - _times));

	SET_FLAG(CF, _value & 1);
	SET_FLAG(OF, (_value & 1) ^ (_value >> 7));

	return _value;
}

uint16_t CPUExecutor::ROL_w(uint16_t _value, uint8_t _times)
{
	if(!(_times & 0xF)) { //if _times==0 || _times>=15
		if(_times & 0x10) { //if _times==16 || _times==24
			SET_FLAG(CF, _value & 1);
			SET_FLAG(OF, (_value & 1) ^ (_value >> 15));
		}
		return _value;
	}
	_times %= 16;

	m_instr->cycles.extra = _times;

	_value = (_value << _times) | (_value >> (16 - _times));

	SET_FLAG(CF, _value & 1);
	SET_FLAG(OF, (_value & 1) ^ (_value >> 15));

	return _value;
}

void CPUExecutor::ROL_eb_ib() { store_eb(ROL_b(load_eb(), m_instr->ib)); }
void CPUExecutor::ROL_ew_ib() { store_ew(ROL_w(load_ew(), m_instr->ib)); }
void CPUExecutor::ROL_eb_1()  { store_eb(ROL_b(load_eb(), 1)); }
void CPUExecutor::ROL_ew_1()  { store_ew(ROL_w(load_ew(), 1)); }
void CPUExecutor::ROL_eb_CL() { store_eb(ROL_b(load_eb(), REG_CL)); }
void CPUExecutor::ROL_ew_CL() { store_ew(ROL_w(load_ew(), REG_CL)); }

uint8_t CPUExecutor::ROR_b(uint8_t _value, uint8_t _times)
{
	if(!(_times & 0x7)) { //if _times==0 || _times>=8
		if(_times & 0x18) { //if times==8 || _times==16 || _times==24
			SET_FLAG(CF, _value >> 7);
			SET_FLAG(OF, (_value >> 7) ^ ((_value >> 6) & 1));
		}
		return _value;
	}
	_times %= 8;

	m_instr->cycles.extra = _times;

	_value = (_value >> _times) | (_value << (8 - _times));

	SET_FLAG(CF, _value >> 7);
	SET_FLAG(OF, (_value >> 7) ^ ((_value >> 6) & 1));

	return _value;
}

uint16_t CPUExecutor::ROR_w(uint16_t _value, uint8_t _times)
{
	if(!(_times & 0xf)) { //if _times==0 || _times>=15
		if(_times & 0x10) { //if _times==16 || _times==24
			SET_FLAG(CF, _value >> 15);
			SET_FLAG(OF, (_value >> 15) ^ ((_value >> 14) & 1));
		}
		return _value;
	}
	_times %= 16;

	m_instr->cycles.extra = _times;

	_value = (_value >> _times) | (_value << (16 - _times));

	SET_FLAG(CF, _value >> 15);
	SET_FLAG(OF, (_value >> 15) ^ ((_value >> 14) & 1));

	return _value;
}

void CPUExecutor::ROR_eb_ib() { store_eb(ROR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::ROR_ew_ib() { store_ew(ROR_w(load_ew(), m_instr->ib)); }
void CPUExecutor::ROR_eb_1()  { store_eb(ROR_b(load_eb(), 1)); }
void CPUExecutor::ROR_ew_1()  { store_ew(ROR_w(load_ew(), 1)); }
void CPUExecutor::ROR_eb_CL() { store_eb(ROR_b(load_eb(), REG_CL)); }
void CPUExecutor::ROR_ew_CL() { store_ew(ROR_w(load_ew(), REG_CL)); }

uint8_t CPUExecutor::RCL_b(uint8_t _value, uint8_t _times)
{
	_times = (_times & 0x1F) % 9;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint8_t res;
	uint8_t cf = FLAG_CF;

	if(_times == 1) {
		res = (_value << 1) | cf;
	} else {
		res = (_value << _times) | (cf << (_times - 1)) | (_value >> (9 - _times));
	}

	cf = (_value >> (8-_times)) & 1;

	SET_FLAG(CF, cf);
	SET_FLAG(OF, cf ^ (res >> 7));

	return res;
}

uint16_t CPUExecutor::RCL_w(uint16_t _value, uint8_t _times)
{
	_times = (_times & 0x1F) % 17;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint16_t res;
	uint16_t cf = FLAG_CF;

	if(_times == 1) {
		res = (_value << 1) | cf;
	} else if(_times == 16) {
		res = (cf << 15) | (_value >> 1);
	} else { // 2..15
		res = (_value << _times) | (cf << (_times - 1)) | (_value >> (17 - _times));
	}

	cf = (_value >> (16-_times)) & 1;
	SET_FLAG(CF, cf);
	SET_FLAG(OF, cf ^ (res >> 15));

	return res;
}

void CPUExecutor::RCL_eb_ib() { store_eb(RCL_b(load_eb(), m_instr->ib)); }
void CPUExecutor::RCL_ew_ib() { store_ew(RCL_w(load_ew(), m_instr->ib)); }
void CPUExecutor::RCL_eb_1()  { store_eb(RCL_b(load_eb(), 1)); }
void CPUExecutor::RCL_ew_1()  { store_ew(RCL_w(load_ew(), 1)); }
void CPUExecutor::RCL_eb_CL() { store_eb(RCL_b(load_eb(), REG_CL)); }
void CPUExecutor::RCL_ew_CL() { store_ew(RCL_w(load_ew(), REG_CL)); }

uint8_t CPUExecutor::RCR_b(uint8_t _value, uint8_t _times)
{
	_times = (_times & 0x1F) % 9;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint8_t cf = FLAG_CF;
	uint8_t res = (_value >> _times) | (cf << (8-_times)) | (_value << (9-_times));

	cf = (_value >> (_times - 1)) & 1;
	SET_FLAG(CF, cf);
	SET_FLAG(OF, (res ^ (res << 1)) & 0x80);

	return res;
}

uint16_t CPUExecutor::RCR_w(uint16_t _value, uint8_t _times)
{
	_times = (_times & 0x1f) % 17;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint16_t cf = FLAG_CF;
	uint16_t res = (_value >> _times) | (cf << (16-_times)) | (_value << (17-_times));

	cf = (_value >> (_times - 1)) & 1;
	SET_FLAG(CF, cf);
	SET_FLAG(OF, (res ^ (res << 1)) & 0x8000);

	return res;
}

void CPUExecutor::RCR_eb_ib() { store_eb(RCR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::RCR_ew_ib() { store_ew(RCR_w(load_ew(), m_instr->ib)); }
void CPUExecutor::RCR_eb_1()  { store_eb(RCR_b(load_eb(), 1)); }
void CPUExecutor::RCR_ew_1()  { store_ew(RCR_w(load_ew(), 1)); }
void CPUExecutor::RCR_eb_CL() { store_eb(RCR_b(load_eb(), REG_CL)); }
void CPUExecutor::RCR_ew_CL() { store_ew(RCR_w(load_ew(), REG_CL)); }


/*******************************************************************************
 * RET-Return from Procedure
 */

void CPUExecutor::RET_near()
{
	uint16_t return_IP = stack_pop_word();

	if(return_IP > REG_CS.desc.limit) {
		PDEBUGF(LOG_V2, LOG_CPU, "RET_near: offset outside of CS limits\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SET_IP(return_IP);
	REG_SP += m_instr->iw1; // pop bytes

	g_cpubus.invalidate_pq();
}

void CPUExecutor::RET_far()
{
	uint16_t popbytes = m_instr->iw1;

	if(IS_PMODE()) {
		return_pmode(popbytes);
		return;
	}

	uint16_t ip     = stack_pop_word();
	uint16_t cs_raw = stack_pop_word();

	// CS.LIMIT can't change when in real mode
	if(ip > REG_CS.desc.limit) {
		PDEBUGF(LOG_V2, LOG_CPU,
				"RET_far: instruction pointer not within code segment limits\n");
		throw CPUException(CPU_GP_EXC, 0);
	}

	SET_CS(cs_raw);
	SET_IP(ip);
	REG_SP += popbytes;

	g_cpubus.invalidate_pq();
}


/*******************************************************************************
 * SAHF-Store AH into Flags
 */

void CPUExecutor::SAHF()
{
	uint16_t ah = REG_AH;
	SET_FLAG(SF, ah & FMASK_SF);
	SET_FLAG(ZF, ah & FMASK_ZF);
	SET_FLAG(AF, ah & FMASK_AF);
	SET_FLAG(PF, ah & FMASK_PF);
	SET_FLAG(CF, ah & FMASK_CF);
}


/*******************************************************************************
 * SALC-Set AL If Carry
 */

void CPUExecutor::SALC()
{
	//http://www.rcollins.org/secrets/opcodes/SALC.html
	PDEBUGF(LOG_V1, LOG_CPU, "SALC: undocumented opcode\n");
	if(FLAG_CF) {
		REG_AL = 0xFF;
	} else {
		REG_AL = 0;
	}
}


/*******************************************************************************
 * SAL/SAR/SHL/SHR-Shift Instructions
 */

uint8_t CPUExecutor::SHL_b(uint8_t _value, uint8_t _times)
{
	_times &= 0x1F;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint of = 0, cf = 0;
	uint8_t res;

	if(_times <= 8) {
		res = (_value << _times);
		cf = (_value >> (8 - _times)) & 0x1;
		of = cf ^ (res >> 7);
	} else {
		res = 0;
	}

	SET_FLAG(OF, of);
	SET_FLAG(CF, cf);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(SF, res & 0x80);

	return res;
}

uint16_t CPUExecutor::SHL_w(uint16_t _value, uint8_t _times)
{
	_times &= 0x1F;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint16_t res;
	uint of = 0, cf = 0;

	if(_times <= 16) {
		res = (_value << _times);
		cf = (_value >> (16 - _times)) & 0x1;
		of = cf ^ (res >> 15);
	} else {
		res = 0;
	}

	SET_FLAG(OF, of);
	SET_FLAG(CF, cf);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(SF, res & 0x8000);

	return res;
}

void CPUExecutor::SAL_eb_ib() { store_eb(SHL_b(load_eb(), m_instr->ib)); }
void CPUExecutor::SAL_ew_ib() { store_ew(SHL_w(load_ew(), m_instr->ib));  }
void CPUExecutor::SAL_eb_1()  { store_eb(SHL_b(load_eb(), 1)); }
void CPUExecutor::SAL_ew_1()  { store_ew(SHL_w(load_ew(), 1)); }
void CPUExecutor::SAL_eb_CL() { store_eb(SHL_b(load_eb(), REG_CL)); }
void CPUExecutor::SAL_ew_CL() { store_ew(SHL_w(load_ew(), REG_CL)); }

uint8_t CPUExecutor::SHR_b(uint8_t _value, uint8_t _times)
{
	_times &= 0x1f;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint8_t res = _value >> _times;

	SET_FLAG(OF, (((res << 1) ^ res) >> 7) & 0x1);
	SET_FLAG(CF, (_value >> (_times - 1)) & 0x1);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(SF, res & 0x80);

	return res;
}

uint16_t CPUExecutor::SHR_w(uint16_t _value, uint8_t _times)
{
	_times &= 0x1f;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint16_t res = _value >> _times;

	SET_FLAG(OF, ((uint16_t)((res << 1) ^ res) >> 15) & 0x1);
	SET_FLAG(CF, (_value >> (_times - 1)) & 1);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(SF, res & 0x8000);

	return res;
}

void CPUExecutor::SHR_eb_ib() { store_eb(SHR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::SHR_ew_ib() { store_ew(SHR_w(load_ew(), m_instr->ib)); }
void CPUExecutor::SHR_eb_1()  { store_eb(SHR_b(load_eb(), 1)); }
void CPUExecutor::SHR_ew_1()  { store_ew(SHR_w(load_ew(), 1)); }
void CPUExecutor::SHR_eb_CL() { store_eb(SHR_b(load_eb(), REG_CL)); }
void CPUExecutor::SHR_ew_CL() { store_ew(SHR_w(load_ew(), REG_CL)); }

uint8_t CPUExecutor::SAR_b(uint8_t _value, uint8_t _times)
{
	_times &= 0x1F;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = _times;

	uint8_t res = ((int8_t) _value) >> _times;

	SET_FLAG(OF, false);
	SET_FLAG(CF, (((int8_t) _value) >> (_times - 1)) & 1);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(PF, PARITY(res));

	return res;
}

uint16_t CPUExecutor::SAR_w(uint16_t _value, uint8_t _times)
{
	_times &= 0x1F;

	if(_times == 0) {
		return _value;
	}

	m_instr->cycles.extra = (_value)?_times:0;

	uint16_t res = ((int16_t) _value) >> _times;

	SET_FLAG(OF, false);
	SET_FLAG(CF, (((int16_t) _value) >> (_times - 1)) & 1);

	SET_FLAG(ZF, res == 0);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(PF, PARITY(res));

	return res;
}

void CPUExecutor::SAR_eb_ib() { store_eb(SAR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::SAR_ew_ib() { store_ew(SAR_w(load_ew(), m_instr->ib)); }
void CPUExecutor::SAR_eb_1()  { store_eb(SAR_b(load_eb(), 1)); }
void CPUExecutor::SAR_ew_1()  { store_ew(SAR_w(load_ew(), 1)); }
void CPUExecutor::SAR_eb_CL() { store_eb(SAR_b(load_eb(), REG_CL)); }
void CPUExecutor::SAR_ew_CL() { store_ew(SAR_w(load_ew(), REG_CL)); }


/*******************************************************************************
 * SBB-Integer Subtraction With Borrow
 */

uint8_t CPUExecutor::SBB_b(uint8_t op1, uint8_t op2)
{
	uint8_t cf = FLAG_CF;
	uint8_t res = op1 - (op2 + cf);

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, (op1 < res) || (cf && (op2==0xff)));

	return res;
}

uint16_t CPUExecutor::SBB_w(uint16_t op1, uint16_t op2)
{
	uint16_t cf = FLAG_CF;
	uint16_t res = op1 - (op2 + cf);

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, (op1 < res) || (cf && (op2==0xffff)));

	return res;
}

void CPUExecutor::SBB_eb_rb() { store_eb(SBB_b(load_eb(), load_rb())); }
void CPUExecutor::SBB_ew_rw() { store_ew(SBB_w(load_ew(), load_rw())); }
void CPUExecutor::SBB_rb_eb() { store_rb(SBB_b(load_rb(), load_eb())); }
void CPUExecutor::SBB_rw_ew() { store_rw(SBB_w(load_rw(), load_ew())); }
void CPUExecutor::SBB_AL_ib() { REG_AL = SBB_b(REG_AL, m_instr->ib); }
void CPUExecutor::SBB_AX_iw() { REG_AX = SBB_w(REG_AX, m_instr->iw1); }
void CPUExecutor::SBB_eb_ib() { store_eb(SBB_b(load_eb(), m_instr->ib)); }
void CPUExecutor::SBB_ew_iw() { store_ew(SBB_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::SBB_ew_ib() { store_ew(SBB_w(load_ew(), int8_t(m_instr->ib))); }


/*******************************************************************************
 * SCASB/SCASW-Compare String Data
 */

void CPUExecutor::SCASB()
{
	uint8_t op1 = REG_AL;
	//no segment override is possible.
	uint8_t op2 = read_byte(REG_ES, REG_DI);

	CMP_b(op1, op2);

	if(FLAG_DF) {
		REG_DI -= 1;
	} else {
		REG_DI += 1;
	}
}

void CPUExecutor::SCASW()
{
	uint16_t op1 = REG_AX;
	//no segment override is possible.
	uint16_t op2 = read_word(REG_ES, REG_DI);

	CMP_w(op1, op2);

	if(FLAG_DF) {
		REG_DI -= 2;
	} else {
		REG_DI += 2;
	}
}


/*******************************************************************************
 * SGDT/SIDT/SLDT-Store Global/Interrupt/Local Descriptor Table Register
 */

void CPUExecutor::SGDT()
{
	uint16_t limit_16 = GET_LIMIT(GDTR);
	uint32_t base_32  = GET_BASE(GDTR);

	SegReg & sr = (this->*EA_get_segreg)();
	uint16_t off = (this->*EA_get_offset)();

	write_word(sr, off, limit_16);
	write_word(sr, off+2, base_32);
	write_byte(sr, off+4, base_32>>16);

	/* Unlike to what described in the iAPX 286 Programmer's Reference Manual,
	 * the 80286 stores 1s in these upper bits. Windows 3.0 checks these bits to
	 * detect the 286 (the 386 stores 0s if the operand-size attribute is 16 bits.)
	 */
	write_byte(sr, off+5, 0xFF);
}

void CPUExecutor::SIDT()
{
	uint16_t limit_16 = GET_LIMIT(IDTR);
	uint32_t base_32  = GET_BASE(IDTR);

	SegReg & sr = (this->*EA_get_segreg)();
	uint16_t off = (this->*EA_get_offset)();

	write_word(sr, off, limit_16);
	write_word(sr, off+2, base_32);
	write_byte(sr, off+4, base_32>>16);

	// See comment for SGDT
	write_byte(sr, off+5, 0xFF);
}

void CPUExecutor::SLDT_ew()
{
	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "SLDT: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	uint16_t val16 = REG_LDTR.sel.value;
	store_ew(val16);
}


/*******************************************************************************
 * SMSW-Store Machine Status Word
 */

void CPUExecutor::SMSW_ew()
{
	uint16_t msw = GET_MSW();
	store_ew(msw);
}


/*******************************************************************************
 * STC/STD/STI-Set Carry/Direction/Interrupt Flag
 */

void CPUExecutor::STC()
{
	SET_FLAG(CF, true);
}

void CPUExecutor::STD()
{
	SET_FLAG(DF, true);
}

void CPUExecutor::STI()
{
	if(IS_PMODE() && (CPL > FLAG_IOPL)) {
		PDEBUGF(LOG_V2, LOG_CPU, "STI: CPL > IOPL in protected mode\n");
		throw CPUException(CPU_GP_EXC, 0);
    }
	if(!FLAG_IF) {
		SET_FLAG(IF, true);
		g_cpu.inhibit_interrupts(CPU_INHIBIT_INTERRUPTS);
	}
}


/*******************************************************************************
 * STOSB/STOSW-Store String Data
 */

void CPUExecutor::STOSB()
{
	//no segment override is possible.
	write_byte(REG_ES, REG_DI, REG_AL);

	if(FLAG_DF) {
		REG_DI -= 1;
	} else {
		REG_DI += 1;
	}
}

void CPUExecutor::STOSW()
{
	//no segment override is possible.
	write_word(REG_ES, REG_DI, REG_AX);

	if(FLAG_DF) {
		REG_DI -= 2;
	} else {
		REG_DI += 2;
	}
}


/*******************************************************************************
 * STR-Store Task Register
 */

void CPUExecutor::STR_ew()
{
	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "STR: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}
	uint16_t val = REG_TR.sel.value;
	store_ew(val);
}


/*******************************************************************************
 * SUB-Integer Subtraction
 */

uint8_t CPUExecutor::SUB_b(uint8_t op1, uint8_t op2)
{
	uint8_t res = op1 - op2;

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x80);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, op1 < op2);

	return res;
}

uint16_t CPUExecutor::SUB_w(uint16_t op1, uint16_t op2)
{
	uint16_t res = op1 - op2;

	SET_FLAG(OF, ((op1 ^ op2) & (op1 ^ res)) & 0x8000);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(AF, ((op1 ^ op2) ^ res) & 0x10);
	SET_FLAG(PF, PARITY(res));
	SET_FLAG(CF, op1 < op2);

	return res;
}

void CPUExecutor::SUB_eb_rb() { store_eb(SUB_b(load_eb(), load_rb())); }
void CPUExecutor::SUB_ew_rw() { store_ew(SUB_w(load_ew(), load_rw())); }
void CPUExecutor::SUB_rb_eb() { store_rb(SUB_b(load_rb(), load_eb())); }
void CPUExecutor::SUB_rw_ew() { store_rw(SUB_w(load_rw(), load_ew())); }
void CPUExecutor::SUB_AL_ib() { REG_AL = SUB_b(REG_AL, m_instr->ib); }
void CPUExecutor::SUB_AX_iw() { REG_AX = SUB_w(REG_AX, m_instr->iw1); }
void CPUExecutor::SUB_eb_ib() { store_eb(SUB_b(load_eb(), m_instr->ib)); }
void CPUExecutor::SUB_ew_iw() { store_ew(SUB_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::SUB_ew_ib() { store_ew(SUB_w(load_ew(), int8_t(m_instr->ib))); }


/*******************************************************************************
 * TEST-Logical Compare
 */

void CPUExecutor::TEST_b(uint8_t _value1, uint8_t _value2)
{
	uint8_t res = _value1 & _value2;

	SET_FLAG(OF, false);
	SET_FLAG(CF, false);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
}

void CPUExecutor::TEST_w(uint16_t _value1, uint16_t _value2)
{
	uint16_t res = _value1 & _value2;

	SET_FLAG(OF, false);
	SET_FLAG(CF, false);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));
}

void CPUExecutor::TEST_eb_rb() { TEST_b(load_eb(), load_rb()); }
void CPUExecutor::TEST_ew_rw() { TEST_w(load_ew(), load_rw()); }
void CPUExecutor::TEST_AL_ib() { TEST_b(REG_AL, m_instr->ib); }
void CPUExecutor::TEST_AX_iw() { TEST_w(REG_AX, m_instr->iw1); }
void CPUExecutor::TEST_eb_ib() { TEST_b(load_eb(), m_instr->ib); }
void CPUExecutor::TEST_ew_iw() { TEST_w(load_ew(), m_instr->iw1); }


/*******************************************************************************
 * VERR,VERW-Verify a Segment for Reading or Writing
 */

void CPUExecutor::VERR_ew()
{
	Descriptor descriptor;
	Selector   selector;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERR: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	selector = load_ew();

	/* if selector null, clear ZF and done */
	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERR: null selector\n");
	    SET_FLAG(ZF, false);
	    return;
	}

	try {
		descriptor = g_cpucore.fetch_descriptor(selector,0);
	} catch(CPUException &e) {
	    PDEBUGF(LOG_V2, LOG_CPU, "VERR: not within descriptor table\n");
	    SET_FLAG(ZF, false);
	    return;
	}

	/* If source selector is visible at CPL & RPL,
	 * within the descriptor table, and of type accepted by VERR instruction,
	 * then load register with segment limit and set ZF
	 */

	if(!descriptor.segment) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERR: system descriptor\n");
		SET_FLAG(ZF, false);
		return;
	}

	if(!descriptor.valid) {
	    PDEBUGF(LOG_V2, LOG_CPU, "VERR: valid bit cleared\n");
		SET_FLAG(ZF, false);
		return;
	}

	// normal data/code segment
	if(descriptor.is_code_segment()) {
		// ignore DPL for readable conforming segments
		if(descriptor.is_code_segment_conforming() && descriptor.is_code_segment_readable())
	    {
			PDEBUGF(LOG_V2, LOG_CPU, "VERR: conforming code, OK\n");
			SET_FLAG(ZF, true);
			return;
	    }
	    if(!descriptor.is_code_segment_readable()) {
	    	PDEBUGF(LOG_V2, LOG_CPU, "VERR: code not readable\n");
			SET_FLAG(ZF, false);
			return;
	    }
	    // readable, non-conforming code segment
	    if((descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl)) {
	    	PDEBUGF(LOG_V2, LOG_CPU, "VERR: non-conforming code not withing priv level\n");
	    	SET_FLAG(ZF, false);
	    } else {
	    	SET_FLAG(ZF, true);
	    }
	} else {
		// data segment
		if((descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl)) {
			PDEBUGF(LOG_V2, LOG_CPU, "VERR: data seg not withing priv level\n");
			SET_FLAG(ZF, false);
	    } else {
	    	SET_FLAG(ZF, true);
	    }
	}
}

void CPUExecutor::VERW_ew()
{
	Descriptor descriptor;
	Selector   selector;

	if(!IS_PMODE()) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: not recognized in real mode\n");
		throw CPUException(CPU_UD_EXC, 0);
	}

	selector = load_ew();

	/* if selector null, clear ZF and done */
	if((selector.value & SELECTOR_RPL_MASK) == 0) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: null selector\n");
		SET_FLAG(ZF, false);
		return;
	}

	/* If source selector is visible at CPL & RPL,
	 * within the descriptor table, and of type accepted by VERW instruction,
	 * then load register with segment limit and set ZF
	 */

	try {
		descriptor = g_cpucore.fetch_descriptor(selector,0);
	} catch(CPUException &e) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: not within descriptor table\n");
		SET_FLAG(ZF, false);
		return;
	}

	// rule out system segments & code segments
	if(!descriptor.segment || descriptor.is_code_segment()) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: system seg or code\n");
		SET_FLAG(ZF, false);
		return;
	}

	if(!descriptor.valid) {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: valid bit cleared\n");
		SET_FLAG(ZF, false);
		return;
	}

	// data segment
	if(descriptor.is_data_segment_writeable()) {
		if((descriptor.dpl < CPL) || (descriptor.dpl < selector.rpl)) {
			PDEBUGF(LOG_V2, LOG_CPU, "VERW: writable data seg not within priv level\n");
			SET_FLAG(ZF, false);
		} else {
			SET_FLAG(ZF, true);
		}
	} else {
		PDEBUGF(LOG_V2, LOG_CPU, "VERW: data seg not writable\n");
		SET_FLAG(ZF, false);
	}
}


/*******************************************************************************
 * WAIT-Wait Until BUSY Pin Is Inactive (HIGH)
 */

void CPUExecutor::WAIT()
{
/* TODO fpu support?
#NM if task switch flag in MSW is set. #MF if 80287 has detected an
unmasked numeric error.
*/
	//checks also MP
	if(CR0_TS && CR0_MP) {
		throw CPUException(CPU_NM_EXC, 0);
	}
}


/*******************************************************************************
 * XCHG-Exchange Memory/Register with Register
 */

void CPUExecutor::XCHG_eb_rb()
{
	uint8_t eb = load_eb();
	uint8_t rb = load_rb();
	store_eb(rb);
	store_rb(eb);
}

void CPUExecutor::XCHG_ew_rw()
{
	uint16_t ew = load_ew();
	uint16_t rw = load_rw();
	store_ew(rw);
	store_rw(ew);
}

void CPUExecutor::XCHG_AX_rw()
{
	uint16_t ax = REG_AX;
	REG_AX = GEN_REG(m_instr->reg).word[0];
	GEN_REG(m_instr->reg).word[0] = ax;
}


/*******************************************************************************
 * XLATB-Table Look-up Translation
 */

void CPUExecutor::XLATB()
{
	REG_AL = read_byte(SEG_REG(m_base_ds), (REG_BX + uint16_t(REG_AL)));
}


/*******************************************************************************
 * XOR-Logical Exclusive OR
 */

uint8_t CPUExecutor::XOR_b(uint8_t _op1, uint8_t _op2)
{
	uint8_t res = _op1 ^ _op2;

	SET_FLAG(CF, false);
	SET_FLAG(OF, false);
	SET_FLAG(SF, res & 0x80);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));

	return res;
}

uint16_t CPUExecutor::XOR_w(uint16_t _op1, uint16_t _op2)
{
	uint16_t res = _op1 ^ _op2;

	SET_FLAG(CF, false);
	SET_FLAG(OF, false);
	SET_FLAG(SF, res & 0x8000);
	SET_FLAG(ZF, res == 0);
	SET_FLAG(PF, PARITY(res));

	return res;
}

void CPUExecutor::XOR_rb_eb() { store_rb(XOR_b(load_rb(), load_eb())); }
void CPUExecutor::XOR_rw_ew() { store_rw(XOR_w(load_rw(), load_ew())); }
void CPUExecutor::XOR_eb_rb() { store_eb(XOR_b(load_eb(), load_rb())); }
void CPUExecutor::XOR_ew_rw() { store_ew(XOR_w(load_ew(), load_rw())); }
void CPUExecutor::XOR_AL_ib() { REG_AL = XOR_b(REG_AL, m_instr->ib); }
void CPUExecutor::XOR_AX_iw() { REG_AX = XOR_w(REG_AX, m_instr->iw1); }
void CPUExecutor::XOR_eb_ib() { store_eb(XOR_b(load_eb(), m_instr->ib)); }
void CPUExecutor::XOR_ew_iw() { store_ew(XOR_w(load_ew(), m_instr->iw1)); }
void CPUExecutor::XOR_ew_ib() { store_ew(XOR_w(load_ew(), int8_t(m_instr->ib))); }

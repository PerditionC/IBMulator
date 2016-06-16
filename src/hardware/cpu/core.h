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

#ifndef IBMULATOR_CPU_CORE_H
#define IBMULATOR_CPU_CORE_H

#include "selector.h"
#include "descriptor.h"
#include "statebuf.h"


class CPUCore;
extern CPUCore g_cpucore;

/* EFLAGS REGISTER:

              CARRY───S─────────────────────────────────────────┐
             PARITY───S───────────────────────────────────┐     │
   AUXILLIARY CARRY───S─────────────────────────────┐     │     │
               ZERO───S───────────────────────┐     │     │     │
               SIGN───S────────────────────┐  │     │     │     │
           OVERFLOW───S────────┐           │  │     │     │     │
                               │           │  │     │     │     │
    31     18 17 16 15 14 13 12│11 10  9  8│ 7│ 6  5│ 4  3│ 2  1│ 0
   ╔════════╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╤══╗
   ║ 0 .. 0 │VM│RF│ 0│NT│IO PL│OF│DF│IF│TF│SF│ZF│ 0│AF│ 0│PF│ 1│CF║
   ╚════════╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╧══╝
              │  │     │   │       │  │  │
              │  │     │   │       │  │  └───────S───TRAP FLAG
              │  │     │   │       │  └──────────X───INTERRUPT ENABLE
              │  │     │   │       └─────────────C───DIRECTION FLAG
              │  │     │   └─────────────────────X───I/O PRIVILEGE LEVEL
              │  │     └─────────────────────────X───NESTED TASK FLAG
              │  └───────────────────────────────X───RESUME FLAG (386)
              └──────────────────────────────────X───VIRTUAL 8086 MODE (386)

    S = STATUS FLAG, C = CONTROL FLAG, X = SYSTEM FLAG
*/
#define FBITN_CF   0
#define FBITN_PF   2
#define FBITN_AF   4
#define FBITN_ZF   6
#define FBITN_SF   7
#define FBITN_TF   8
#define FBITN_IF   9
#define FBITN_DF   10
#define FBITN_OF   11
#define FBITN_IOPL 12
#define FBITN_NT   14
#define FBITN_RF   16
#define FBITN_VM   17

#define FMASK_CF   (1 << FBITN_CF)  // 0 CARRY
#define FMASK_PF   (1 << FBITN_PF)  // 2 PARITY
#define FMASK_AF   (1 << FBITN_AF)  // 4 AUXILLIARY CARRY
#define FMASK_ZF   (1 << FBITN_ZF)  // 6 ZERO
#define FMASK_SF   (1 << FBITN_SF)  // 7 SIGN
#define FMASK_TF   (1 << FBITN_TF)  // 8 TRAP FLAG
#define FMASK_IF   (1 << FBITN_IF)  // 9 INTERRUPT ENABLE
#define FMASK_DF   (1 << FBITN_DF)  // 10 DIRECTION FLAG
#define FMASK_OF   (1 << FBITN_OF)  // 11 OVERFLOW
#define FMASK_IOPL (3 << FBITN_IOPL)// 12-13 I/O PRIVILEGE LEVEL
#define FMASK_NT   (1 << FBITN_NT)  // 14 NESTED TASK FLAG
#define FMASK_RF   (1 << FBITN_RF)  // 16 RESUME FLAG
#define FMASK_VM   (1 << FBITN_VM)  // 17 VIRTUAL 8086 MODE

#define FMASK_ALL   0x3FFFF
#define FMASK_VALID  0x00037FD5 // only supported bits for EFLAGS register

#define GET_FLAG(TYPE)     g_cpucore.get_FLAGS(FMASK_ ## TYPE)
#define SET_FLAG(TYPE,VAL) g_cpucore.set_##TYPE (VAL)
#define GET_FLAGS()        g_cpucore.get_FLAGS(FMASK_ALL)
#define SET_FLAGS(VAL)     g_cpucore.set_FLAGS(VAL)

#define FLAG_CF   (GET_FLAG(CF) >> FBITN_CF)
#define FLAG_PF   (GET_FLAG(PF) >> FBITN_PF)
#define FLAG_AF   (GET_FLAG(AF) >> FBITN_AF)
#define FLAG_ZF   (GET_FLAG(ZF) >> FBITN_ZF)
#define FLAG_SF   (GET_FLAG(SF) >> FBITN_SF)
#define FLAG_TF   (GET_FLAG(TF) >> FBITN_TF)
#define FLAG_IF   (GET_FLAG(IF) >> FBITN_IF)
#define FLAG_DF   (GET_FLAG(DF) >> FBITN_DF)
#define FLAG_OF   (GET_FLAG(OF) >> FBITN_OF)
#define FLAG_IOPL (GET_FLAG(IOPL) >> FBITN_IOPL)
#define FLAG_NT   (GET_FLAG(NT) >> FBITN_NT)
#define FLAG_RF   (GET_FLAG(RF) >> FBITN_RF)
#define FLAG_VM   (GET_FLAG(VM) >> FBITN_VM)

// Machine Status Word
#define MSW_PE (1 << 0)  // Protected mode enable
#define MSW_MP (1 << 1)  // Monitor processor extension
#define MSW_EM (1 << 2)  // Emulate processor extension
#define MSW_TS (1 << 3)  // Task switched

#define MSW_ALL 0xF
#define MSWMASK_VALID 0xF

#define GET_MSW     g_cpucore.get_MSW
#define SET_MSW		g_cpucore.set_MSW

#define IS_PMODE() GET_MSW(MSW_PE)


enum GenRegIndex8 {
	REGI_AL = 0,
	REGI_CL = 1,
	REGI_DL = 2,
	REGI_BL = 3,
	REGI_AH = 4,
	REGI_CH = 5,
	REGI_DH = 6,
	REGI_BH = 7
};

enum GenRegIndex32 {
	REGI_EAX = 0,
	REGI_ECX = 1,
	REGI_EDX = 2,
	REGI_EBX = 3,
	REGI_ESP = 4,
	REGI_EBP = 5,
	REGI_ESI = 6,
	REGI_EDI = 7
};

enum SegRegIndex {
	REGI_ES = 0,
	REGI_CS = 1,
	REGI_SS = 2,
	REGI_DS = 3,
	REGI_FS = 4,
	REGI_GS = 5
};

#define REGI_NONE 0xFF


#define REG_AL  g_cpucore.gen_reg(REGI_EAX).byte[0]
#define REG_AH  g_cpucore.gen_reg(REGI_EAX).byte[1]
#define REG_AX  g_cpucore.gen_reg(REGI_EAX).word[0]
#define REG_EAX g_cpucore.gen_reg(REGI_EAX).dword[0]
#define REG_BL  g_cpucore.gen_reg(REGI_EBX).byte[0]
#define REG_BH  g_cpucore.gen_reg(REGI_EBX).byte[1]
#define REG_BX  g_cpucore.gen_reg(REGI_EBX).word[0]
#define REG_EBX g_cpucore.gen_reg(REGI_EBX).dword[0]
#define REG_CL  g_cpucore.gen_reg(REGI_ECX).byte[0]
#define REG_CH  g_cpucore.gen_reg(REGI_ECX).byte[1]
#define REG_CX  g_cpucore.gen_reg(REGI_ECX).word[0]
#define REG_ECX g_cpucore.gen_reg(REGI_ECX).dword[0]
#define REG_DL  g_cpucore.gen_reg(REGI_EDX).byte[0]
#define REG_DH  g_cpucore.gen_reg(REGI_EDX).byte[1]
#define REG_DX  g_cpucore.gen_reg(REGI_EDX).word[0]
#define REG_EDX g_cpucore.gen_reg(REGI_EDX).dword[0]
#define REG_SI  g_cpucore.gen_reg(REGI_ESI).word[0]
#define REG_ESI g_cpucore.gen_reg(REGI_ESI).dword[0]
#define REG_DI  g_cpucore.gen_reg(REGI_EDI).word[0]
#define REG_EDI g_cpucore.gen_reg(REGI_EDI).dword[0]
#define REG_SP  g_cpucore.gen_reg(REGI_ESP).word[0]
#define REG_ESP g_cpucore.gen_reg(REGI_ESP).dword[0]
#define REG_BP  g_cpucore.gen_reg(REGI_EBP).word[0]
#define REG_EBP g_cpucore.gen_reg(REGI_EBP).dword[0]

#define REG_IP g_cpucore.get_IP()
#define SET_IP(value) g_cpucore.set_IP(value)
#define RESTORE_IP g_cpucore.restore_IP

#define GET_REG(NAME)     (g_cpucore.get_ ## NAME ())
#define SET_REG(NAME,VAL) (g_cpucore.set_ ## NAME (VAL))
#define SET_CS g_cpucore.set_CS
#define SET_SS g_cpucore.set_SS
#define SET_ES g_cpucore.set_ES
#define SET_DS g_cpucore.set_DS
#define SET_FS g_cpucore.set_FS
#define SET_GS g_cpucore.set_GS

#define SET_IDTR(base,limit) g_cpucore.set_IDTR(base,limit)
#define SET_GDTR(base,limit) g_cpucore.set_GDTR(base,limit)

#define REG_ES GET_REG(ES)
#define REG_DS GET_REG(DS)
#define REG_SS GET_REG(SS)
#define REG_CS GET_REG(CS)
#define REG_FS GET_REG(FS)
#define REG_GS GET_REG(GS)
#define REG_TR GET_REG(TR)
#define REG_LDTR GET_REG(LDTR)

#define SEG_REG(idx) g_cpucore.seg_reg(idx)
#define GEN_REG(idx) g_cpucore.gen_reg(idx)

#define GET_BASE(S)	 g_cpucore.get_ ## S ## _base()
#define GET_LIMIT(S) g_cpucore.get_ ## S ## _limit()

#define GET_PHYADDR(SEG,OFF) g_cpucore.get_ ## SEG ## _phyaddr(OFF)

#define IP_CHAIN_SIZE 10

#define CPL g_cpucore.get_CPL()

union GenReg
{
	uint32_t dword[1];
	uint16_t word[2];
	uint8_t  byte[4];
};

struct SegReg
{
	Selector sel;
	Descriptor desc;

	inline bool is(const SegReg & _segreg) {
		return (this==&_segreg);
	}

	void validate();
	const char *to_string();
};


class CPUCore
{
protected:

	// general registers
	GenReg m_genregs[8];

	// segment registers
	SegReg m_segregs[6];
	SegReg m_tr, m_ldtr;
	uint32_t m_idtr_base;
	uint16_t m_idtr_limit;
	uint32_t m_gdtr_base;
	uint16_t m_gdtr_limit;

	// status and control registers
	uint32_t m_flags;
	uint16_t m_ip, m_prev_ip, m_msw;


	inline void load_segment_register(SegReg & _segreg, uint16_t _value)
	{
		if(is_pmode()) {
			load_segment_protected(_segreg, _value);
		} else {
			load_segment_real(_segreg, _value, false);
		}
	}
	void load_segment_real(SegReg & _segreg, uint16_t _value, bool _defaults);
	void load_segment_protected(SegReg & _segreg, uint16_t _value);

	inline void set_FLAGS(uint8_t _flagnum, bool _val) {
		m_flags = (m_flags &~ (1<<_flagnum)) | ((_val)<<_flagnum);
	}

public:

	void reset();

	uint64_t fetch_descriptor(Selector & _selector, uint8_t _exc_vec) const;
	void touch_segment(Selector & _selector, Descriptor & _descriptor) const;


	inline GenReg & gen_reg(uint8_t idx) { assert(idx<8); return m_genregs[idx]; }
	inline SegReg & seg_reg(uint8_t idx) { assert(idx<6); return m_segregs[idx]; }

	//only real mode:
	inline void set_CS(uint16_t _val) {
		assert(!IS_PMODE());
		load_segment_real(m_segregs[REGI_CS], _val, true);
	}
	//only protected mode
	void set_CS(Selector &sel, Descriptor &desc, uint8_t cpl);
	static void check_CS(uint16_t selector, Descriptor &descriptor, uint8_t rpl, uint8_t cpl);
	static void check_CS(Selector &selector, Descriptor &descriptor, uint8_t rpl, uint8_t cpl)
		{ CPUCore::check_CS(selector.value, descriptor, rpl, cpl); }
	inline void set_DS(uint16_t _val) { load_segment_register(m_segregs[REGI_DS], _val); }
	inline void set_SS(uint16_t _val) { load_segment_register(m_segregs[REGI_SS], _val); }
	void set_SS(Selector &sel, Descriptor &desc, uint8_t cpl);
	inline void set_ES(uint16_t _val) { load_segment_register(m_segregs[REGI_ES], _val); }
	inline void set_FS(uint16_t _val) { load_segment_register(m_segregs[REGI_FS], _val); }
	inline void set_GS(uint16_t _val) { load_segment_register(m_segregs[REGI_GS], _val); }

	inline void set_IDTR(uint32_t _base, uint16_t _limit)
		{ m_idtr_base = _base; m_idtr_limit = _limit; }
	inline void set_GDTR(uint32_t _base, uint16_t _limit)
		{ m_gdtr_base = _base; m_gdtr_limit = _limit; }

	inline SegReg & get_CS() { return m_segregs[REGI_CS]; }
	inline SegReg & get_DS() { return m_segregs[REGI_DS]; }
	inline SegReg & get_SS() { return m_segregs[REGI_SS]; }
	inline SegReg & get_ES() { return m_segregs[REGI_ES]; }
	inline SegReg & get_FS() { return m_segregs[REGI_FS]; }
	inline SegReg & get_GS() { return m_segregs[REGI_GS]; }
	inline SegReg & get_TR() { return m_tr; }
	inline SegReg & get_LDTR() { return m_ldtr; }

	inline void set_IP(uint16_t _val) {
		m_prev_ip = m_ip;
		m_ip = _val;
	}
	inline uint16_t get_IP() const { return m_ip; }
	inline void restore_IP() { m_ip = m_prev_ip; }

	inline uint32_t get_FLAGS(uint32_t _mask) const { return (m_flags & _mask); }

	       void set_FLAGS(uint32_t _val);
	inline void set_CF(bool _val) { set_FLAGS(FBITN_CF,_val); }
	inline void set_PF(bool _val) { set_FLAGS(FBITN_PF,_val); }
	inline void set_AF(bool _val) { set_FLAGS(FBITN_AF,_val); }
	inline void set_ZF(bool _val) { set_FLAGS(FBITN_ZF,_val); }
	inline void set_SF(bool _val) { set_FLAGS(FBITN_SF,_val); }
	       void set_TF(bool _val);
	       void set_IF(bool _val);
	inline void set_DF(bool _val) { set_FLAGS(FBITN_DF,_val); }
	inline void set_OF(bool _val) { set_FLAGS(FBITN_OF,_val); }
	inline void set_IOPL(uint16_t _val) {
		m_flags &= ~(3 << 12);
		m_flags |= ((3 & _val) << 12);
	}
	inline void set_NT(bool _val) { set_FLAGS(FBITN_NT,_val); }

	inline void set_MSW(uint8_t _flagnum, bool _val) {
		m_msw = (m_msw &~ (1<<_flagnum)) | ((_val)<<_flagnum);
	}
	inline void set_MSW(uint16_t _msw) {
		m_msw = _msw & MSWMASK_VALID;
	}
	inline uint16_t get_MSW(uint16_t _msw) const {
		return (m_msw & _msw);
	}

	inline bool is_pmode() const { return m_msw & MSW_PE; }
	/*
	 * From the 80286 to the Pentium, all Intel processors derive their current
	 * privilege level (CPL) from the SS access rights. The CPL is loaded from
	 * the SS descriptor table entry when the SS register is loaded.
	 * The undocumented LOADALL instruction (or system-management mode RSM
	 * instruction) can be used to manipulate the SS descriptor-cache access
	 * rights, thereby directly manipulating the CPL of the microprocessors.
	 * (http://www.rcollins.org/ddj/Aug98/Aug98.html)
	 */
	inline uint8_t & get_CPL() { return m_segregs[REGI_CS].sel.cpl; }

	inline uint32_t get_CS_base() const { return m_segregs[REGI_CS].desc.base; }
	inline uint32_t get_DS_base() const { return m_segregs[REGI_DS].desc.base; }
	inline uint32_t get_SS_base() const { return m_segregs[REGI_SS].desc.base; }
	inline uint32_t get_ES_base() const { return m_segregs[REGI_ES].desc.base; }
	inline uint32_t get_TR_base() const { return m_tr.desc.base; }
	inline uint32_t get_LDTR_base() const { return m_ldtr.desc.base; }
	inline uint32_t get_IDTR_base() const { return m_idtr_base; }
	inline uint32_t get_GDTR_base() const { return m_gdtr_base; }

	inline uint32_t get_CS_phyaddr(uint16_t _offset) const { return get_CS_base() + _offset; }
	inline uint32_t get_DS_phyaddr(uint16_t _offset) const { return get_DS_base() + _offset; }
	inline uint32_t get_SS_phyaddr(uint16_t _offset) const { return get_SS_base() + _offset; }
	inline uint32_t get_ES_phyaddr(uint16_t _offset) const { return get_ES_base() + _offset; }

	inline uint16_t get_CS_limit() const { return m_segregs[REGI_CS].desc.limit; }
	inline uint16_t get_DS_limit() const { return m_segregs[REGI_DS].desc.limit; }
	inline uint16_t get_SS_limit() const { return m_segregs[REGI_SS].desc.limit; }
	inline uint16_t get_ES_limit() const { return m_segregs[REGI_ES].desc.limit; }
	inline uint16_t get_TR_limit() const { return m_tr.desc.limit; }
	inline uint16_t get_LDTR_limit() const { return m_ldtr.desc.limit; }
	inline uint16_t get_IDTR_limit() const { return m_idtr_limit; }
	inline uint16_t get_GDTR_limit() const { return m_gdtr_limit; }

	inline uint8_t  get_AL() const  { return m_genregs[REGI_EAX].byte[0]; }
	inline uint8_t  get_AH() const  { return m_genregs[REGI_EAX].byte[1]; }
	inline uint16_t get_AX() const  { return m_genregs[REGI_EAX].word[0]; }
	inline uint32_t get_EAX() const { return m_genregs[REGI_EAX].dword[0]; }
	inline uint8_t  get_BL() const  { return m_genregs[REGI_EBX].byte[0]; }
	inline uint8_t  get_BH() const  { return m_genregs[REGI_EBX].byte[1]; }
	inline uint16_t get_BX() const  { return m_genregs[REGI_EBX].word[0]; }
	inline uint32_t get_EBX() const { return m_genregs[REGI_EBX].dword[0]; }
	inline uint8_t  get_CL() const  { return m_genregs[REGI_ECX].byte[0]; }
	inline uint8_t  get_CH() const  { return m_genregs[REGI_ECX].byte[1]; }
	inline uint16_t get_CX() const  { return m_genregs[REGI_ECX].word[0]; }
	inline uint32_t get_ECX() const { return m_genregs[REGI_ECX].dword[0]; }
	inline uint8_t  get_DL() const  { return m_genregs[REGI_EDX].byte[0]; }
	inline uint8_t  get_DH() const  { return m_genregs[REGI_EDX].byte[1]; }
	inline uint16_t get_DX() const  { return m_genregs[REGI_EDX].word[0]; }
	inline uint32_t get_EDX() const { return m_genregs[REGI_EDX].dword[0]; }
	inline uint16_t get_SI() const  { return m_genregs[REGI_ESI].word[0]; }
	inline uint32_t get_ESI() const { return m_genregs[REGI_ESI].dword[0]; }
	inline uint16_t get_BP() const  { return m_genregs[REGI_EBP].word[0]; }
	inline uint32_t get_EBP() const { return m_genregs[REGI_EBP].dword[0]; }
	inline uint16_t get_DI() const  { return m_genregs[REGI_EDI].word[0]; }
	inline uint32_t get_EDI() const { return m_genregs[REGI_EDI].dword[0]; }
	inline uint16_t get_SP() const  { return m_genregs[REGI_ESP].word[0]; }
	inline uint32_t get_ESP() const { return m_genregs[REGI_ESP].dword[0]; }

	void save_state(StateBuf &_state) const;
	void restore_state(StateBuf &_state);
};


#endif

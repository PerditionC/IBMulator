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

#ifndef IBMULATOR_HW_CPU_H
#define IBMULATOR_HW_CPU_H

#include "cpu/core.h"
#include "cpu/decoder.h"
#include "cpu/state.h"
#include "cpu/logger.h"
#include <regex>

class CPU;
extern CPU g_cpu;

enum CPUFamily {
	CPU_286 = 2,
	CPU_386 = 3,

	CPU_COUNT = 2
};

/* The names of CPU interrupts reflect those reported in the 80286 programmers's
 * reference manual. Some codes have the same value but different names because
 * of this.
 *
 * Interrupts and exceptions are special cases of control transfer within a
 * program. An interrupt occurs as a result of an event that is independent of
 * the currently executing program, while exceptions are a direct result of the
 * program currently being executed, Interrupts may be external or internal.
 * External interrupts are generated by either the INTR or NMI input pins.
 * Internal interrupts are caused by the INT instruction. Exceptions occur when
 * an instruction cannot be completed normally. (cfr. 9-1)
 *
 */
enum CPUInterrupt {
	CPU_DIV_ER_EXC      = 0,  // Divide Error exception
	CPU_SINGLE_STEP_INT = 1,  // Single step interrupt
	CPU_NMI_INT         = 2,  // NMI interrupt
	CPU_BREAKPOINT_INT  = 3,  // Breakpoint interrupt
	CPU_INTO_EXC        = 4,  // INTO detected overflow exception
	CPU_BOUND_EXC       = 5,  // BOUND range exceeded exception
	CPU_UD_EXC          = 6,  // Undefined opcode exception (rmode/pmode)
	CPU_NM_EXC          = 7,  // NPX not available exception (rmode/pmode)
	CPU_IDT_LIMIT_EXC   = 8,  // Interrupt table limit too small exception (rmode)
	CPU_DF_EXC          = 8,  // Double Fault exception (pmode)
	CPU_NPX_SEG_OVR_INT = 9,  // NPX segment overrun interrupt (rmode)
	CPU_MP_EXC          = 9,  // NPX protection fault exception (pmode)
	CPU_TS_EXC          = 10, // Invalid Task State Segment exception (pmode)
	CPU_NP_EXC          = 11, // Segment Not Present exception (pmode)
	CPU_SS_EXC          = 12, // Stack Fault exception (pmode)
	CPU_SEG_OVR_EXC     = 13, // Segment overrun exception (rmode)
	CPU_GP_EXC          = 13, // General Protection exception (pmode)
	CPU_PF_EXC          = 14, // Page fault (pmode)
	CPU_NPX_ERR_INT     = 16, // NPX error interrupt (rmode)
	CPU_MF_EXC          = 16, // Math Fault exception (pmode)

	CPU_MAX_INT,
	CPU_INVALID_INT = CPU_MAX_INT
};

#include "cpu/executor.h"

#define CPU_FAMILY     g_cpu.family()
#define CPU_SIGNATURE  g_cpu.signature()

/* Various known CPU signatures
Sig    Model       Step
-----------------------
0303   386 DX      B1
0305   386 DX      D0
0308   386 DX      D1/D2/E1
2304   386 SX      A0
2305   386 SX      D0
2308   386 SX      D1
43??   386 SL      ??
0400   486 DX      A1
0401   486 DX      Bx
0402   486 DX      C0
0404   486 DX      D0
0410   486 DX      cAx
0411   486 DX      cBx
0420   486 SX      A0
0433   486 DX2-66
*/

#define CPU_SIG_386SX 0x2300


#define CPU_EVENT_NMI           (1 << 0)
#define CPU_EVENT_PENDING_INTR  (1 << 1)

#define CPU_INHIBIT_INTERRUPTS  0x01
#define CPU_INHIBIT_DEBUG       0x02

#define CPU_INHIBIT_INTERRUPTS_BY_MOVSS \
	(CPU_INHIBIT_INTERRUPTS | CPU_INHIBIT_DEBUG)

// exception types for interrupt method
enum CPUInterruptType {
	CPU_EXTERNAL_INTERRUPT = 0,
	CPU_NMI = 2,
	CPU_HARDWARE_EXCEPTION = 3,  // all exceptions except #BP and #OF
	CPU_SOFTWARE_INTERRUPT = 4,
	CPU_PRIVILEGED_SOFTWARE_INTERRUPT = 5,
	CPU_SOFTWARE_EXCEPTION = 6
};

class CPUException
{
public:
	uint8_t vector;
	uint16_t error_code;

	CPUException(uint8_t _vector, uint16_t _error_code)
	: vector(_vector), error_code(_error_code) { }
};

class CPUShutdown : public std::runtime_error
{
public:
	CPUShutdown(const char *_what) : std::runtime_error(_what) {}
};

class CPU
{
protected:
	unsigned m_family;
	unsigned m_signature;
	uint32_t m_freq;
	uint32_t m_cycle_time;
	Instruction *m_instr;
	std::function<void(void)> m_shutdown_trap;

	CPUState m_s;

	void handle_async_event();
	void mask_event(uint32_t event);
	void signal_event(uint32_t _event);
	void clear_event(uint32_t _event);
	bool is_masked_event(uint32_t _event);
	bool is_pending(uint32_t _event);
	bool is_unmasked_event_pending(uint32_t _event);
	uint32_t unmasked_events_pending();
	void default_shutdown_trap() {}
	bool is_double_fault(uint8_t _first_vec, uint8_t _current_vec);
	bool interrupts_inhibited(unsigned mask);
	bool v86_redirect_interrupt(uint8_t _vector);

	void wait_for_event();

	CPULogger m_logger;
	std::string m_log_prg_name;
	std::regex m_log_prg_regex;

	unsigned get_execution_cycles(bool _memtx);
	unsigned get_io_cycles(unsigned _io_time);

public:

	CPU();
	~CPU();

	void init();
	void reset(uint _signal);
	void power_off();
	void config_changed();

	uint step();

	inline unsigned family() const { return m_family; }
	inline unsigned signature() const { return m_signature; }
	inline uint32_t get_freq() { return m_freq; }
	GCC_ATTRIBUTE(always_inline)
	inline uint32_t get_cycle_time_ns() { return m_cycle_time; }

	void interrupt(uint8_t _vector, unsigned _type,	bool _push_error, uint16_t _error_code);
	void clear_INTR();
	void raise_INTR();
	void deliver_NMI();
	void inhibit_interrupts(unsigned mask);
	void interrupt_mask_change();
	void unmask_event(uint32_t event);
	inline void set_async_event() { m_s.async_event = true; }
	inline void clear_inhibit_mask() { m_s.inhibit_mask = 0; }
	inline void clear_debug_trap() { m_s.debug_trap = false; }

	void set_HRQ(bool _val);
	bool get_HRQ() { return m_s.HRQ; }

	void set_shutdown_trap(std::function<void(void)> _fn);
	void enter_sleep_state(CPUActivityState _state);
	void exception(CPUException _exc);

	void write_log();
	void enable_prg_log(std::string _prg_name);
	void disable_prg_log();
	void DOS_program_launch(std::string _name);
	void DOS_program_start(std::string _name);
	void DOS_program_finish(std::string _name);

	void save_state(StateBuf &_state);
	void restore_state(StateBuf &_state);
};


#endif

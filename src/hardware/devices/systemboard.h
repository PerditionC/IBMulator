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

#ifndef IBMULATOR_HW_SYSTEM_BOARD_H
#define IBMULATOR_HW_SYSTEM_BOARD_H

#include "hardware/iodevice.h"
class Parallel;
class Serial;

class SystemBoard : public IODevice
{
	IODEVICE(SystemBoard, "System Board")

protected:
	struct {
		// Port 0x0094
		bool VGA_enable;   // bit 5, VGA enable/setup mode
		bool board_enable; // bit 7, system board enable/setup mode

		// POS register 2
		uint8_t POS_2;
		bool VGA_awake;    // bit 0, VGA sleep bit
		bool POS2_bit1;    // bit 1, unknown/unused (floppy enabled?)
		bool COM_enabled;  // bit 2, COM enabled
		bool COM_port;     // bit 3, COM port: COM1, COM2
		bool LPT_enabled;  // bit 4, LPT enabled
		uint8_t LPT_port;  // bit 5-6, LPT port: LPT1, LPT2, LPT3
		uint8_t LPT_mode;  // bit 7, LPT normal/extended mode

		// other POS registers
		uint8_t POS_3, POS_4, POS_5;

		//the POST code
		uint8_t POST;

		//Card Selected Feedback
		uint8_t CSF;
	} m_s;

	// config at program launch or when a new config file is loaded:
	uint8_t m_COM_port, m_LPT_port;

	// serial and parallel ports can be not installed
	Parallel *m_parallel;
	Serial *m_serial;

	virtual void update_POS2_state();
	virtual void update_POS3_state() {}
	virtual void update_POS4_state() {}
	virtual void update_POS5_state() {}
	virtual void update_board_state();
	virtual void update_state();

public:
	SystemBoard(Devices* _dev);
	~SystemBoard() {}

	void reset(unsigned type);
	void config_changed();
	uint16_t read(uint16_t _address, unsigned _io_len);
	void write(uint16_t _address, uint16_t _value, unsigned _io_len);

	void set_feedback();

	void save_state(StateBuf &_state);
	void restore_state(StateBuf &_state);
};





#endif

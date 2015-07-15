/*
 * 	Copyright (c) 2002-2014  The Bochs Project
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
#include "program.h"
#include "machine.h"
#include "hardware/devices.h"
#include "pic.h"
#include "cmos.h"
#include <cstring>
#include <fstream>

CMOS g_cmos;

// CMOS register definitions from the IBM PS/1 Technical Reference
#define  REG_SEC           0x00
#define  REG_SEC_ALARM     0x01
#define  REG_MIN           0x02
#define  REG_MIN_ALARM     0x03
#define  REG_HOUR          0x04
#define  REG_HOUR_ALARM    0x05
#define  REG_WEEK_DAY      0x06
#define  REG_MONTH_DAY     0x07
#define  REG_MONTH         0x08
#define  REG_YEAR          0x09
#define  REG_STAT_A        0x0a
#define  REG_STAT_B        0x0b
#define  REG_STAT_C        0x0c
#define  REG_STAT_D        0x0d
#define  REG_CENTURY_BYTE  0x37


uint8_t bcd_to_bin(uint8_t value, bool is_binary)
{
	if (is_binary)
		return value;
	else
		return ((value >> 4) * 10) + (value & 0x0f);
}

uint8_t bin_to_bcd(uint8_t value, bool is_binary)
{
	if (is_binary)
		return value;
	else
		return ((value  / 10) << 4) | (value % 10);
}


CMOS::CMOS()
{
	m_periodic_timer_index = NULL_TIMER_HANDLE;
	m_one_second_timer_index = NULL_TIMER_HANDLE;
	m_uip_timer_index = NULL_TIMER_HANDLE;
}

CMOS::~CMOS()
{
}

void CMOS::init()
{
	g_devices.register_read_handler(this, 0x70, 1);
	g_devices.register_read_handler(this, 0x71, 1);
	g_devices.register_write_handler(this, 0x70, 1);
	g_devices.register_write_handler(this, 0x71, 1);

	g_machine.register_irq(8, "CMOS RTC");

	m_periodic_timer_index = g_machine.register_timer(
			std::bind(&CMOS::periodic_timer,this),
			1000000,
			true, //continuous
			false, //not active
			"CMOS periodic");

	m_one_second_timer_index = g_machine.register_timer(
			std::bind(&CMOS::one_second_timer,this),
			1000000,
			true, //continuous
			false, //not active
			"CMOS one second");

	m_uip_timer_index = g_machine.register_timer(
			std::bind(&CMOS::uip_timer,this),
			244,
			false, // one-shot
			false, // not-active
			"CMOS uip");
}

void CMOS::config_changed()
{
	//nothing to do.
	//everything is processed at power on
}

void CMOS::reset(unsigned type)
{
	if(type==MACHINE_SOFT_RESET) {
		return;
	}

	if(type==MACHINE_POWER_ON) {
		memset(&m_s, 0, sizeof(m_s));

		m_s.timeval = time(NULL);

		std::string binfile = g_program.config().find_file(CMOS_SECTION, CMOS_IMAGE_FILE);
		std::ifstream fd(binfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
		if(!fd.is_open()) {
			PERRF(LOG_CMOS,"error opening CMOS image file '%s'\n", binfile.c_str());
			throw std::exception();
		}
		std::streampos size = fd.tellg();
		if(size != CMOS_SIZE) {
			PERRF(LOG_CMOS,"CMOS image file size must be %d\n", CMOS_SIZE);
			throw std::exception();
		}

		fd.seekg(0, std::ios::beg);
		fd.read((char*)(m_s.reg), CMOS_SIZE);

		if(fd.rdstate() & std::ifstream::failbit) {
			PERRF(LOG_CMOS,"error reading CMOS file\n");
			throw std::exception();
		}

		fd.close();

		PINFOF(LOG_V0,LOG_CMOS,"CMOS image read from '%s'\n", binfile.c_str());

		m_s.rtc_mode_12hour = ((m_s.reg[REG_STAT_B] & 0x02) == 0);
		m_s.rtc_mode_binary = ((m_s.reg[REG_STAT_B] & 0x04) != 0);

		if(g_program.config().get_bool(CMOS_SECTION, CMOS_IMAGE_RTC_INIT)) {
			update_timeval();
		} else {
			update_clock();
		}

		char *tmptime;
		while((tmptime = strdup(ctime(&(m_s.timeval)))) == NULL) {
			PERRF_ABORT(LOG_CMOS,"Out of memory\n");
		}
		tmptime[strlen(tmptime)-1] = '\0';
		PINFOF(LOG_V1,LOG_CMOS,"Setting initial clock to: %s (time0=%u)\n",
				tmptime, (uint32_t) m_s.timeval);
		free(tmptime);

		m_s.timeval_change = 0;
	}

	//MACHINE_HARD_RESET and POWER_ON
	m_s.cmos_mem_address = 0;

	// RESET affects the following registers:
	//  CRA: no effects
	//  CRB: bits 4,5,6 forced to 0
	//  CRC: bits 4,5,6,7 forced to 0
	//  CRD: no effects
	m_s.reg[REG_STAT_B] &= 0x8f;
	m_s.reg[REG_STAT_C] = 0;

	// One second timer for updating clock & alarm functions
	g_machine.activate_timer(m_one_second_timer_index, 1000000, true);

	// handle periodic interrupt rate select
	CRA_change();
}

void CMOS::save_state(StateBuf &_state)
{
	PINFOF(LOG_V1, LOG_CMOS, "saving state\n");

	StateHeader h;
	h.name = get_name();
	h.data_size = sizeof(m_s);
	_state.write(&m_s,h);
}

void CMOS::restore_state(StateBuf &_state)
{
	PINFOF(LOG_V1, LOG_CMOS, "restoring state\n");

	StateHeader h;
	h.name = get_name();
	h.data_size = sizeof(m_s);
	_state.read(&m_s,h);

	if(!g_program.config().get_bool(CMOS_SECTION, CMOS_IMAGE_RTC_INIT)) {
		//TODO DOS clock should be updated too
		m_s.timeval = time(NULL);
		update_clock();
		m_s.timeval_change = 0;
	}
}

void CMOS::power_off()
{
	// save CMOS to image file if requested.
	if(g_program.config().get_bool(CMOS_SECTION,CMOS_IMAGE_SAVE)) {
		save_image();
	} else {
		PINFOF(LOG_V0, LOG_CMOS, "CMOS not saved\n");
	}
}

void CMOS::save_image()
{
	std::string binfile = g_program.config().get_file(CMOS_SECTION, CMOS_IMAGE_FILE, FILE_TYPE_USER);
	std::ofstream fd(binfile.c_str(), std::ofstream::binary);
	if(!fd.is_open()) {
		PERRF(LOG_CMOS,"unable to open CMOS image file '%s' for writing\n", binfile.c_str());
	} else {
		fd.write((char*)m_s.reg, CMOS_SIZE);
		if(fd.rdstate() & std::ofstream::failbit) {
			PERRF(LOG_CMOS,"error writing CMOS image file '%s'\n", binfile.c_str());
		} else {
			PINFOF(LOG_V0, LOG_CMOS, "CMOS image saved to '%s'\n", binfile.c_str());
		}
		fd.close();
	}
}

void CMOS::CRA_change(void)
{
	uint8_t nibble, dcc;

	// Periodic Interrupt timer
	nibble =  m_s.reg[REG_STAT_A] & 0x0f;
	dcc = ( m_s.reg[REG_STAT_A] >> 4) & 0x07;
	if((nibble == 0) || ((dcc & 0x06) == 0)) {
		// No Periodic Interrupt Rate when 0, deactivate timer
		g_machine.deactivate_timer(m_periodic_timer_index);
		m_s.periodic_interval_usec = (uint32_t) -1; // max value
	} else {
		// values 0001b and 0010b are the same as 1000b and 1001b
		if (nibble <= 2)
			nibble += 7;
		m_s.periodic_interval_usec = (unsigned) (1000000.0L / (32768.0L / (1 << (nibble - 1))));

		// if Periodic Interrupt Enable bit set, activate timer
		if(m_s.reg[REG_STAT_B] & 0x40) {
			PDEBUGF(LOG_V1, LOG_CMOS, "periodic timer ENABLED\n");
			g_machine.activate_timer(m_periodic_timer_index, m_s.periodic_interval_usec, 1);
		} else {
			PDEBUGF(LOG_V1, LOG_CMOS, "periodic timer DISABLED\n");
			g_machine.deactivate_timer(m_periodic_timer_index);
		}
	}
}

uint16_t CMOS::read(uint16_t address, unsigned /*io_len*/)
{
	uint8_t ret8;

	PDEBUGF(LOG_V2, LOG_CMOS, "CMOS read of register 0x%02x\n", (unsigned)  m_s.cmos_mem_address);

	switch (address) {
		case 0x0070:
			// this register is write-only on most machines
			PDEBUGF(LOG_V2, LOG_CMOS, "CMOS read of index port 0x70. returning 0xff\n");
			return 0xff;
		case 0x0071:
			ret8 =  m_s.reg[m_s.cmos_mem_address];
			// all bits of Register C are cleared after a read occurs.
			if(m_s.cmos_mem_address == REG_STAT_C) {
				m_s.reg[REG_STAT_C] = 0x00;
				g_pic.lower_irq(8);
			}
			return ret8;

		default:
			PERRF(LOG_CMOS, "unsupported CMOS read, address=0x%04x!\n", (unsigned) address);
			return 0;
	}
}

void CMOS::write(uint16_t address, uint16_t value, unsigned /*io_len*/)
{
	PDEBUGF(LOG_V2, LOG_CMOS, "CMOS write to address: 0x%04x = 0x%02x\n", address, value);

	switch (address) {
		case 0x0070:
			m_s.cmos_mem_address = value & 0x7F;
			break;

		case 0x0071:
			switch (m_s.cmos_mem_address) {
				case REG_SEC:           // seconds
				case REG_MIN:           // minutes
				case REG_HOUR:          // hours
				case REG_WEEK_DAY:      // day of the week
				case REG_MONTH_DAY:     // day of the month
				case REG_MONTH:         // month
				case REG_YEAR:          // year
				case REG_CENTURY_BYTE:  // century
					m_s.reg[m_s.cmos_mem_address] = value;
					if(m_s.reg[REG_STAT_B] & 0x80) {
						m_s.timeval_change = 1;
					} else {
						update_timeval();
					}
					break;

				case REG_STAT_A: // Control Register A
					// bit 7: Update in Progress (read-only)
					//   1 = signifies time registers will be updated within 244us
					//   0 = time registers will not occur before 244us
					//   note: this bit reads 0 when CRB bit 7 is 1
					// bit 6..4: Divider Chain Control
					//   000 oscillator disabled
					//   001 oscillator disabled
					//   010 Normal operation (32.768 KHz time base)
					//   011 TEST
					//   100 TEST
					//   101 TEST
					//   110 Divider Chain RESET
					//   111 Divider Chain RESET
					// bit 3..0: Periodic Interrupt Rate Select
					//   0000 None
					//   0001 3.90625  ms
					//   0010 7.8125   ms
					//   0011 122.070  us
					//   0100 244.141  us
					//   0101 488.281  us
					//   0110 976.562  us (value after POST)
					//   0111 1.953125 ms
					//   1000 3.90625  ms
					//   1001 7.8125   ms
					//   1010 15.625   ms
					//   1011 31.25    ms
					//   1100 62.5     ms
					//   1101 125      ms
					//   1110 250      ms
					//   1111 500      ms

					PDEBUGF(LOG_V2, LOG_CMOS, "CMOS write status reg A: 0x%02x\n", value);

					unsigned dcc;
					dcc = (value >> 4) & 0x07;
					if((dcc & 0x06) == 0x06) {
						PINFOF(LOG_V2, LOG_CMOS, "CRA: divider chain RESET\n");
					} else if(dcc == 0x02) {
						PINFOF(LOG_V2, LOG_CMOS, "CRA: Normal operation (32.768 KHz time base)\n");
					} else if (dcc > 0x02) {
						PERRF_ABORT(LOG_CMOS, "CRA: divider chain control 0x%02x\n", dcc);
					}
					m_s.reg[REG_STAT_A] &= 0x80;
					m_s.reg[REG_STAT_A] |= (value & 0x7f);
					CRA_change();
					break;

				case REG_STAT_B: // Control Register B
					// bit 0: Daylight Savings Enable
					//   1 = enable daylight savings
					//   0 = disable daylight savings
					// bit 1: 24/12 hour mode
					//   1 = 24 hour format
					//   0 = 12 hour format
					// bit 2: Data Mode
					//   1 = binary format
					//   0 = BCD format
					// bit 3: "square wave enable"
					//   Not supported and always read as 0
					// bit 4: Update Ended Interrupt Enable
					//   1 = enable generation of update ended interrupt
					//   0 = disable
					// bit 5: Alarm Interrupt Enable
					//   1 = enable generation of alarm interrupt
					//   0 = disable
					// bit 6: Periodic Interrupt Enable
					//   1 = enable generation of periodic interrupt
					//   0 = disable
					// bit 7: Set mode
					//   1 = user copy of time is "frozen" allowing time registers
					//       to be accessed without regard for an occurance of an update
					//   0 = time updates occur normally

					PDEBUGF(LOG_V2, LOG_CMOS, "CMOS write status reg B: 0x%02x\n", value);
					if(value & 0x01) {
						PINFOF(LOG_V2,LOG_CMOS,"daylight savings unsupported\n");
					}

					value &= 0xf7; // bit3 always 0
					// Note: setting bit 7 clears bit 4
					if(value & 0x80)
						value &= 0xef;

					unsigned prev_CRB;
					prev_CRB =  m_s.reg[REG_STAT_B];
					m_s.reg[REG_STAT_B] = value;
					if((prev_CRB & 0x02) != (value & 0x02)) {
						m_s.rtc_mode_12hour = ((value & 0x02) == 0);
						update_clock();
					}
					if((prev_CRB & 0x04) != (value & 0x04)) {
						m_s.rtc_mode_binary = ((value & 0x04) != 0);
						update_clock();
					}
					if((prev_CRB & 0x40) != (value & 0x40)) {
						// Periodic Interrupt Enabled changed
						if(prev_CRB & 0x40) {
							// transition from 1 to 0, deactivate timer
							PDEBUGF(LOG_V2, LOG_CMOS, "periodic timer DEACTIVATED\n");
							g_machine.deactivate_timer(m_periodic_timer_index);
						} else {
							// transition from 0 to 1
							// if rate select is not 0, activate timer
							if((m_s.reg[REG_STAT_A] & 0x0f) != 0) {
								PDEBUGF(LOG_V2, LOG_CMOS, "periodic timer ACTIVATED\n");
								g_machine.activate_timer(m_periodic_timer_index, m_s.periodic_interval_usec, 1);
							}
						}
					}
					if((prev_CRB & 0x80) != (value & 0x80)) {
						if(prev_CRB & 0x80) {
							PDEBUGF(LOG_V2, LOG_CMOS, "RTC update ENABLE\n");
						} else {
							PDEBUGF(LOG_V2, LOG_CMOS, "RTC update DISABLE\n");
						}
					}
					if((prev_CRB >= 0x80) && (value < 0x80) &&  m_s.timeval_change) {
						update_timeval();
						m_s.timeval_change = 0;
					}
					break;

				case REG_STAT_C: // Control Register C
				case REG_STAT_D: // Control Register D
					PERRF(LOG_CMOS, "CMOS write to control register 0x%02x ignored (read-only)\n", m_s.cmos_mem_address);
					break;

				default:
					m_s.reg[m_s.cmos_mem_address] = value;
					break;
			}
			break;
	}
}

void CMOS::periodic_timer()
{
	// if periodic interrupts are enabled, trip IRQ 8, and
	// update status register C
	if(m_s.reg[REG_STAT_B] & 0x40) {
		m_s.reg[REG_STAT_C] |= 0xc0; // Interrupt Request, Periodic Int
		PDEBUGF(LOG_V2, LOG_CMOS, "Interrupt Request, Periodic Int\n");
		g_pic.raise_irq(8);
	}
}

void CMOS::one_second_timer()
{
	// divider chain reset - RTC stopped
	if((m_s.reg[REG_STAT_A] & 0x60) == 0x60) {
		PDEBUGF(LOG_V2, LOG_CMOS, "RTC not updated because divider chain reset\n");
		return;
	}

	// update internal time/date buffer
	m_s.timeval++;

	// Dont update CMOS user copy of time/date if CRB bit7 is 1
	// Nothing else do to
	if(m_s.reg[REG_STAT_B] & 0x80) {
		PDEBUGF(LOG_V2, LOG_CMOS, "RTC not updated because CRB bit7 is 1\n");
		return;
	}

	m_s.reg[REG_STAT_A] |= 0x80; // set UIP bit

	// UIP timer for updating clock & alarm functions
	g_machine.activate_timer(m_uip_timer_index, 244, 0);
}

void CMOS::uip_timer()
{
	update_clock();

	// if update interrupts are enabled, trip IRQ 8, and
	// update status register C
	if(m_s.reg[REG_STAT_B] & 0x10) {
		m_s.reg[REG_STAT_C] |= 0x90; // Interrupt Request, Update Ended
		PDEBUGF(LOG_V2, LOG_CMOS, "Interrupt Request, Update Ended\n");
		g_pic.raise_irq(8);
	}

	// compare CMOS user copy of time/date to alarm time/date here
	if(m_s.reg[REG_STAT_B] & 0x20) {
		// Alarm interrupts enabled
		bool alarm_match = true;
		if((m_s.reg[REG_SEC_ALARM] & 0xc0) != 0xc0) {
			// seconds alarm not in dont care mode
			if(m_s.reg[REG_SEC] !=  m_s.reg[REG_SEC_ALARM])
				alarm_match = false;
		}
		if((m_s.reg[REG_MIN_ALARM] & 0xc0) != 0xc0) {
			// minutes alarm not in dont care mode
			if(m_s.reg[REG_MIN] !=  m_s.reg[REG_MIN_ALARM])
				alarm_match = false;
		}
		if((m_s.reg[REG_HOUR_ALARM] & 0xc0) != 0xc0) {
			// hours alarm not in dont care mode
			if(m_s.reg[REG_HOUR] !=  m_s.reg[REG_HOUR_ALARM])
				alarm_match = false;
		}
		if(alarm_match) {
			m_s.reg[REG_STAT_C] |= 0xa0; // Interrupt Request, Alarm Int
			PDEBUGF(LOG_V2, LOG_CMOS, "Interrupt Request, Alarm Int\n");
			g_pic.raise_irq(8);
		}
	}
	m_s.reg[REG_STAT_A] &= 0x7f; // clear UIP bit
}

void CMOS::update_clock()
{
	struct tm *time_calendar;
	unsigned year, month, day, century;
	uint8_t val_bcd, hour;

	PDEBUGF(LOG_V2, LOG_CMOS, "RTC updating\n");

	time_calendar = localtime(&m_s.timeval);

	// update seconds
	m_s.reg[REG_SEC] = bin_to_bcd(time_calendar->tm_sec, m_s.rtc_mode_binary);

	// update minutes
	m_s.reg[REG_MIN] = bin_to_bcd(time_calendar->tm_min, m_s.rtc_mode_binary);

	// update hours
	if(m_s.rtc_mode_12hour) {
		hour = time_calendar->tm_hour;
		val_bcd = (hour > 11) ? 0x80 : 0x00;
		if (hour > 11) hour -= 12;
		if (hour == 0) hour = 12;
		val_bcd |= bin_to_bcd(hour,  m_s.rtc_mode_binary);
		m_s.reg[REG_HOUR] = val_bcd;
	} else {
		m_s.reg[REG_HOUR] = bin_to_bcd(time_calendar->tm_hour, m_s.rtc_mode_binary);
	}

	// update day of the week
	day = time_calendar->tm_wday + 1; // 0..6 to 1..7
	m_s.reg[REG_WEEK_DAY] = bin_to_bcd(day, m_s.rtc_mode_binary);

	// update day of the month
	day = time_calendar->tm_mday;
	m_s.reg[REG_MONTH_DAY] = bin_to_bcd(day, m_s.rtc_mode_binary);

	// update month
	month = time_calendar->tm_mon + 1;
	m_s.reg[REG_MONTH] = bin_to_bcd(month, m_s.rtc_mode_binary);

	// update year
	year = time_calendar->tm_year % 100;
	m_s.reg[REG_YEAR] = bin_to_bcd(year, m_s.rtc_mode_binary);

	// update century
	century = (time_calendar->tm_year / 100) + 19;
	m_s.reg[REG_CENTURY_BYTE] = bin_to_bcd(century, m_s.rtc_mode_binary);
}

void CMOS::update_timeval()
{
	struct tm time_calendar;
	uint8_t val_bin, pm_flag;

	// update seconds
	time_calendar.tm_sec = bcd_to_bin(m_s.reg[REG_SEC], m_s.rtc_mode_binary);

	// update minutes
	time_calendar.tm_min = bcd_to_bin(m_s.reg[REG_MIN], m_s.rtc_mode_binary);

	// update hours
	if(m_s.rtc_mode_12hour) {
		pm_flag =  m_s.reg[REG_HOUR] & 0x80;
		val_bin = bcd_to_bin(m_s.reg[REG_HOUR] & 0x70, m_s.rtc_mode_binary);
		if((val_bin < 12) & (pm_flag > 0)) {
			val_bin += 12;
		} else if ((val_bin == 12) & (pm_flag == 0)) {
			val_bin = 0;
		}
		time_calendar.tm_hour = val_bin;
	} else {
		time_calendar.tm_hour = bcd_to_bin(m_s.reg[REG_HOUR], m_s.rtc_mode_binary);
	}

	// update day of the month
	time_calendar.tm_mday = bcd_to_bin(m_s.reg[REG_MONTH_DAY], m_s.rtc_mode_binary);

	// update month
	time_calendar.tm_mon = bcd_to_bin(m_s.reg[REG_MONTH], m_s.rtc_mode_binary) - 1;

	// update year
	val_bin = bcd_to_bin(m_s.reg[REG_CENTURY_BYTE], m_s.rtc_mode_binary);
	val_bin = (val_bin - 19) * 100;
	val_bin += bcd_to_bin(m_s.reg[REG_YEAR], m_s.rtc_mode_binary);
	time_calendar.tm_year = val_bin;

	m_s.timeval = mktime(& time_calendar);
}



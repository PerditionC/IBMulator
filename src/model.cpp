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

#include "ibmulator.h"
#include "model.h"
#include "machine.h"
#include "hardware/devices/floppy.h"

bios_db_t g_bios_db = {
	{ "f605396b48f02c5e81bc9e5e5fb60717", {
		"1057756 (C) COPYRIGHT IBM CORPORATION 1981, 1989 ALL RIGHTS RESERVED",
		"PS/1 (LW-Type 44)",
		PS1_2011, PS1_2011_C34,
		"PS1_2011",
		0x4F8F
	} },
	{ "9cac91f1fa7fe58d9509b754785f7fd2", {
		"1057760 (C) COPYRIGHT IBM CORPORATION 1981, 1989 ALL RIGHTS RESERVED",
		"PS/1 Model 2011 (10 MHz 286)",
		PS1_2011, PS1_2011_C34,
		"PS1_2011",
		0x4CEF
	} },
	{ "159413f190f075b92ffb882331c70eaf", {
		"92F9674 (C) COPYRIGHT IBM CORPORATION 1981, 1991 ALL RIGHTS RESERVED",
		"PS/1 Model 2121 (16 MHz 386SX)",
		PS1_2121, PS1_2121_B82,
		"PS1_2121",
		0x3D4D
	} },
	{ "7b5f6e3803ee57fd95047738d36f12fd", {
		"92F9606 (C) COPYRIGHT IBM CORPORATION 1981, 1991 ALL RIGHTS RESERVED",
		"PS/1 Model 2121 (16 MHz 386SX)",
		PS1_2121, PS1_2121_B82,
		"PS1_2121",
		0x3D4D
	} },
	{ "01ae622ab197b057c92ad7832f868b4c", {
		"93F2455 (C) COPYRIGHT IBM CORPORATION 1981, 1991 ALL RIGHTS RESERVED",
		"PS/1 Model 2121 (20 MHz 386SX)",
		PS1_2121, PS1_2121_A82,
		"PS1_2121",
		0x0245
	} }
};

machine_db_t g_machine_db = {
//
// Model name     CPU     MHz  RAM   floppy A   Floppy B  Storage HDD
//                                                        ctrl    type
{ PS1_2011_C34, { "286",   10, 1024, FDD_350HD, FDD_NONE, "ps1",  35 } },
{ PS1_2121_B82, { "386SX", 16, 2048, FDD_350HD, FDD_NONE, "ata",  43 } },
{ PS1_2121_A82, { "386SX", 20, 2048, FDD_350HD, FDD_NONE, "ata",  43 } }
};

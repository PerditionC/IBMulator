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

#ifndef IBMULATOR_STATEBUF_H
#define IBMULATOR_STATEBUF_H

#include <string>

struct StateHeader
{
	size_t data_size;
	std::string name;

	size_t read(uint8_t *_source, size_t _source_size);
	size_t write(uint8_t *_dest, size_t _dest_size) const;
	bool check(uint8_t *_raw, size_t _raw_size) const;
	size_t get_size() const;
};

/* TODO working with binary blobs is not the best solution.
 * first of all it's not portable, and also it's error prone.
 * maybe I should use something like BSON
 */
class StateBuf
{
private:

	std::string m_basename;
	uint8_t *m_buf;
	size_t m_size;
	uint8_t *m_curptr;

public:

	bool m_last_restore;

	StateBuf(const std::string &_basename);
	~StateBuf();

	void write(const void *_data, const StateHeader &_header);
	void read(void *_data, const StateHeader &_header);
	void check(const StateHeader &_header);
	void seek(size_t _pos);
	void advance(size_t _off);
	void skip();
	void get_next_lump_header(StateHeader &);
	std::string get_basename() { return m_basename; }

	inline uint8_t * get_buf() { return m_curptr; }
	inline size_t get_size() { return m_size; }
	inline size_t get_bytesleft() {
		return m_size - (m_curptr - m_buf);
	}

	void load(const std::string &_path);
	void save(const std::string &_path);
};

#endif

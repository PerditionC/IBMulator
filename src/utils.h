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

#ifndef IBMULATOR_UTILS_H
#define IBMULATOR_UTILS_H

void str_replace_all(std::string &_str, const std::string &_search, const std::string &_replace);


#include <functional>
#include <chrono>
#include <future>

template <class callable, class... arguments>
	void timed_event(int _after_ms, callable&& _func, arguments&&... _args)
{
	std::function<typename std::result_of<callable(arguments...)>::type()>
		event(std::bind(std::forward<callable>(_func), std::forward<arguments>(_args)...));

	std::thread([_after_ms, event]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(_after_ms));
		event();
	}).detach();
}

#endif
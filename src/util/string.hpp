/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_XBOXDRV_UTIL_STRING_HPP
#define HEADER_XBOXDRV_UTIL_STRING_HPP

#include <functional>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

int hexstr2int(const std::string& str);
uint16_t hexstr2uint16(const std::string& str);

bool str2bool(std::string const& str);
int str2int(std::string const& str);
float str2float(std::string const& str);

std::string raw2str(const uint8_t* buffer, int len);
bool is_number(const std::string& str);
bool is_float(const std::string& str);

/** Convert the given string \a str to an integer, the string can
    either be an exact integer or a percent value (i.e. "75%"), in
    which case it is handled as (range * int(str)) */
int to_number(int range, const std::string& str);

/** Splits apart a string of the form "NAME=VALUE,..." and calls
    func(NAME, VALUE) on each.

    When VALUE is supposed to contain a "," the value has to be quoted
    with [], i.e. "NAME=[VALUE1,VALUE2]", the "[" and "]" itself can
    be quoted with "\[" and "\]" */
void process_name_value_string(const std::string& str, const std::function<void (const std::string&, const std::string&)>& func);

#endif

/* EOF */

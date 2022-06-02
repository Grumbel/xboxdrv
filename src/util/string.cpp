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

#include "util/string.hpp"

#include <fmt/format.h>
#include <strut/split.hpp>

#include "raise_exception.hpp"

namespace xboxdrv {

int hexstr2int(const std::string& str)
{
  unsigned int value = 0;
  if (sscanf(str.c_str(), "%x", &value) == 1)
  {
    return value;
  }
  else if (sscanf(str.c_str(), "0x%x", &value) == 1)
  {
    return value;
  }
  else
  {
    raise_exception(std::runtime_error, "couldn't convert '" << str << "' to int");
  }
}

uint16_t hexstr2uint16(const std::string& str)
{
  return static_cast<uint16_t>(hexstr2int(str));
}


bool str2bool(std::string const& str)
{
  try
  {
    int result = std::stoi(str);
    if (result == 1)
    {
      return true;
    }
    else if (result == 0)
    {
      return false;
    }
    else
    {
      throw std::runtime_error(fmt::format("str2bool(): couldn't convert '{}' to bool", str));
    }
  }
  catch(...)
  {
    std::throw_with_nested(std::runtime_error(fmt::format("str2bool(): couldn't convert '{}' to bool", str)));
  }
}

int str2int(std::string const& str)
{
  try
  {
    return std::stoi(str);
  }
  catch(...)
  {
    std::throw_with_nested(std::runtime_error(fmt::format("str2int(): couldn't convert '{}' to int", str)));
  }
}

float str2float(std::string const& str)
{
  try
  {
    return std::stof(str);
  }
  catch(...)
  {
    std::throw_with_nested(std::runtime_error(fmt::format("str2float(): couldn't convert '{}' to float", str)));
  }
}

std::string raw2str(const uint8_t* data, int len)
{
  std::ostringstream out;
  out << "len: " << len
      << " data: ";

  for(int i = 0; i < len; ++i)
    out << fmt::format("{:02x} ", int(data[i]));

  return out.str();
}

void process_name_value_string(const std::string& str, const std::function<void (const std::string&, const std::string&)>& func)
{
  int quote_count = 0;
  std::string res;
  for(std::string::size_type i = 0; i < str.size(); ++i)
  {
    if (str[i] == '[')
    {
      quote_count += 1;
    }
    else if (str[i] == ']')
    {
      quote_count -= 1;
      if (quote_count < 0)
      {
        raise_exception(std::runtime_error, "unexpected ']' at " << i);
      }
    }
    else if (str[i] == '\\')
    {
      i += 1;
      if (i < str.size())
      {
        res += str[i+1];
      }
      else
      {
        res += '\\';
      }
    }
    else if (str[i] == ',')
    {
      if (quote_count == 0)
      {
        if (!res.empty())
        {
          std::string lhs, rhs;
          strut::split_at(res, '=', &lhs, &rhs);
          func(lhs, rhs);

          res.clear();
        }
      }
      else
      {
        res += str[i];
      }
    }
    else
    {
      res += str[i];
    }
  }

  if (quote_count != 0)
  {
    raise_exception(std::runtime_error, "unclosed '['");
  }

  if (!res.empty())
  {
    std::string lhs, rhs;
    strut::split_at(res, '=', &lhs, &rhs);
    func(lhs, rhs);
  }
}

bool is_number(const std::string& str)
{
  for(std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    if (!isdigit(*i))
      return false;
  return true;
}

bool is_float(const std::string& str)
{
  for(std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    if (!isdigit(*i) && *i != '.')
      return false;
  return true;
}

int to_number(int range, const std::string& str)
{
  if (str.empty())
  {
    return 0;
  }
  else
  {
    if (str[str.size() - 1] == '%')
    {
      int percent = std::stoi(str.substr(0, str.size()-1));
      return range * percent / 100;
    }
    else
    {
      return str2int(str);
    }
  }
}

} // namespace xboxdrv

/* EOF */

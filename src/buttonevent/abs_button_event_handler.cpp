/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmail.com>
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

#include "abs_button_event_handler.hpp"

#include <assert.h>
#include <sstream>

#include <uinpp/multi_device.hpp>

namespace xboxdrv {

AbsButtonEventHandler*
AbsButtonEventHandler::from_string(uinpp::MultiDevice& uinput, int slot, bool extra_devices,
                                   std::string const& str)
{
  // FIXME: Need magic to detect min/max of the axis
  assert(false && "not implemented");
  return {};
}

AbsButtonEventHandler::AbsButtonEventHandler(uinpp::MultiDevice& uinput, int slot, bool extra_devices,
                                             int code) :
  m_code(uinpp::Event::invalid()),
  m_value(),
  m_abs_emitter()
{
  assert(false && "Not implemented");
  // FIXME: Need magic to detect min/max of the axis
}

void
AbsButtonEventHandler::send(bool value)
{
  if (value)
  {
    m_abs_emitter->send(m_value);
  }
}

std::string
AbsButtonEventHandler::str() const
{
  std::ostringstream out;
  out << "abs: " << m_code.get_device_id() << "-" << m_code.code << ":" << m_value;
  return out.str();
}

} // namespace xboxdrv

/* EOF */

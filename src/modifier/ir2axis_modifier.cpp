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

#include "ir2axis_modifier.hpp"

#include <stdexcept>
#include <math.h>

#include "raise_exception.hpp"

namespace xboxdrv {

IR2AxisModifier*
IR2AxisModifier::from_string(std::vector<std::string> const& args)
{
  if (args.size() != 2)
  {
    raise_exception(std::runtime_error, "two axis required as argument");
  }
  else
  {
    return new IR2AxisModifier(args[0], args[1]);
  }
}

IR2AxisModifier::IR2AxisModifier(std::string const& xaxis, std::string const& yaxis) :
  m_xaxis_str(xaxis),
  m_yaxis_str(yaxis),
  m_xaxis(-1),
  m_yaxis(-1)
{
}

void
IR2AxisModifier::init(ControllerMessageDescriptor& desc)
{
  m_xaxis = desc.abs().get(m_xaxis_str);
  m_yaxis = desc.abs().get(m_yaxis_str);
}

void
IR2AxisModifier::update(int msec_delta, ControllerMessage& msg, ControllerMessageDescriptor const& desc)
{
#if 0
  // find center of two biggest points, return that as axis values
  float x1 = 0.0f;
  float y1 = 0.0f;
  int size1 = 0;

  float x2 = 0.0f;
  float y2 = 0.0f;
  int size2 = 0;

  bool valid1 = false;
  bool valid2 = false;

  for(int idx = 0; idx < 4; ++idx)
  {
    if (msg.get_abs(static_cast<int>(WIIMOTE_IR_SIZE + 3*idx)) >= 0)
    {
      float x = msg.get_abs_float(WIIMOTE_IR_X + 3*idx);
      float y = msg.get_abs_float(WIIMOTE_IR_Y + 3*idx);
      int size = msg.get_abs(WIIMOTE_IR_SIZE + 3*idx);

      if (!valid1)
      {
        x1 = x;
        y1 = y;
        size1 = size;
        valid1 = true;
      }
      else if (!valid2)
      {
        x2 = x;
        y2 = y;
        size2 = size;
        valid2 = true;
      }
      else if (size > size1)
      {
        x2 = x1;
        y2 = y1;
        size2 = size1;

        x1 = x;
        y1 = y;
        size1 = size;
      }
      else if (size > size2)
      {
        x2 = x;
        y2 = y;
        size2 = size;
      }
    }
  }

  if (!valid1 && !valid2)
  {
    // no IR data, can't do anything
  }
  else if (!valid2)
  {
    log_tmp(x1 << " " << y1);
    msg.set_abs_float(m_xaxis, -x1);
    msg.set_abs_float(m_yaxis, y1);
  }
  else // both valid
  {
    // FIXME: need accelerometer data to find out where is up
    //log_tmp(x1 << " " << y1 << " - " << x2 << " " << y2);
    log_tmp(atan2f(y1 - y2, x1 - x2));
    msg.set_abs_float(m_xaxis, -((x1+x2)/2.0f));
    msg.set_abs_float(m_yaxis, (y1+y2)/2.0f);
  }
#endif
}

std::string
IR2AxisModifier::str() const
{
  return "ir2axis";
}

} // namespace xboxdrv

/* EOF */

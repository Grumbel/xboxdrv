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

#include "controller_slot_config.hpp"

#include <assert.h>
#include <functional>

#include <uinpp/multi_device.hpp>
#include <uinpp/parse.hpp>

#include "controller_slot_options.hpp"
#include "raise_exception.hpp"

namespace xboxdrv {

using namespace std::placeholders;

ControllerSlotConfigPtr
ControllerSlotConfig::create(uinpp::MultiDevice& uinput, int slot, bool extra_devices, const ControllerSlotOptions& opts)
{
  ControllerSlotConfigPtr m_config(new ControllerSlotConfig);

  for(ControllerSlotOptions::Options::const_iterator i = opts.get_options().begin();
      i != opts.get_options().end(); ++i)
  {
    const ControllerOptions& ctrl_opt = i->second;

    ControllerConfigPtr config(new ControllerConfig(uinput, slot, extra_devices, ctrl_opt));
    m_config->add_config(config);
  }

  // LED
  //ioctl(fd, UI_SET_EVBIT, EV_LED);
  //ioctl(fd, UI_SET_LEDBIT, LED_MISC);

  if (opts.get_force_feedback())
  {
    // FF_GAIN     - relative strength of rumble
    // FF_RUMBLE   - basic rumble (delay, time)
    // FF_CONSTANT - envelope, emulate with rumble
    // FF_RAMP     - same as constant, except strength grows
    // FF_PERIODIC - envelope
    // |- FF_SINE      types of periodic effects
    // |- FF_TRIANGLE
    // |- FF_SQUARE
    // |- FF_SAW_UP
    // |- FF_SAW_DOWN
    // '- FF_CUSTOM

    // FIXME: this should go through the regular resolution process
    uint32_t ff_device = uinpp::create_device_id(static_cast<uint16_t>(slot),
                                                 static_cast<uint16_t>(opts.get_ff_device()));

    // basic types
    uinput.add_ff(ff_device, FF_RUMBLE);
    uinput.add_ff(ff_device, FF_PERIODIC);
    uinput.add_ff(ff_device, FF_CONSTANT);
    uinput.add_ff(ff_device, FF_RAMP);

    // periodic effect subtypes
    uinput.add_ff(ff_device, FF_SINE);
    uinput.add_ff(ff_device, FF_TRIANGLE);
    uinput.add_ff(ff_device, FF_SQUARE);
    uinput.add_ff(ff_device, FF_SAW_UP);
    uinput.add_ff(ff_device, FF_SAW_DOWN);
    uinput.add_ff(ff_device, FF_CUSTOM);

    // gin support
    uinput.add_ff(ff_device, FF_GAIN);

    // Unsupported effects
    // uinput.add_ff(ff_device, FF_SPRING);
    // uinput.add_ff(ff_device, FF_FRICTION);
    // uinput.add_ff(ff_device, FF_DAMPER);
    // uinput.add_ff(ff_device, FF_INERTIA);

    uinput.set_ff_callback(ff_device, std::bind(&ControllerSlotConfig::set_rumble, m_config.get(), _1, _2));
  }

  return m_config;
}

ControllerSlotConfig::ControllerSlotConfig() :
  m_config(),
  m_current_config(0),
  m_rumble_callback()
{
}

void
ControllerSlotConfig::next_config()
{
  m_current_config += 1;

  if (m_current_config >= static_cast<int>(m_config.size()))
  {
    m_current_config = 0;
  }
}

void
ControllerSlotConfig::prev_config()
{
  m_current_config -= 1;

  if (m_current_config < 0)
  {
    m_current_config = static_cast<int>(m_config.size()) - 1;
  }
}

int
ControllerSlotConfig::config_count() const
{
  return static_cast<int>(m_config.size());
}

ControllerConfigPtr
ControllerSlotConfig::get_config(int i) const
{
  assert(i >= 0);
  assert(i < static_cast<int>(m_config.size()));

  return m_config[i];
}

void
ControllerSlotConfig::set_current_config(int num)
{
  if (num >= 0 && num < static_cast<int>(m_config.size()))
  {
    m_current_config = num;
  }
  else
  {
    raise_exception(std::runtime_error, "argument out of range");
  }
}

ControllerConfigPtr
ControllerSlotConfig::get_config() const
{
  assert(!m_config.empty());

  return m_config[m_current_config];
}

void
ControllerSlotConfig::add_config(ControllerConfigPtr config)
{
  m_config.push_back(config);
}

void
ControllerSlotConfig::set_rumble(uint8_t strong, uint8_t weak)
{
  if (m_rumble_callback)
  {
    m_rumble_callback(strong, weak);
  }
}

void
ControllerSlotConfig::set_ff_callback(const std::function<void (uint8_t, uint8_t)>& callback)
{
  m_rumble_callback = callback;
}

} // namespace xboxdrv

/* EOF */

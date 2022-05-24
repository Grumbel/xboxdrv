/*
**  Logitech Gamepad F310 driver for xboxdrv
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmail.com>
**  Contributed by Doug Morse <dm@dougmorse.org>
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

#ifndef HEADER_LOGITECH_F310_CONTROLLER_HPP
#define HEADER_LOGITECH_F310_CONTROLLER_HPP

#include <libusb.h>

#include "xboxmsg.hpp"
#include "controller/usb_controller.hpp"
#include "xbox360_default_names.hpp"

class LogitechF310Controller : public USBController
{
private:
  int left_rumble;
  int right_rumble;

  Xbox360DefaultNames xbox;

public:
  LogitechF310Controller(libusb_device* dev, bool try_detach);
  ~LogitechF310Controller();

  void set_rumble_real(uint8_t left, uint8_t right) override;
  void set_led_real(uint8_t status) override;

  bool parse(const uint8_t* data, int len, ControllerMessage* msg_out) override;

private:
  LogitechF310Controller(const LogitechF310Controller&);
  LogitechF310Controller& operator=(const LogitechF310Controller&);
};

#endif

/* EOF */

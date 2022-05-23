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

#ifndef HEADER_XBOXDRV_PLAYSTATION3_USB_CONTROLLER_HPP
#define HEADER_XBOXDRV_PLAYSTATION3_USB_CONTROLLER_HPP

#include <libusb.h>

#include "controller/usb_controller.hpp"
#include "xbox360_default_names.hpp"

class Playstation3USBController : public USBController
{
private:
  int endpoint_in;
  int endpoint_out;

  Xbox360DefaultNames xbox;

public:
  Playstation3USBController(libusb_device* dev, bool try_detach);
  ~Playstation3USBController();

  void set_rumble_real(uint8_t left, uint8_t right) override;
  void set_led_real(uint8_t status) override;

  bool parse(const uint8_t* data, int len, ControllerMessage* msg_out) override;

private:
  Playstation3USBController(const Playstation3USBController&);
  Playstation3USBController& operator=(const Playstation3USBController&);
};

#endif

/* EOF */

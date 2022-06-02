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

#include "controller/playstation3_bluetooth_controller.hpp"

namespace xboxdrv {

Playstation3BluetoothController::Playstation3BluetoothController()
{
}

Playstation3BluetoothController::~Playstation3BluetoothController()
{
}

void
Playstation3BluetoothController::set_rumble_real(uint8_t left, uint8_t right)
{
}

void
Playstation3BluetoothController::set_led_real(uint8_t status)
{
}

} // namespace xboxdrv

/* EOF */

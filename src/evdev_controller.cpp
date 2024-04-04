/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmail.com>
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

#include "evdev_controller.hpp"

#include <algorithm>
#include <format>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <stdio.h>

#include "evdev_helper.hpp"
#include "helper.hpp"
#include "log.hpp"


#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define ioctl_x(fd, request, ...) errno_check(ioctl(fd, request, __VA_ARGS__), __FILE__, __LINE__, STRINGIFY(ioctl(fd, request, __VA_ARGS__)));

#define BITS_PER_LONG (sizeof(long) * 8)
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_LONG)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

static void errno_check(int status, char const *file, int line, char const *expr)
{
  if (status < 0)
  {
    warn("%s:%d: %s", file, line, expr);
  }
}

EvdevController::EvdevController(const std::string& filename,
                                 const EvdevAbsMap& absmap,
                                 const std::map<int, UIActionPtr>& keymap,
                                 bool grab,
                                 bool debug) :
  m_fd(-1),
  m_io_channel(),
  m_name(),
  m_grab(grab),
  m_debug(debug),
  m_absmap(absmap),
  m_keymap(keymap),
  m_absinfo(ABS_MAX),
  m_event_buffer(),
  m_msg()
{
  XboxGenericMsg msg;
  memset(&msg, 0, sizeof(XboxGenericMsg));
  msg.type = XBOX_MSG_XBOX360;

  m_fd = open(filename.c_str(), O_RDWR | O_NONBLOCK);

  if (m_fd == -1)
  {
    throw std::runtime_error(filename + ": " + std::string(strerror(errno)));
  }

  { // Get the human readable name
    char c_name[1024] = "unknown";
    ioctl_x(m_fd, EVIOCGNAME(sizeof(c_name)), c_name);
    m_name = c_name;
    log_debug("name: " << m_name);
  }

  if (m_grab)
  { // grab the device, so it doesn't broadcast events into the wild
    int ret = ioctl(m_fd, EVIOCGRAB, 1);
    if ( ret == -1 )
    {
      close(m_fd);
      throw std::runtime_error(strerror(errno));
    }
  }

  { // Read in how many btn/abs/rel/ff the device has
    unsigned long abs_bit[BITS_TO_LONGS(ABS_CNT)] = { 0 };
    unsigned long rel_bit[BITS_TO_LONGS(REL_CNT)] = { 0 };
    unsigned long key_bit[BITS_TO_LONGS(KEY_CNT)] = { 0 };
    unsigned long  ff_bit[BITS_TO_LONGS( FF_CNT)] = { 0 };

    ioctl_x(m_fd, EVIOCGBIT(EV_ABS, sizeof(abs_bit)), abs_bit);
    ioctl_x(m_fd, EVIOCGBIT(EV_REL, sizeof(rel_bit)), rel_bit);
    ioctl_x(m_fd, EVIOCGBIT(EV_KEY, sizeof(key_bit)), key_bit);
    ioctl_x(m_fd, EVIOCGBIT(EV_FF,  sizeof( ff_bit)),  ff_bit);

    for(int i = 0; i < ABS_CNT; ++i)
    {
      if (test_bit(i, abs_bit))
      {
        struct input_absinfo absinfo;
        ioctl_x(m_fd, EVIOCGABS(i), &absinfo);

        log_debug(std::format("abs: {:20s} min: {:6d} max: {:6d}", abs2str(i), absinfo.minimum, absinfo.maximum));
        m_absinfo[i] = absinfo;
      }
    }

    for(int i = 0; i < REL_CNT; ++i)
    {
      if (test_bit(i, rel_bit))
      {
        log_debug("rel: " << rel2str(i));
      }
    }

    for(int i = 0; i < KEY_CNT; ++i)
    {
      if (test_bit(i, key_bit))
      {
        log_debug("key: " << key2str(i));
      }
    }

    for(int i = 0; i < FF_CNT; ++i)
    {
      if (test_bit(i, ff_bit))
      {
        std::cout.setf(std::ios::hex);
        log_debug("ff: 0x" << i);
        std::cout.unsetf(std::ios::hex);
        m_ff_features.push_back(i);
      }
    }
  }

  if (ioctl(m_fd, EVIOCGEFFECTS, &m_num_ff_effects) < 0)
  {
    perror("EVIOCGEFFECTS");
  }

  { // start g_io_channel
    m_io_channel = g_io_channel_unix_new(m_fd);

    // set encoding to binary
    GError* error = NULL;
    if (g_io_channel_set_encoding(m_io_channel, NULL, &error) != G_IO_STATUS_NORMAL)
    {
      log_error(error->message);
      g_error_free(error);
    }

    g_io_channel_set_buffered(m_io_channel, false);

    g_io_add_watch(m_io_channel,
                   static_cast<GIOCondition>(G_IO_IN | G_IO_ERR | G_IO_HUP),
                   &EvdevController::on_read_data_wrap, this);
  }
}

EvdevController::~EvdevController()
{
  g_io_channel_unref(m_io_channel);
  close(m_fd);
}

void
EvdevController::set_rumble_real(uint8_t left, uint8_t right)
{
  // not implemented
}

void
EvdevController::set_led_real(uint8_t status)
{
  // not implemented
}

void
EvdevController::upload(const struct ff_effect& effect)
{
  struct ff_effect cpy = effect;
  cpy.id = -1;
  if (ioctl(m_fd, EVIOCSFF, &cpy) < 0) {
    perror("force-feedback: uploading event");
  }
}

void
EvdevController::erase(int id)
{
  if (ioctl(m_fd, EVIOCRMFF, id) < 0) {
    perror("force-feedback: erasing event");
  }
}

static void write_event(int fd, int code, int value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = EV_FF;
    event.code = code;
    event.value = value;

    if (write(fd, (const void*) &event, sizeof(event)) < 0) {
      perror("force-feedback: sending event");
    }
}

void
EvdevController::play(int id)
{
   write_event(m_fd, id, 1);
}

void
EvdevController::stop(int id)
{
  write_event(m_fd, id, 0);
}

void
EvdevController::set_gain(int gain)
{
  if (std::find(m_ff_features.begin(), m_ff_features.end(), FF_GAIN) != m_ff_features.end())
  {
    log_debug("FF_GAIN is supported");
    write_event(m_fd, FF_GAIN, gain);
  }
}

bool
EvdevController::parse(const struct input_event& ev, XboxGenericMsg &msg_inout) const
{
  if (m_debug)
  {
    switch(ev.type)
    {
      case EV_KEY:
        std::cout << "EV_KEY " << key2str(ev.code) << " " << ev.value << std::endl;
        break;

      case EV_REL:
        std::cout << "EV_REL " << rel2str(ev.code) << " " << ev.value << std::endl;
        break;

      case EV_ABS:
        std::cout << "EV_ABS " << abs2str(ev.code) << " " << ev.value << std::endl;
        break;

      case EV_SYN:
        std::cout << "------------------- sync -------------------" << std::endl;
        break;

      case EV_MSC:
        // FIXME: no idea what those are good for, but they pop up
        // after key presses (something with scancodes maybe?!)
        break;

      default:
        log_info("unknown: " << ev.type << " " << ev.code << " " << ev.value);
        break;
    }
  }

  switch(ev.type)
  {
    case EV_KEY:
      {
        KeyMap::const_iterator it = m_keymap.find(ev.code);
        if (it != m_keymap.end())
        {
          it->second->parse(msg_inout, ev);
          return true;
        }
        else
        {
          return false;
        }
      }
      break;

    case EV_ABS:
      {
        const struct input_absinfo& absinfo = m_absinfo[ev.code];
        m_absmap.process(msg_inout, ev.code,
                         // some buggy USB devices report values
                         // outside the given range, so we clamp it
                         std::clamp(ev.value, absinfo.minimum, absinfo.maximum),
                         absinfo.minimum, absinfo.maximum);
        return true; // FIXME: wrong
        break;
      }

    default:
      // not supported event
      return false;
      break;
  }
}

gboolean
EvdevController::on_read_data(GIOChannel* source, GIOCondition condition)
{
  // read data
  struct input_event ev[128];
  int rd = 0;
  while((rd = ::read(m_fd, ev, sizeof(struct input_event) * 128)) > 0)
  {
    for (size_t i = 0; i < rd / sizeof(struct input_event); ++i)
    {
      if (ev[i].type == EV_SYN)
      {
        submit_msg(m_msg);
      }
      else
      {
        parse(ev[i], m_msg);
      }
    }
  }

  return TRUE;
}

/* EOF */

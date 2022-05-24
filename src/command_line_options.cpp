/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "command_line_options.hpp"

#include <assert.h>
#include <functional>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <iterator>

#include <uinpp/event.hpp>
#include <uinpp/parse.hpp>
#include <yaini/parser.hpp>
#include <yaini/schema_builder.hpp>

#include "evdev_helper.hpp"
#include "util/string.hpp"
#include "options.hpp"
#include "path.hpp"
#include "raise_exception.hpp"

#include "axis_map_option.hpp"
#include "button_event_factory.hpp"
#include "button_map_option.hpp"

#include "axisfilter/relative_axis_filter.hpp"
#include "axisfilter/calibration_axis_filter.hpp"
#include "axisfilter/sensitivity_axis_filter.hpp"
#include "buttonfilter/autofire_button_filter.hpp"

#include "modifier/axismap_modifier.hpp"
#include "modifier/buttonmap_modifier.hpp"

#include "xboxdrv_vfs.hpp"

using namespace std::placeholders;

enum {
  OPTION_HELP,
  OPTION_VERBOSE,
  OPTION_VERSION,
  OPTION_DEBUG,
  OPTION_QUIET,
  OPTION_SILENT,
  OPTION_USB_DEBUG,
  OPTION_DAEMON,
  OPTION_CONFIG_OPTION,
  OPTION_CONFIG,
  OPTION_ALT_CONFIG,
  OPTION_WRITE_CONFIG,
  OPTION_TEST_RUMBLE,
  OPTION_RUMBLE,
  OPTION_FF_DEVICE,
  OPTION_PRIORITY,
  OPTION_QUIT,
  OPTION_NO_UINPUT,
  OPTION_MIMIC_XPAD,
  OPTION_MIMIC_XPAD_WIRELESS,
  OPTION_NO_EXTRA_DEVICES,
  OPTION_NO_EXTRA_EVENTS,
  OPTION_TYPE,
  OPTION_FORCE_FEEDBACK,
  OPTION_RUMBLE_GAIN,
  OPTION_MODIFIER,
  OPTION_BUTTONMAP,
  OPTION_AXISMAP,
  OPTION_DEVICE_NAME,
  OPTION_DEVICE_NAMES,
  OPTION_DEVICE_USBID,
  OPTION_DEVICE_USBIDS,
  OPTION_NEXT_CONFIG,
  OPTION_NEXT_CONTROLLER,
  OPTION_CONFIG_SLOT,
  OPTION_CONTROLLER_SLOT,
  OPTION_UI_CLEAR,
  OPTION_TOGGLE,
  OPTION_ABSMAP,
  OPTION_KEYMAP,
  OPTION_RELMAP,
  OPTION_ID,
  OPTION_WID,
  OPTION_LED,
  OPTION_DPAD_ONLY,
  OPTION_DPAD_AS_BUTTON,
  OPTION_DEADZONE,
  OPTION_DEADZONE_TRIGGER,
  OPTION_TRIGGER_AS_BUTTON,
  OPTION_TRIGGER_AS_ZAXIS,
  OPTION_AUTOFIRE,
  OPTION_CALIBRARIOTION,
  OPTION_RELATIVE_AXIS,
  OPTION_SQUARE_AXIS,
  OPTION_FOUR_WAY_RESTRICTOR,
  OPTION_DPAD_ROTATION,
  OPTION_AXIS_SENSITIVITY,
  OPTION_HELP_LED,
  OPTION_DEVICE_BY_ID,
  OPTION_DEVICE_BY_PATH,
  OPTION_GENERIC_USB_SPEC,
  OPTION_LIST_SUPPORTED_DEVICES,
  OPTION_LIST_SUPPORTED_DEVICES_XPAD,
  OPTION_LIST_CONTROLLER,
  OPTION_MOUSE,
  OPTION_GUITAR,
  OPTION_EVDEV,
  OPTION_EVDEV_NO_GRAB,
  OPTION_EVDEV_DEBUG,
  OPTION_EVDEV_ABSMAP,
  OPTION_EVDEV_KEYMAP,
  OPTION_EVDEV_RELMAP,
  OPTION_WIIMOTE,
  OPTION_CHATPAD,
  OPTION_CHATPAD_NO_INIT,
  OPTION_CHATPAD_DEBUG,
  OPTION_TIMEOUT,
  OPTION_HEADSET,
  OPTION_HEADSET_DUMP,
  OPTION_HEADSET_PLAY,
  OPTION_DETACH_KERNEL_DRIVER,
  OPTION_DAEMON_DETACH,
  OPTION_DAEMON_PID_FILE,
  OPTION_DAEMON_MATCH,
  OPTION_DAEMON_MATCH_GROUP,
  OPTION_DAEMON_NO_DBUS,
  OPTION_DAEMON_DBUS,
  OPTION_HELP_DEVICES,
  OPTION_LIST_ALL,
  OPTION_LIST_ABS,
  OPTION_LIST_REL,
  OPTION_LIST_KEY,
  OPTION_LIST_X11KEYSYM,
  OPTION_DAEMON_ON_CONNECT,
  OPTION_DAEMON_ON_DISCONNECT
};

CommandLineParser::CommandLineParser() :
  m_argp(),
  m_ini(),
  m_options(),
  m_directory_context()
{
}

void
CommandLineParser::init_argp(int argc, char** argv)
{
  constexpr char NUL = '\0';
  m_argp.add_usage(argv[0], "[OPTION]...")
    .add_text("Xbox360 USB Gamepad Userspace Driver");

  m_argp.add_group("General Options: ")
    .add_option(OPTION_HELP,         'h', "help",         "", "display this help and exit")
    .add_option(OPTION_VERSION,      'V', "version",      "", "print the version number and exit")
    .add_option(OPTION_VERBOSE,      'v', "verbose",      "", "print verbose messages")
    .add_option(OPTION_DEBUG,        NUL, "debug",   "",  "be even more verbose then --verbose")
    .add_option(OPTION_SILENT,       's', "silent",  "",  "do not display events on console")
    .add_option(OPTION_QUIET,        NUL, "quiet",   "",  "do not display startup text")
    .add_option(OPTION_USB_DEBUG,    NUL, "usb-debug", "",  "enable log messages from libusb")
    .add_option(OPTION_PRIORITY,     NUL, "priority", "PRI", "increases process priority (default: normal)");

  m_argp.add_group("List Options: ")
    .add_option(OPTION_LIST_SUPPORTED_DEVICES,NUL, "list-supported-devices", "", "list supported devices (used by xboxdrv-daemon.py)", false)
    .add_option(OPTION_LIST_SUPPORTED_DEVICES_XPAD,NUL, "list-supported-devices-xpad", "", "list supported devices in xpad.c style", false)
    .add_option(OPTION_HELP_LED,     NUL, "help-led",     "", "list possible values for the led")
    .add_option(OPTION_HELP_DEVICES, NUL, "help-devices", "", "list supported devices")
    .add_option(OPTION_LIST_ABS,      NUL, "help-abs",       "", "list all possible EV_ABS names")
    .add_option(OPTION_LIST_REL,      NUL, "help-rel",       "", "list all possible EV_REL names")
    .add_option(OPTION_LIST_KEY,      NUL, "help-key",       "", "list all possible EV_KEY names")
    .add_option(OPTION_LIST_X11KEYSYM,NUL, "help-x11keysym", "", "list all possible X11KeySym")
    .add_option(OPTION_LIST_ALL,      NUL, "help-all",       "", "list all symbols above");

  m_argp.add_group("Config File Options: ")
    .add_option(OPTION_CONFIG,       'c', "config",      "FILE", "read configuration from FILE")
    .add_option(OPTION_ALT_CONFIG,   NUL, "alt-config",   "FILE", "read alternative configuration from FILE ")
    .add_option(OPTION_CONFIG_OPTION,'o', "option",      "NAME=VALUE", "Set the given configuration option")
    .add_option(OPTION_WRITE_CONFIG, NUL, "write-config", "FILE", "write an example configuration to FILE");

  m_argp.add_group("Daemon Options: ")
    .add_option(OPTION_DAEMON,        'D', "daemon",    "", "Run as daemon")
    .add_option(OPTION_DAEMON_DETACH,  NUL, "detach",      "", "Detach the daemon from the current shell")
    .add_option(OPTION_DAEMON_PID_FILE,NUL, "pid-file",    "FILE", "Write daemon pid to FILE")
    .add_option(OPTION_DAEMON_NO_DBUS, NUL, "no-dbus",    "", "Disables D-Bus support in the daemon", false)
    .add_option(OPTION_DAEMON_DBUS,    NUL, "dbus",    "MODE", "Set D-Bus mode (auto, system, session, disabled)")
    .add_option(OPTION_DAEMON_ON_CONNECT,   NUL, "on-connect", "FILE", "Launch EXE when a new controller is connected")
    .add_option(OPTION_DAEMON_ON_DISCONNECT,NUL, "on-disconnect", "FILE", "Launch EXE when a controller is disconnected");

  m_argp.add_group("Device Options: ")
    .add_option(OPTION_LIST_CONTROLLER, 'L', "list-controller", "", "list available controllers")
    .add_option(OPTION_ID,           'i', "id",      "N", "use controller with id N (default: 0)")
    .add_option(OPTION_WID,          'w', "wid",     "N", "use wireless controller with wid N (default: 0)")
    .add_option(OPTION_DEVICE_BY_PATH,NUL, "device-by-path", "BUS:DEV", "Use device BUS:DEV, do not do any scanning")
    .add_option(OPTION_DEVICE_BY_ID,  NUL, "device-by-id",   "VENDOR:PRODUCT", "Use device that matches VENDOR:PRODUCT (as returned by lsusb)")
    .add_option(OPTION_TYPE,          NUL, "type",    "TYPE", "Ignore autodetection and enforce controller type (xbox, xbox-mat, xbox360, xbox360-wireless, xbox360-guitar)")
    .add_option(OPTION_DETACH_KERNEL_DRIVER, 'd', "detach-kernel-driver", "", "Detaches the kernel driver currently associated with the device")
    .add_option(OPTION_GENERIC_USB_SPEC,NUL, "generic-usb-spec", "SPEC", "Specification for generic USB device");

  m_argp.add_group("Evdev Options: ")
    .add_option(OPTION_EVDEV,         NUL, "evdev",   "DEVICE", "Read events from a evdev device, instead of USB")
    .add_option(OPTION_EVDEV_DEBUG,   NUL, "evdev-debug", "", "Print out all events received from evdev")
    .add_option(OPTION_EVDEV_NO_GRAB, NUL, "evdev-no-grab", "", "Do not grab the event device, allow other apps to receive events")
    .add_option(OPTION_EVDEV_ABSMAP,  NUL, "evdev-absmap", "MAP", "Set how evdev abs names are mapped to xboxdrv names")
    .add_option(OPTION_EVDEV_KEYMAP,  NUL, "evdev-keymap", "MAP", "Set how evdev abs names are mapped to xboxdrv names")
    .add_option(OPTION_EVDEV_RELMAP,  NUL, "evdev-relmap", "MAP", "Set how evdev abs names are mapped to xboxdrv names");

  m_argp.add_group("Wiimote Options: ")
    .add_option(OPTION_WIIMOTE,  NUL, "wiimote", "", "Use Wiimote as main controller");

  m_argp.add_group("Status Options: ")
    .add_option(OPTION_LED,     'l', "led",    "STATUS", "set LED status, see --help-led for possible values")
    .add_option(OPTION_RUMBLE,  'r', "rumble", "L,R", "set the speed for both rumble motors [0-255] (default: 0,0)")
    .add_option(OPTION_QUIT,    'q', "quit",   "",    "only set led and rumble status then quit");

  m_argp.add_group("Chatpad Options (experimental): ")
    .add_option(OPTION_CHATPAD,       NUL, "chatpad", "",  "Enable Chatpad support for Xbox360 USB controller")
    .add_option(OPTION_CHATPAD_NO_INIT,NUL, "chatpad-no-init", "",  "To not send init code to the Chatpad")
    .add_option(OPTION_CHATPAD_DEBUG,NUL, "chatpad-debug", "",  "To not send init code to the Chatpad");

  m_argp.add_group("Headset Options (experimental, Xbox360 USB only): ")
    .add_option(OPTION_HEADSET,       NUL, "headset", "",  "Enable Headset support for Xbox360 USB controller (not working)")
    .add_option(OPTION_HEADSET_DUMP,  NUL, "headset-dump", "FILE",  "Dump headset data to FILE")
    .add_option(OPTION_HEADSET_PLAY,  NUL, "headset-play", "FILE",  "Play FILE on the headset");

  m_argp.add_group("Force Feedback: ")
    .add_option(OPTION_FORCE_FEEDBACK,    NUL, "force-feedback",   "",     "Enable force feedback support")
    .add_option(OPTION_RUMBLE_GAIN,       NUL, "rumble-gain",      "NUM",  "Set relative rumble strength (default: 255)")
    .add_option(OPTION_TEST_RUMBLE,      'R', "test-rumble", "", "map rumbling to LT and RT (for testing only)")
    .add_option(OPTION_FF_DEVICE,         NUL, "ff-device", "DEV", "select to which evdev the force feedback should be connected (default: joystick)");

  m_argp.add_group("Controller Slot Options: ")
    .add_option(OPTION_CONTROLLER_SLOT,   NUL, "controller-slot", "N", "Use controller slot N")
    .add_option(OPTION_NEXT_CONTROLLER,   NUL, "next-controller", "", "Create a new controller entry")
    .add_option(OPTION_DAEMON_MATCH,      NUL, "match", "RULES",   "Only allow controllers that match any of RULES")
    .add_option(OPTION_DAEMON_MATCH_GROUP,NUL, "match-group", "RULES", "Only allow controllers that match all of RULES");

  m_argp.add_group("Config Slot Options: ")
    .add_option(OPTION_CONFIG_SLOT,       NUL, "config-slot",     "N", "Use configuration slot N")
    .add_option(OPTION_NEXT_CONFIG,       NUL, "ui-new", "", "", false) // backward compatibility
    .add_option(OPTION_NEXT_CONFIG,       NUL, "next-config", "", "Create a new configuration entry")
    .add_option(OPTION_TOGGLE,            NUL, "toggle", "BTN", "Set button to use for toggling between configs")
    .add_option(OPTION_TOGGLE,            NUL, "ui-toggle", "BTN", "", false); // backward compatibility

  m_argp.add_group("Configuration Options:")
    .add_option(OPTION_MODIFIER,          'm', "modifier",       "MOD=ARG:..", "Add a modifier to the modifier spec")
    .add_option(OPTION_TIMEOUT,           NUL, "timeout",         "INT",  "Amount of time to wait fo a device event before processing autofire, etc. (default: 25)")
    .add_option(OPTION_BUTTONMAP,         'b', "buttonmap",      "MAP",   "Remap the buttons as specified by MAP (example: B=A,X=A,Y=A)")
    .add_option(OPTION_AXISMAP,           'a', "axismap",        "MAP",   "Remap the axis as specified by MAP (example: -Y1=Y1,X1=X2)");

  m_argp.add_group("Modifier Preset Options: ")
    .add_option(OPTION_AUTOFIRE,          NUL, "autofire",         "MAP",  "Cause the given buttons to act as autofire (example: A=250)")
    .add_option(OPTION_AXIS_SENSITIVITY,  NUL, "axis-sensitivity", "MAP",  "Adjust the axis sensitivity (example: X1=2.0,Y1=1.0)")
    .add_option(OPTION_CALIBRARIOTION,    NUL, "calibration",      "MAP",  "Changes the calibration for the given axis (example: X2=-32768:0:32767)")
    .add_option(OPTION_DEADZONE,          NUL, "deadzone",         "INT",  "Threshold under which axis events are ignored (default: 0)")
    .add_option(OPTION_DEADZONE_TRIGGER,  NUL, "deadzone-trigger", "INT",  "Threshold under which trigger events are ignored (default: 0)")
    .add_option(OPTION_DPAD_ROTATION,     NUL, "dpad-rotation",    "DEGREE", "Rotate the dpad by the given DEGREE, must be a multiple of 45")
    .add_option(OPTION_FOUR_WAY_RESTRICTOR, NUL, "four-way-restrictor", "",  "Restrict axis movement to one axis at a time")
    .add_option(OPTION_RELATIVE_AXIS,     NUL, "relative-axis",    "MAP",  "Make an axis emulate a joystick throttle (example: y2=64000)")
    .add_option(OPTION_SQUARE_AXIS,       NUL, "square-axis",       "",     "Cause the diagonals to be reported as (1,1) instead of (0.7, 0.7)");

  m_argp.add_group("Uinput Preset Configuration Options: ")
    .add_option(OPTION_TRIGGER_AS_BUTTON, NUL, "trigger-as-button", "",    "LT and RT send button instead of axis events")
    .add_option(OPTION_TRIGGER_AS_ZAXIS,  NUL, "trigger-as-zaxis", "",     "Combine LT and RT to form a zaxis instead")
    .add_option(OPTION_DPAD_AS_BUTTON,    NUL, "dpad-as-button",   "",     "DPad sends button instead of axis events")
    .add_option(OPTION_DPAD_ONLY,         NUL, "dpad-only",        "",     "Both sticks are ignored, only DPad sends out axis events")
    .add_option(OPTION_GUITAR,            NUL, "guitar",            "",     "Enables guitar button and axis mapping")
    .add_option(OPTION_MOUSE,             NUL, "mouse",            "",     "Enable mouse emulation")
    .add_option(OPTION_MIMIC_XPAD,        NUL, "mimic-xpad",  "", "Causes xboxdrv to use the same axis and button names as the xpad kernel driver for wired gamepads")
    .add_option(OPTION_MIMIC_XPAD_WIRELESS,NUL, "mimic-xpad-wireless",  "", "Causes xboxdrv to use the same axis and button names as the xpad kernel driver for wireless gamepads");

  m_argp.add_group("Uinput Options: ")
    .add_option(OPTION_NO_UINPUT,         NUL, "no-uinput",   "", "do not try to start uinput event dispatching")
    .add_option(OPTION_NO_EXTRA_DEVICES,  NUL, "no-extra-devices",  "", "Do not create separate virtual keyboard and mouse devices, just use a single virtual device")
    .add_option(OPTION_NO_EXTRA_EVENTS,   NUL, "no-extra-events",  "", "Do not create dummy events to facilitate device type detection")
    .add_option(OPTION_DEVICE_NAME,       NUL, "device-name",     "NAME", "Changes the name prefix used for devices in the current slot")
    .add_option(OPTION_DEVICE_NAMES,      NUL, "device-names",    "DEVID=NAME,...", "Changes the descriptive name the given devices")
    .add_option(OPTION_DEVICE_USBID,      NUL, "device-usbid",     "VENDOR:PRODUCT:VERSION", "Changes the USB Id used for devices in the current slot")
    .add_option(OPTION_DEVICE_USBIDS,     NUL, "device-usbids",    "DEVID=VENDOR:PRODUCT:VERSION,...", "Changes the USB Id for the given devices");

  m_argp.add_group("Emitter Options: ")
    .add_option(OPTION_UI_CLEAR,    NUL, "ui-clear",         "",     "Removes all existing uinput bindings")
    .add_option(OPTION_ABSMAP,      NUL, "absmap", "MAP", "Changes the uinput events send when moving a abs (example: X1=ABS_X2)")
    .add_option(OPTION_KEYMAP,      NUL, "keymap", "MAP", "Changes the uinput events send when hitting a key (example: X=BTN_Y,A=KEY_A)")
    .add_option(OPTION_RELMAP,      NUL, "relmap", "MAP", "Changes the uinput events send when moving a rel (example: X1=REL_X)")
    .add_option(OPTION_KEYMAP,      NUL, "ui-buttonmap",     "MAP", "", false)
    .add_option(OPTION_ABSMAP,      NUL, "ui-axismap",       "MAP", "", false);

  m_argp.add_group("Axis Filter:")
    .add_pseudo("  cal, calibration MIN:CENTER:MAX", "Set the calibration values for the axis")
    .add_pseudo("  sen, sensitivity:SENSITIVITY", "Set the axis sensitivity")
    .add_pseudo("  dead:VALUE, dead:MIN:CENTER:MAX", "Set the axis deadzone")
    .add_pseudo("  rel, relative:SPEED", "Turn axis into a relative-axis")
    .add_pseudo("  resp, response:VALUES:...", "Set values of the response curve")
    .add_pseudo("  log:STRING", "Print axis value to stdout");

  m_argp.add_group("Button Filter:")
    .add_pseudo("  tog, toggle", "Turn button into a toggle button")
    .add_pseudo("  inv, invert", "Invert the button value")
    .add_pseudo("  auto, autofire:RATE:DELAY", "Enable automatic button press repetition")
    .add_pseudo("  log:STRING", "Print button value to stdout");

  m_argp.add_group("Modifier:")
    .add_pseudo("  btn2axis=BTN:BTN:AXIS", "Turns two buttons into an axis")
    .add_pseudo("  dpad-rotate=DEGREE", "Rotate the dpad by the given number of degree")
    .add_pseudo("  dpad-restrictor=RESTRICTION", "Restrict dpad movment to 'x-axis', 'y-axis' or 'four-way'")
    .add_pseudo("  4wayrest, four-way-restrictor=XAXIS:YAXIS", "Restrict the given stick to four directions")
    .add_pseudo("  square, square-axis=XAXIS:YAXIS", "Convert the circular motion range of the given stick to a square one")
    .add_pseudo("  rotate=XAXIS:YAXIS:DEGREE[:MIRROR]", "Rotate the given stick by DEGREE, optionally also mirror it");

  m_argp.add_group()
    .add_text("See README for more documentation and examples.")
    .add_text("Report bugs to Ingo Ruhnke <grumbel@gmail.com>");
}

void
CommandLineParser::init_ini(Options* opts)
{
  m_ini.clear();

  m_ini.section("xboxdrv")
    ("verbose", std::bind(&Options::set_verbose, opts), std::function<void ()>())
    ("silent", &opts->silent)
    ("quiet",  &opts->quiet)
    ("usb-debug",  &opts->usb_debug)
    ("rumble", &opts->rumble)
    ("led", std::bind(&Options::set_led, opts, _1))
    ("rumble-l", &opts->rumble_l)
    ("rumble-r", &opts->rumble_r)
    ("rumble-gain", &opts->rumble_gain)
    ("controller-id", &opts->controller_id)
    ("wireless-id", &opts->wireless_id)
    ("instant-exit", &opts->instant_exit)
    ("no-uinput", &opts->no_uinput)
    ("detach-kernel-driver", &opts->detach_kernel_driver)
    ("busid", &opts->busid)
    ("devid", &opts->devid)
    ("vendor-id", &opts->vendor_id)
    ("product-id", &opts->product_id)
    ("evdev", &opts->evdev_device)
    ("evdev-grab", &opts->evdev_grab)
    ("evdev-debug", &opts->evdev_debug)
    ("config", std::bind(&CommandLineParser::read_config_file, this, _1))
    ("alt-config", std::bind(&CommandLineParser::read_alt_config_file, this, _1))
    ("timeout", &opts->timeout)
    ("priority", std::bind(&Options::set_priority, opts, _1))
    ("next", std::bind(&Options::next_config, opts), std::function<void ()>())
    ("next-controller", std::bind(&Options::next_controller, opts), std::function<void ()>())
    ("extra-devices", &opts->extra_devices)
    ("extra-events", &opts->extra_events)
    ("toggle", std::bind(&Options::set_toggle_button, opts, _1))
    ("ff-device", std::bind(&Options::set_ff_device, opts, _1))

    ("deadzone", std::bind(&CommandLineParser::set_deadzone, this, _1))
    ("deadzone-trigger", std::bind(&CommandLineParser::set_deadzone_trigger, this, _1))
    ("square-axis", std::bind(&CommandLineParser::set_square_axis, this), std::function<void ()>())
    ("four-way-restrictor", std::bind(&CommandLineParser::set_four_way_restrictor, this), std::function<void ()>())
    ("dpad-rotation", std::bind(&CommandLineParser::set_dpad_rotation, this, _1))

    // uinput stuff
    ("device-name",       std::bind(&Options::set_device_name, opts, _1))
    ("device-usbid",      std::bind(&Options::set_device_usbid, opts, _1))
    ("mouse",             std::bind(&CommandLineParser::mouse, this), std::function<void ()>())
    ("guitar",            std::bind(&Options::set_guitar, opts),            std::function<void ()>())
    ("trigger-as-button", std::bind(&Options::set_trigger_as_button, opts), std::function<void ()>())
    ("trigger-as-zaxis",  std::bind(&Options::set_trigger_as_zaxis, opts),  std::function<void ()>())
    ("dpad-as-button",    std::bind(&Options::set_dpad_as_button, opts),    std::function<void ()>())
    ("dpad-only",         std::bind(&Options::set_dpad_only, opts),         std::function<void ()>())
    ("force-feedback",    std::bind(&Options::set_force_feedback, opts, _1))
    ("mimic-xpad",        std::bind(&Options::set_mimic_xpad, opts),        std::function<void ()>())
    ("mimic-xpad-wireless", std::bind(&Options::set_mimic_xpad_wireless, opts), std::function<void ()>())

    ("chatpad",         &opts->chatpad)
    ("chatpad-no-init", &opts->chatpad_no_init)
    ("chatpad-debug",   &opts->chatpad_debug)

    ("headset",         &opts->headset)
    ("headset-debug",   &opts->headset_debug)
    ("headset-dump",    &opts->headset_dump)
    ("headset-play",    &opts->headset_play)
    ("ui-clear",        std::bind(&Options::set_ui_clear, opts), std::function<void ()>())
    ;

  m_ini.section("xboxdrv-daemon")
    ("detach",
     std::bind(&Options::set_daemon_detach, opts, true),
     std::bind(&Options::set_daemon_detach, opts, false))
    ("dbus", std::bind(&Options::set_dbus_mode, opts, _1))
    ("pid-file",      &opts->pid_file)
    ("on-connect",    &opts->on_connect)
    ("on-disconnect", &opts->on_disconnect)
    ;

  m_ini.section("modifier",     std::bind(&CommandLineParser::set_modifier,     this, _1, _2));
  m_ini.section("ui-buttonmap", std::bind(&CommandLineParser::set_keymap, this, _1, _2)); // backward compatibility
  m_ini.section("ui-axismap",   std::bind(&CommandLineParser::set_absmap,   this, _1, _2)); // backward compatibility
  m_ini.section("absmap",       std::bind(&CommandLineParser::set_absmap,   this, _1, _2));
  m_ini.section("keymap",       std::bind(&CommandLineParser::set_keymap, this, _1, _2));
  m_ini.section("relmap",       std::bind(&CommandLineParser::set_absmap,   this, _1, _2));


  m_ini.section("buttonmap", std::bind(&CommandLineParser::set_buttonmap, this, _1, _2));
  m_ini.section("axismap",   std::bind(&CommandLineParser::set_axismap,   this, _1, _2));

  m_ini.section("autofire",   std::bind(&CommandLineParser::set_autofire, this, _1, _2));
  m_ini.section("relative-axis",   std::bind(&CommandLineParser::set_relative_axis, this, _1, _2));
  m_ini.section("calibration",   std::bind(&CommandLineParser::set_calibration, this, _1, _2));
  m_ini.section("axis-sensitivity",   std::bind(&CommandLineParser::set_axis_sensitivity, this, _1, _2));
  m_ini.section("device-name", std::bind(&CommandLineParser::set_device_name, this, _1, _2));
  m_ini.section("device-usbid", std::bind(&CommandLineParser::set_device_usbid, this, _1, _2));

  for(int controller = 0; controller <= 9; ++controller)
  {
    for(int config = 0; config <= 9; ++config)
    {
      m_ini.section(fmt::format("controller{}/config{}/modifier", controller, config),
                    std::bind(&CommandLineParser::set_modifier_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/keymap", controller, config),
                    std::bind(&CommandLineParser::set_keymap_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/absmap", controller, config),
                    std::bind(&CommandLineParser::set_absmap_n, this, controller, config, _1, _2));

      m_ini.section(fmt::format("controller{}/config{}/buttonmap", controller, config),
                    std::bind(&CommandLineParser::set_buttonmap_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/axismap", controller, config),
                    std::bind(&CommandLineParser::set_axismap_n,   this, controller, config, _1, _2));

      m_ini.section(fmt::format("controller{}/config{}/autofire", controller, config),
                    std::bind(&CommandLineParser::set_autofire_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/relative-axis", controller, config),
                    std::bind(&CommandLineParser::set_relative_axis_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/calibration", controller, config),
                    std::bind(&CommandLineParser::set_calibration_n, this, controller, config, _1, _2));
      m_ini.section(fmt::format("controller{}/config{}/axis-sensitivity", controller, config),
                    std::bind(&CommandLineParser::set_axis_sensitivity_n, this, controller, config, _1, _2));
    }
  }

  m_ini.section("evdev-absmap", std::bind(&CommandLineParser::set_evdev_absmap, this, _1, _2));
  m_ini.section("evdev-keymap", std::bind(&CommandLineParser::set_evdev_keymap, this, _1, _2));
  m_ini.section("evdev-relmap", std::bind(&CommandLineParser::set_evdev_keymap, this, _1, _2));
}

void
CommandLineParser::parse_args(int argc, char** argv, Options* options)
{
  Options& opts = *options;

  init_ini(options);
  m_options = options;

  init_argp(argc, argv);
  for(auto const& opt : m_argp.parse_args(argc, argv))
  {
    try
    {
      apply_opt(opt, opts);
    }
    catch(std::exception const& err)
    {
      std::ostringstream out;
      out << "error: invalid argument ";
      if (opt.argument.empty())
      {
        out << "'" << opt.option << "'";
      }
      else
      {
        out << "'" << opt.option << " " << opt.argument << "'";
      }
      out << "\n    " << err.what();
      throw std::runtime_error(out.str());
    }
  }

  options->finish();
}

void
CommandLineParser::apply_opt(argparser::ParsedOption const& opt, Options& opts)
{
  switch (opt.key)
    {
      case OPTION_HELP:
        opts.mode = Options::PRINT_HELP;
        break;

      case OPTION_VERSION:
        opts.mode = Options::PRINT_VERSION;
        break;

      case OPTION_VERBOSE:
        opts.set_verbose();
        break;

      case OPTION_QUIET:
        opts.quiet = true;
        break;

      case OPTION_SILENT:
        opts.silent = true;
        break;

      case OPTION_DEBUG:
        opts.set_debug();
        break;

      case OPTION_USB_DEBUG:
        opts.set_usb_debug();
        break;

      case OPTION_PRIORITY:
        opts.set_priority(opt.argument);
        break;

      case OPTION_DAEMON:
        opts.set_daemon();
        break;

      case OPTION_DAEMON_MATCH:
        opts.set_match(opt.argument);
        break;

      case OPTION_DAEMON_MATCH_GROUP:
        opts.set_match_group(opt.argument);
        break;

      case OPTION_WRITE_CONFIG:
        {
          opts.instant_exit = true;

          std::ofstream out(opt.argument.c_str());
          if (!out)
          {
            std::ostringstream str;
            str << "Couldn't create " << opt.argument;
            throw std::runtime_error(str.str());
          }
          else
          {
            // FIXME: implement me
            m_ini.save(out);
          }
        }
        break;

      case OPTION_CONFIG_OPTION:
        {
          std::string name, value;
          split_string_at(opt.argument, '=', &name, &value);

          yaini::SchemaBuilder builder(m_ini);
          builder.send_section("xboxdrv");
          builder.send_pair(name, value);
        }
        break;

      case OPTION_CONFIG:
        read_config_file(opt.argument);
        break;

      case OPTION_ALT_CONFIG:
        read_alt_config_file(opt.argument);
        break;

      case OPTION_TEST_RUMBLE:
        opts.rumble = true;
        break;

      case OPTION_RUMBLE:
        if (sscanf(opt.argument.c_str(), "%d,%d", &opts.rumble_l, &opts.rumble_r) == 2)
        {
          opts.rumble_l = std::max(0, std::min(255, opts.rumble_l));
          opts.rumble_r = std::max(0, std::min(255, opts.rumble_r));
        }
        else
        {
          raise_exception(std::runtime_error, opt.option << " expected an argument in form INT,INT");
        }
        break;

      case OPTION_FF_DEVICE:
        opts.set_ff_device(opt.argument);
        break;

      case OPTION_QUIT:
        opts.instant_exit = true;
        break;

      case OPTION_TIMEOUT:
        opts.timeout = str2int(opt.argument);
        break;

      case OPTION_NO_UINPUT:
        opts.no_uinput = true;
        break;

      case OPTION_MIMIC_XPAD:
        opts.set_mimic_xpad();
        break;

      case OPTION_MIMIC_XPAD_WIRELESS:
        opts.set_mimic_xpad_wireless();
        break;

      case OPTION_TYPE:
        if (opt.argument == "xbox")
        {
          opts.gamepad_type = GAMEPAD_XBOX;
        }
        else if (opt.argument == "xbox-mat")
        {
          opts.gamepad_type = GAMEPAD_XBOX_MAT;
        }
        else if (opt.argument == "xbox360")
        {
          opts.gamepad_type = GAMEPAD_XBOX360;
        }
        else if (opt.argument == "xbox360-guitar")
        {
          opts.gamepad_type = GAMEPAD_XBOX360_GUITAR;
        }
        else if (opt.argument == "xbox360-wireless")
        {
          opts.gamepad_type = GAMEPAD_XBOX360_WIRELESS;
        }
        else if (opt.argument == "firestorm")
        {
          opts.gamepad_type = GAMEPAD_FIRESTORM;
        }
        else if (opt.argument == "firestorm-vsb")
        {
          opts.gamepad_type = GAMEPAD_FIRESTORM_VSB;
        }
        else if (opt.argument == "saitek-p2500")
        {
          opts.gamepad_type = GAMEPAD_SAITEK_P2500;
        }
        else if (opt.argument == "logitech-f310")
        {
          opts.gamepad_type = GAMEPAD_LOGITECH_F310;
        }
        else if (opt.argument == "playstation3-usb")
        {
          opts.gamepad_type = GAMEPAD_PLAYSTATION3_USB;
        }
        else if (opt.argument == "generic-usb")
        {
          opts.gamepad_type = GAMEPAD_GENERIC_USB;
        }
        else
        {
          raise_exception(std::runtime_error, "unknown type: " << opt.argument << '\n'
                          << "Possible types are:\n"
                          << " * xbox\n"
                          << " * xbox-mat\n"
                          << " * xbox360\n"
                          << " * xbox360-guitar\n"
                          << " * xbox360-wireless\n"
                          << " * firestorm\n"
                          << " * firestorm-vsb\n"
                          << " * saitek-p2500\n"
                          << " * logitech-f310\n"
                          << " * generic-usb\n");
        }
        break;

      case OPTION_CHATPAD:
        opts.chatpad = true;
        break;

      case OPTION_CHATPAD_NO_INIT:
        opts.chatpad_no_init = true;
        opts.chatpad = true;
        break;

      case OPTION_CHATPAD_DEBUG:
        opts.chatpad_debug = true;
        break;

      case OPTION_HEADSET:
        opts.headset = true;
        break;

      case OPTION_HEADSET_DUMP:
        opts.headset = true;
        opts.headset_dump = opt.argument;
        break;

      case OPTION_HEADSET_PLAY:
        opts.headset = true;
        opts.headset_play = opt.argument;
        break;

      case OPTION_FORCE_FEEDBACK:
        opts.get_controller_slot().set_force_feedback(true);
        break;

      case OPTION_RUMBLE_GAIN:
        opts.rumble_gain = to_number(255, opt.argument);
        break;

      case OPTION_MODIFIER:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_modifier, this, _1, _2));
        break;

      case OPTION_BUTTONMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_buttonmap, this, _1, _2));
        break;

      case OPTION_AXISMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_axismap, this, _1, _2));
        break;

      case OPTION_DEVICE_USBID:
        opts.set_device_usbid(opt.argument);
        break;

      case OPTION_DEVICE_USBIDS:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_device_usbid, this, _1, _2));
        break;

      case OPTION_DEVICE_NAME:
        opts.set_device_name(opt.argument);
        break;

      case OPTION_DEVICE_NAMES:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_device_name, this, _1, _2));
        break;

      case OPTION_NEXT_CONFIG:
        opts.next_config();
        break;

      case OPTION_NEXT_CONTROLLER:
        opts.next_controller();
        break;

      case OPTION_CONTROLLER_SLOT:
        opts.controller_slot = str2int(opt.argument);
        break;

      case OPTION_CONFIG_SLOT:
        opts.config_slot = str2int(opt.argument);
        break;

      case OPTION_TOGGLE:
        opts.set_toggle_button(opt.argument);
        break;

      case OPTION_UI_CLEAR:
        opts.set_ui_clear();
        break;

      case OPTION_ABSMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_absmap, this, _1, _2));
        break;

      case OPTION_KEYMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_keymap, this, _1, _2));
        break;

      case OPTION_MOUSE:
        mouse();
        break;

      case OPTION_GUITAR:
        opts.get_controller_options().uinput.guitar();
        break;

      case OPTION_DETACH_KERNEL_DRIVER:
        opts.detach_kernel_driver = true;
        break;

      case OPTION_EVDEV:
        opts.evdev_device = opt.argument;
        break;

      case OPTION_EVDEV_DEBUG:
        opts.evdev_debug = true;
        break;

      case OPTION_EVDEV_NO_GRAB:
        opts.evdev_grab = false;
        break;

      case OPTION_EVDEV_ABSMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_evdev_absmap, this, _1, _2));
        break;

      case OPTION_EVDEV_KEYMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_evdev_keymap, this, _1, _2));
        break;

      case OPTION_EVDEV_RELMAP:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_evdev_relmap, this, _1, _2));
        break;

      case OPTION_WIIMOTE:
        opts.wiimote = true;
        break;

      case OPTION_ID:
        opts.controller_id = str2int(opt.argument);
        break;

      case OPTION_WID:
        opts.wireless_id = str2int(opt.argument);
        if (opts.wireless_id < 0 || opts.wireless_id > 3)
        {
          throw std::runtime_error("wireless id must be within 0 and 3");
        }
        break;

      case OPTION_LED:
        if (opt.argument == "help")
        {
          opts.mode = Options::PRINT_LED_HELP;
        }
        else
        {
          opts.set_led(opt.argument);
        }
        break;

      case OPTION_NO_EXTRA_DEVICES:
        opts.extra_devices = false;
        break;

      case OPTION_NO_EXTRA_EVENTS:
        opts.extra_events = false;
        break;

      case OPTION_DPAD_ONLY:
        opts.set_dpad_only();
        break;

      case OPTION_DPAD_AS_BUTTON:
        opts.set_dpad_as_button();
        break;

      case OPTION_DEADZONE:
        set_deadzone(opt.argument);
        break;

      case OPTION_DEADZONE_TRIGGER:
        set_deadzone_trigger(opt.argument);
        break;

      case OPTION_TRIGGER_AS_BUTTON:
        opts.set_trigger_as_button();
        break;

      case OPTION_TRIGGER_AS_ZAXIS:
        opts.set_trigger_as_zaxis();
        break;

      case OPTION_AUTOFIRE:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_autofire, this, _1, _2));
        break;

      case OPTION_CALIBRARIOTION:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_calibration, this, _1, _2));
        break;

      case OPTION_RELATIVE_AXIS:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_relative_axis, this, _1, _2));
        break;

      case OPTION_AXIS_SENSITIVITY:
        process_name_value_string(opt.argument, std::bind(&CommandLineParser::set_axis_sensitivity, this, _1, _2));
        break;

      case OPTION_FOUR_WAY_RESTRICTOR:
        set_four_way_restrictor();
        break;

      case OPTION_DPAD_ROTATION:
        set_dpad_rotation(opt.argument);
        break;

      case OPTION_SQUARE_AXIS:
        set_square_axis();
        break;

      case OPTION_HELP_LED:
        opts.mode = Options::PRINT_LED_HELP;
        break;

      case OPTION_DAEMON_DETACH:
        opts.set_daemon_detach(true);
        break;

      case OPTION_DAEMON_PID_FILE:
        opts.pid_file = opt.argument;
        break;

      case OPTION_DAEMON_ON_CONNECT:
        opts.on_connect = opt.argument;
        break;

      case OPTION_DAEMON_ON_DISCONNECT:
        opts.on_disconnect = opt.argument;
        break;

      case OPTION_DAEMON_DBUS:
        opts.set_dbus_mode(opt.argument);
        break;

      case OPTION_DAEMON_NO_DBUS:
        opts.dbus = Options::kDBusDisabled;
        break;

      case OPTION_DEVICE_BY_ID:
        {
          unsigned int tmp_product_id;
          unsigned int tmp_vendor_id;
          if (sscanf(opt.argument.c_str(), "%x:%x", &tmp_vendor_id, &tmp_product_id) == 2)
          {
            opts.vendor_id  = tmp_vendor_id;
            opts.product_id = tmp_product_id;
          }
          else
          {
            raise_exception(std::runtime_error, opt.option << " expected an argument in form PRODUCT:VENDOR (i.e. 046d:c626)");
          }
          break;
        }

      case OPTION_DEVICE_BY_PATH:
        {
          char busid[4] = { '\0' };
          char devid[4] = { '\0' };

          if (sscanf(opt.argument.c_str(), "%3s:%3s", busid, devid) != 2)
          {
            raise_exception(std::runtime_error, opt.option << " expected an argument in form BUS:DEV (i.e. 006:003)");
          }
          else
          {
            opts.busid = busid;
            opts.devid = devid;
          }
        }
        break;

      case OPTION_GENERIC_USB_SPEC:
        set_generic_usb_spec(opt.argument);
        break;

      case OPTION_LIST_SUPPORTED_DEVICES:
        opts.mode = Options::RUN_LIST_SUPPORTED_DEVICES;
        break;

      case OPTION_LIST_SUPPORTED_DEVICES_XPAD:
        opts.mode = Options::RUN_LIST_SUPPORTED_DEVICES_XPAD;
        break;

      case OPTION_LIST_CONTROLLER:
        opts.mode = Options::RUN_LIST_CONTROLLER;
        break;

      case OPTION_HELP_DEVICES:
        opts.mode = Options::PRINT_HELP_DEVICES;
        break;

      case OPTION_LIST_ALL:
        opts.mode = Options::PRINT_ENUMS;
        opts.list_enums |= Options::LIST_ALL;
        break;

      case OPTION_LIST_ABS:
        opts.mode = Options::PRINT_ENUMS;
        opts.list_enums |= Options::LIST_ABS;
        break;

      case OPTION_LIST_REL:
        opts.mode = Options::PRINT_ENUMS;
        opts.list_enums |= Options::LIST_REL;
        break;

      case OPTION_LIST_KEY:
        opts.mode = Options::PRINT_ENUMS;
        opts.list_enums |= Options::LIST_KEY;
        break;

      case OPTION_LIST_X11KEYSYM:
        opts.mode = Options::PRINT_ENUMS;
        opts.list_enums |= Options::LIST_X11KEYSYM;
        break;

      case argparser::ArgumentType::REST:
        opts.exec.push_back(opt.argument);
        break;

      default:
        raise_exception(std::runtime_error, "unknown command line option: " << opt.option);
        break;
    }
  }

void
CommandLineParser::print_help() const
{
  m_argp.print_help(std::cout);
}

void
CommandLineParser::print_led_help() const
{
  std::cout <<
    "Possible values for '--led VALUE' are:\n\n"
    "   0: off\n"
    "   1: all blinking\n"
    "   2: 1/top-left blink, then on\n"
    "   3: 2/top-right blink, then on\n"
    "   4: 3/bottom-left blink, then on\n"
    "   5: 4/bottom-right blink, then on\n"
    "   6: 1/top-left on\n"
    "   7: 2/top-right on\n"
    "   8: 3/bottom-left on\n"
    "   9: 4/bottom-right on\n"
    "  10: rotate\n"
    "  11: blink\n"
    "  12: blink slower\n"
    "  13: rotate with two lights\n"
    "  14: blink\n"
    "  15: blink once\n"
            << std::endl;
}

void
CommandLineParser::print_version() const
{
  std::cout
    << "xboxdrv " PACKAGE_VERSION " - http://pingus.seul.org/~grumbel/xboxdrv/\n"
    << "Copyright © 2008-2015 Ingo Ruhnke <grumbel@gmail.com>\n"
    << "Licensed under GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
    << "This program comes with ABSOLUTELY NO WARRANTY.\n"
    << "This is free software, and you are welcome to redistribute it under certain\n"
    << "conditions; see the file COPYING for details.\n";
}

void
CommandLineParser::set_modifier(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().modifier.push_back(ModifierOption(name, value));
}

void
CommandLineParser::set_device_usbid(const std::string& name, const std::string& value)
{
  uint32_t devid = uinpp::parse_device_id(name);
  m_options->uinput_device_usbids[devid] = uinpp::parse_input_id(value);
}

void
CommandLineParser::set_device_name(const std::string& name, const std::string& value)
{
  uint32_t devid = uinpp::parse_device_id(name);
  m_options->uinput_device_names[devid] = value;
}

void
CommandLineParser::set_keymap(const std::string& name, const std::string& value)
{
  set_keymap_helper(m_options->get_controller_options().uinput.get_btn_map(),
                    name, value);
}

void
CommandLineParser::set_keymap_helper(ButtonMapOptions& btn_map, const std::string& name, const std::string& value)
{
  std::vector<std::string> lst = string_split(name, "^");

  int idx = 0;
  for(std::vector<std::string>::iterator t = lst.begin(); t != lst.end(); ++t, ++idx)
  {
    switch(idx)
    {
      case 0: // shift+key portion
        btn_map.push_back(ButtonMapOption(*t, value, get_directory_context()));
        break;

      default:
        btn_map.back().add_filter(*t);
        break;
    }
  }
}

void
CommandLineParser::set_absmap(const std::string& name, const std::string& value)
{
  set_absmap_helper(m_options->get_controller_options().uinput.get_axis_map(),
                    name, value);
}

void
CommandLineParser::set_absmap_helper(AxisMapOptions& axis_map, const std::string& name, const std::string& value)
{
  std::vector<std::string> lst = string_split(name, "^");

  int idx = 0;
  for(std::vector<std::string>::iterator t = lst.begin(); t != lst.end(); ++t, ++idx)
  {
    switch(idx)
    {
      case 0: // shift+key portion
        axis_map.push_back(AxisMapOption(*t, value, get_directory_context()));
        break;

      default:
        axis_map.back().add_filter(*t);
        break;
    }
  }
}

void
CommandLineParser::set_axismap(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().axismap.push_back(AxisMappingOption(name, value));
}

void
CommandLineParser::set_buttonmap(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().buttonmap.push_back(ButtonMappingOption(name, value));
}

void
CommandLineParser::set_evdev_absmap(const std::string& name, const std::string& value)
{
  m_options->evdev_absmap[str2abs(name)] = value;
}

void
CommandLineParser::set_evdev_keymap(const std::string& name, const std::string& value)
{
  m_options->evdev_keymap[str2key(name)] = value;
}

void
CommandLineParser::set_evdev_relmap(const std::string& name, const std::string& value)
{
  m_options->evdev_relmap[str2rel(name)] = value;
}

void
CommandLineParser::set_relative_axis(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().relative_axis_map[name]
    = AxisFilterPtr(new RelativeAxisFilter(str2int(value)));
}

void
CommandLineParser::set_autofire(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().autofire_map[name]
    = ButtonFilterPtr(new AutofireButtonFilter(str2int(value), 0));
}

void
CommandLineParser::set_calibration(const std::string& name, const std::string& value)
{
  std::vector<std::string> args = string_split(name, ":");

  if (args.size() != 3)
  {
    throw std::runtime_error("calibration requires MIN:CENTER:MAX as argument");
  }
  else
  {
    m_options->get_controller_options().calibration_map[name]
      = AxisFilterPtr(new CalibrationAxisFilter(str2int(args[0]),
                                                str2int(args[1]),
                                                str2int(args[2])));
  }
}

void
CommandLineParser::set_axis_sensitivity(const std::string& name, const std::string& value)
{
  m_options->get_controller_options().sensitivity_map[name]
    = AxisFilterPtr(new SensitivityAxisFilter(str2float(value)));
}

void
CommandLineParser::set_deadzone(const std::string& value)
{
  m_options->get_controller_options().deadzone = to_number(32767, value);
}

void
CommandLineParser::set_deadzone_trigger(const std::string& value)
{
  m_options->get_controller_options().deadzone_trigger = to_number(255, value);
}

void
CommandLineParser::set_square_axis()
{
  m_options->get_controller_options().square_axis = true;
}

void
CommandLineParser::set_four_way_restrictor()
{
  m_options->get_controller_options().four_way_restrictor = true;
}

void
CommandLineParser::set_dpad_rotation(const std::string& value)
{
  int degree = str2int(value);
  degree /= 45;
  degree %= 8;
  if (degree < 0) degree += 8;

  m_options->get_controller_options().dpad_rotation = degree;
}

void
CommandLineParser::read_buildin_config_file(const std::string& filename,
                                            const char* data, unsigned int data_len)
{
  log_info("reading 'buildin://{}'", filename);

  std::string str(data, data_len);
  std::istringstream in(str);
  if (!in)
  {
    raise_exception(std::runtime_error, "couldn't open: buildin://" << filename);
  }
  else
  {
    yaini::SchemaBuilder builder(m_ini);
    yaini::Parser parser(in, builder, filename);
    parser.run();
  }
}

void
CommandLineParser::read_config_file(const std::string& filename)
{
  log_info("reading '{}'", filename);

  std::ifstream in(filename.c_str());
  if (!in)
  {
    raise_exception(std::runtime_error, "couldn't open: " << filename);
  }
  else
  {
    m_directory_context.push_back(path::dirname(filename));

    yaini::SchemaBuilder builder(m_ini);
    yaini::Parser parser(in, builder, filename);
    parser.run();

    m_directory_context.pop_back();
  }
}

void
CommandLineParser::read_alt_config_file(const std::string& filename)
{
  m_options->next_config();
  read_config_file(filename);
}

void
CommandLineParser::set_keymap_n(int controller, int config, const std::string& name, const std::string& value)
{
  set_keymap_helper(m_options->controller_slots[controller].get_options(config).uinput.get_btn_map(),
                    name, value);
}

void
CommandLineParser::set_absmap_n(int controller, int config, const std::string& name, const std::string& value)
{
  set_absmap_helper(m_options->controller_slots[controller].get_options(config).uinput.get_axis_map(),
                    name, value);
}

void
CommandLineParser::set_modifier_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config).modifier.push_back(ModifierOption(name, value));
}

void
CommandLineParser::set_axismap_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config).axismap.push_back(AxisMappingOption(name, value));
}

void
CommandLineParser::set_buttonmap_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config).buttonmap.push_back(ButtonMappingOption(name, value));
}

void
CommandLineParser::set_relative_axis_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config)
    .relative_axis_map[name] = AxisFilterPtr(new RelativeAxisFilter(str2int(value)));
}

void
CommandLineParser::set_autofire_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config)
    .autofire_map[name] = ButtonFilterPtr(new AutofireButtonFilter(str2int(value), 0));
}

void
CommandLineParser::set_calibration_n(int controller, int config, const std::string& name, const std::string& value)
{
  // FIXME: not implemented
  assert(false && "implement me");
}

void
CommandLineParser::set_axis_sensitivity_n(int controller, int config, const std::string& name, const std::string& value)
{
  m_options->controller_slots[controller].get_options(config)
    .sensitivity_map[name] = AxisFilterPtr(new SensitivityAxisFilter(str2float(value)));
}

void
CommandLineParser::mouse()
{
  read_buildin_config_file("examples/mouse.xboxdrv",
                           xboxdrv_vfs::examples_mouse_xboxdrv,
                           sizeof(xboxdrv_vfs::examples_mouse_xboxdrv));
}

void
CommandLineParser::set_generic_usb_spec(const std::string& spec)
{
  m_options->m_generic_usb_specs.push_back(Options::GenericUSBSpec::from_string(spec));
}

std::string
CommandLineParser::get_directory_context() const
{
  if (m_directory_context.empty())
  {
    return std::string();
  }
  else
  {
    return m_directory_context.back();
  }
}

/* EOF */

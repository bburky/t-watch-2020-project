Initial PlatformIO project for TTGO-T-Watch-2020 based of SimpleWatch example

A BLE UART device is advertised to emulate a bangle.js/Espruino to receive [json messages](https://www.espruino.com/Gadgetbridge) from [Gadgetbridge](https://gadgetbridge.org/). This allows receiving all notifications from an Android phone.

Time updates from Gadgetbridge are parsed and applied to the RTC and system time.

Gadgetbridge `notify` messages are shown on screen in a popup message. Other message types are not yet handled.

`gui.cpp` was refactored slightly to separate GUI header and class implementation. Class definitions are now in `gui.h` so that other files may reference the GUI classes.
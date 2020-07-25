Initial PlatformIO project for TTGO-T-Watch-2020 based of SimpleWatch example

A BLE UART device is advertised to emulate a bangle.js/Espruino to receive [json messages](https://www.espruino.com/Gadgetbridge) from [Gadgetbridge](https://gadgetbridge.org/). This allows receiving all notifications from an Android phone.

Currently the BLE connection is lost when the screen turns off (and goes into low power mode?) and the JSON is not yet decoded. The messages are only written to the serial port for now and not yet shown on screen.

Time updates from Gadgetbridge are parsed and applied to the RTC and system time.

`gui.cpp` was refactored slightly to separate GUI header and class implementation. Class definitions are now in `gui.h` so that other files may reference the GUI classes.
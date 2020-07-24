Initial PlatformIO project for TTGO-T-Watch-2020 based of SimpleWatch example

A BLE UART device is advertised to emulate a bangle.js/Espruino to receive [json messages](https://www.espruino.com/Gadgetbridge) from [Gadgetbridge](https://gadgetbridge.org/). This allows receiving all notifications from an Android phone.

Currently the BLE connection is lost when the screen turns off (and goes into low power mode?) and the JSON is not yet decoded. The messages are only written to the serial port for now and not yet shown on screen.

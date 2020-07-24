/* 
    Initially based on: https://github.com/nkolban/esp32-snippets/blob/fe3d318acddf87c6918944f24e8b899d63c816dd/cpp_utils/tests/BLETests/Arduino/BLE_uart/BLE_uart.ino

    Gadgetbridge supports sending JSONP-style messages to Bangle.js/Espruino
    devices. It is possible to fake a Espruino by advertising the BLE service
    UUID of 6E400001-B5A3-F393-E0A9-E50E24DCCA9E with a name of "Espruino".
    See the code for more detail:
    https://codeberg.org/Freeyourgadget/Gadgetbridge/src/branch/master/app/src/main/java/nodomain/freeyourgadget/gadgetbridge/devices/banglejs/BangleJSCoordinator.java

    The Espruino Gadgetbridge JSON protocol is documented here:
    https://www.espruino.com/Gadgetbridge
*/

#include "Arduino.h"
#include <limits.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
uint8_t txValue = 0;
bool bleConnected = false;
bool bleEnabled = false;

#define MAX_MESSAGE_SIZE 512
String message;

void processMessage();

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Serial.println("BLE Connected");
        bleConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        Serial.println("BLE Disconnected");
        bleConnected = false;

        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("BLE advertising...");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0)
        {
            for (int i = 0; i < rxValue.length(); i++) {
                if (rxValue[i] == 0x10) {
                    if (message.length()) {
                        Serial.printf("BLE: Discarding %d bytes\n", message.length());
                    }
                    message.clear();
                } else if (rxValue[i] == '\n') {
                   if (message.length()+1 > MAX_MESSAGE_SIZE) {
                        message.clear();
                        Serial.println("BLE Error: Message too long");
                        return;
                    }
                    message[message.length()] = 0;
                    processMessage();
                    message.clear();
                } else {
                    message += rxValue[i];
                    if (message.length() > MAX_MESSAGE_SIZE) {
                        message.clear();
                        Serial.println("BLE Error: Message too long");
                        return;
                    }
                }
            }
        }
    }
};

void processMessage() {
    // 6 characters: GB({})
    if (message.length() >= 6 && message[0] == 'G' && message[1] == 'B' && message[2] == '(' && message[message.length()-1] == ')') {
        Serial.printf("BLE GB JSON: %s\n", message.substring(3, message.length()-1).c_str());
    } else if (message.startsWith("setTime(")) {
        Serial.printf("BLE set time: %ld\n", message.substring(8).toInt());
    } else {
        Serial.printf("BLE other data: %s\n", message.c_str());
    }
}

void setupBle()
{
    bleEnabled = true;

    // Create the BLE Device
    // Name needs to match filter in Gadgetbridge's banglejs getSupportedType() function.
    // This is too long I think:
    // BLEDevice::init("Espruino Gadgetbridge Compatible Device");
    BLEDevice::init("Espruino");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());
    pServer->getAdvertising()->start();
    Serial.println("BLE advertising...");
}
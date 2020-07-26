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

#include "config.h"
#include "Arduino.h"
#include <limits.h>
#include <LilyGoWatch.h>
#include <time.h>
#include "gui.h"
#include "gadgetbridge.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID BLEUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E") // UART service UUID
#define CHARACTERISTIC_UUID_RX BLEUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
#define CHARACTERISTIC_UUID_TX BLEUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

static MBox *mbox = nullptr;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
uint8_t txValue = 0;
bool bleConnected = false;
bool bleEnabled = false;
bool blePairing = false;
bool restoreMenubars = true;

#define MAX_MESSAGE_SIZE 512
String message;

void processMessage();
void destroyMBox();

class MySecurity : public BLESecurityCallbacks {

    uint32_t onPassKeyRequest(){
        Serial.println("BLE: PassKeyRequest");
        // TODO: when is this used?
        return 123456;
    }
    void onPassKeyNotify(uint32_t pass_key){
        blePairing = true;

        char format[256];
        snprintf(format, sizeof(format), "Bluetooth Pairing Request\n\nPIN: %06d", pass_key);
        Serial.println(format);
        delete mbox;
        mbox = new MBox;
        mbox->create(format, [](lv_obj_t *obj, lv_event_t event) {
            if (event == LV_EVENT_VALUE_CHANGED) {
                destroyMBox();
            }
        });
    }
    bool onConfirmPIN(uint32_t pass_key){
        Serial.printf("BLE: The passkey YES/NO number :%06d\n", pass_key);
        // vTaskDelay(5000);
        // return true;
        // TODO: when is this used?
        return false;
    }
    bool onSecurityRequest(){
        Serial.println("BLE: SecurityRequest");
        // TODO: when is this used?
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
        char format[256];
        snprintf(format, sizeof(format), "Bluetooth pairing %s", cmpl.success ? "successful" : "unsuccessful");
        Serial.println(format);

        if(cmpl.success){
            uint16_t length;
            esp_ble_gap_get_whitelist_size(&length);
            ESP_LOGD(LOG_TAG, "size: %d", length);
        } else {
            // Restart advertising
            pServer->startAdvertising();
        }

        if (blePairing) {
            blePairing = false;
            delete mbox;
            mbox = new MBox;
            mbox->create(format, [](lv_obj_t *obj, lv_event_t event) {
                if (event == LV_EVENT_VALUE_CHANGED) {
                    destroyMBox();
                }
            });
        }
    }
};

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Serial.println("BLE Connected");
        bleConnected = true;
        StatusBar *statusBar = StatusBar::getStatusBar();
        statusBar->show(LV_STATUS_BAR_BLUETOOTH);
    };

    void onDisconnect(BLEServer *pServer)
    {
        Serial.println("BLE Disconnected");
        bleConnected = false;
        StatusBar *statusBar = StatusBar::getStatusBar();
        statusBar->hidden(LV_STATUS_BAR_BLUETOOTH);
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
    if (message.startsWith("GB(")) {
        Serial.printf("BLE GB JSON: %s\n", message.substring(3, message.length()-1).c_str());
        process_gadgetbridge_json(message.substring(3, message.length()-1).c_str());
    } else if (message.startsWith("setTime(")) {
        time_t time = message.substring(8).toInt();
        int tz_str_offset = message.indexOf("E.setTimeZone(");
        int tz = message.substring(tz_str_offset+14).toInt();

        struct tm timeinfo;
        localtime_r(&time, &timeinfo);
        Serial.printf("BLE set time %ld %d: %d-%d-%d/%d:%d:%d\n", time, tz, timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, (timeinfo.tm_hour + 24 + tz) % 24, timeinfo.tm_min, timeinfo.tm_sec);

        TTGOClass *ttgo = TTGOClass::getWatch();
        ttgo->rtc->setDateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, (timeinfo.tm_hour + 24 + tz) % 24, timeinfo.tm_min, timeinfo.tm_sec);
        ttgo->rtc->syncToSystem();
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

    // Enable encryption
    BLEServer* pServer = BLEDevice::createServer();
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);
    BLEDevice::setSecurityCallbacks(new MySecurity());

    // Enable authentication
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    pSecurity->setCapability(ESP_IO_CAP_OUT);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());
    // Trying setting the advertising interval to 1s to get better battery life?
    pServer->getAdvertising()->setMinInterval(1000);
    pServer->getAdvertising()->setMaxInterval(1500);
    pServer->getAdvertising()->start();
    Serial.println("BLE advertising...");
}

void bluetooth_event_cb() {
    // Actually, bluetooth is always advertising currently. This menu button isn't really needed right now.
    restoreMenubars = true;
    pServer->getAdvertising()->start();

    // static const char *btns[] = {"Stop", ""};
    delete mbox;
    mbox = new MBox;
    mbox->create("Connect a Bluetooth Device\n\nBluetooth is in discoverable mode now.", [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_VALUE_CHANGED) {
            // pServer->getAdvertising()->stop();
            destroyMBox();
        }
    });
    // mbox->setBtn(btns);
}

void destroyMBox() {
    delete mbox;
    mbox = nullptr;
    if (restoreMenubars) {
        MenuBar *menubars = MenuBar::getMenuBar();
        menubars->hidden(false);
    }
}
#include "ble_manager.h"

#include <Arduino.h>
#include <BLEDevice.h>

static BLEUUID serviceNotifyUuid("0000ffe0-0000-1000-8000-00805f9b34fb");

namespace
{
    BLEScan *pBLEScan = nullptr;
    BLEClient *pClient = nullptr;
    BLERemoteService *pRemoteService = nullptr;
    BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;

static BLEUUID charReadUUID("ffe1");

    enum class BLEState
    {
        Scanning,
        Connecting,
        Connected
    };

    BLEState state = BLEState::Scanning;
    bool scanning = false;

    BLEAdvertisedDevice *pDevice = nullptr;
}

static BleNotifyCallback notifyCallback = nullptr;

void bleSetNotifyCallback(BleNotifyCallback callback)
{
    notifyCallback = callback;
}

static void notifyCB(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
    if (notifyCallback)
        notifyCallback(pData, length);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (!advertisedDevice.haveServiceUUID())
            return;

        if (!advertisedDevice.isAdvertisingService(serviceNotifyUuid))
            return;

        Serial.println();
        Serial.println("*** JK BMS FOUND ***");

        Serial.print("Address : ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());

        Serial.print("RSSI    : ");
        Serial.println(advertisedDevice.getRSSI());

        pDevice = new BLEAdvertisedDevice(advertisedDevice);

        state = BLEState::Connecting;

        BLEDevice::getScan()->stop();
    }
};

bool bleBegin()
{
    Serial.println("BLE init");

    BLEDevice::init("");

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);

    return true;
}

void bleLoop()
{
    switch (state)
    {
        case BLEState::Scanning:

            if (!scanning)
            {
                scanning = true;

                Serial.println("Scanning...");

                pBLEScan->start(5, false);

                pBLEScan->clearResults();

                scanning = false;
            }

            break;

        case BLEState::Connecting:

           Serial.println("Connecting...");

           pClient = BLEDevice::createClient();

           if (pClient->connect(pDevice))
           {
               pClient->setMTU(517);

               Serial.print("MTU = ");
               Serial.println(pClient->getMTU());
               Serial.println("*** CONNECTED ***");

               pRemoteService = pClient->getService(serviceNotifyUuid);

               if (pRemoteService == nullptr)
               {
                   Serial.println("*** SERVICE NOT FOUND ***");

                   pClient->disconnect();
                   delete pClient;
                   pClient = nullptr;

                   delete pDevice;
                   pDevice = nullptr;

                   state = BLEState::Scanning;
               }
               else
               {
                   Serial.println("*** SERVICE FOUND ***");

                   pRemoteCharacteristic = pRemoteService->getCharacteristic(charReadUUID);

                   if (pRemoteCharacteristic == nullptr)
                   {
                       Serial.println("*** CHARACTERISTIC NOT FOUND ***");

                       pClient->disconnect();

                       delete pClient;
                       pClient = nullptr;

                       delete pDevice;
                       pDevice = nullptr;

                       state = BLEState::Scanning;
                   }
                   else
                   {
                       //Serial.println("*** CHARACTERISTIC FOUND ***");
                       //Serial.print("Can read     : ");
                       //Serial.println(pRemoteCharacteristic->canRead());

                       //Serial.print("Can write    : ");
                       //Serial.println(pRemoteCharacteristic->canWrite());

                       //Serial.print("Can write NR : ");
                       //Serial.println(pRemoteCharacteristic->canWriteNoResponse());

                       //Serial.print("Can notify   : ");
                       //Serial.println(pRemoteCharacteristic->canNotify());

                       uint8_t enable_read_handle[] = {0x01, 0x00};
                       Serial.println("*** ENABLE HANDLE ***");
                       pRemoteCharacteristic->writeValue(enable_read_handle, 2);
                       
                       pRemoteCharacteristic->registerForNotify(notifyCB);

                       Serial.println("*** NOTIFY REGISTERED ***");

                       state = BLEState::Connected;
                   }
               }
           }
           else
           {
               Serial.println("*** CONNECT FAILED ***");

               delete pClient;
               pClient = nullptr;

               delete pDevice;
               pDevice = nullptr;

               state = BLEState::Scanning;
           }

    break;

        case BLEState::Connected:
            if (pClient == nullptr || !pClient->isConnected())
            {
                Serial.println("*** DISCONNECTED ***");
        
                if (pClient)
                {
                    delete pClient;
                    pClient = nullptr;
                }
        
                if (pDevice)
                {
                    delete pDevice;
                    pDevice = nullptr;
                }
        
                state = BLEState::Scanning;
            }
            break;
    }
}

bool bleWrite(const uint8_t *data, size_t length)
{
    if (pRemoteCharacteristic == nullptr)
        return false;

    pRemoteCharacteristic->writeValue((uint8_t *)data, length);

    return true;
}

bool bleConnected()
{
    return state == BLEState::Connected;
}

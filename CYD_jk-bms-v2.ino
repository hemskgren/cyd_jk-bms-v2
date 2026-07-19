#include "ble_manager.h"
#include "display.h"
#include "jk_protocol.h"

void setup()
{
    Serial.begin(115200);

    displayBegin();
    bleBegin();
    jkProtocolBegin();
}

void loop()
{
    bleLoop();
    jkProtocolLoop();
    //displayUpdate();
    static uint32_t lastDisplay = 0;
    
    if (millis() - lastDisplay >= 500)
    {
        lastDisplay = millis();
        displayUpdate();
    }
}

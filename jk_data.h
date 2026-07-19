#pragma once

#include <stdint.h>

struct JKData {

    bool connected = false;

    uint32_t lastUpdate = 0;

    float packVoltage = 0;

    float batteryCapacity = 0;
    float soc = 0;
    float capacityAh = 0;

    float mosTemp = 0;

    uint16_t cycles = 0;

    float cellCount = 0;
    float cellVoltage[32] = {0};

    float cellMin = 0;
    float cellMax = 0;
    float cellDiff = 0;

    float mosTemperature = 0;
    float temperature1 = 0;
    float temperature2 = 0;

    float totalVoltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;

    float stateOfCharge = 0.0f;
    float remainingAh = 0.0f;

    float minCellVoltage = 0.0f;
    float maxCellVoltage = 0.0f;
    float deltaCellVoltage = 0.0f;
    float averageCellVoltage = 0.0f;


    uint32_t alarms = 0;
    char alarmText[64] = "";
};

extern JKData jkData;

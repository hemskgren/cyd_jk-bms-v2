#include "jk_protocol.h"
#include "jk_data.h"
#include <Arduino.h>

#include "ble_manager.h"

#include <vector>

static bool got_device_info = false;
static bool got_settings = false;

float cellVoltage[24];

static std::vector<uint8_t> frame_buffer_;

static uint8_t jk_state = 0;

static const uint8_t readAllCommand[] = {
    0xAA, 0x55, 0x90, 0xEB,
    0x97, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11,
    0x00, 0x00, 0x00
};

struct AlarmEntry
{
    uint16_t bit;
    const char *text;
};

static const AlarmEntry alarmTable[] =
{
    {0x0002, "Charge Under Temperature"},
    {0x0008, "Cell Under Voltage"},
    {0x0400, "Cell Count Mismatch"},
    {0x0800, "Current Sensor Fault"},
    {0x1000, "Cell Over Voltage"},
};

const char *getAlarmText(uint16_t alarms)
{
    if (alarms == 0)
        return "None";

    for (const auto &a : alarmTable)
    {
        if (alarms == a.bit)
            return a.text;
    }

    return "Unknown";
}

void printAlarmText(uint16_t alarms)
{
    if (alarms == 0)
    {
        Serial.println("  - None");
        return;
    }

    bool found = false;

    for (const auto &a : alarmTable)
    {
        if (alarms & a.bit)
        {
            Serial.printf("  - %s\n", a.text);
            found = true;
        }
    }

    if (!found)
        Serial.println("  - Unknown");
}

uint8_t crc(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += data[i];

    return sum & 0xFF;
}

static uint32_t lastRequest = 0;

void jkNotify(const uint8_t *data, size_t length)
{   
    // Start of a new frame
    if (length >= 4 &&
        data[0] == 0x55 &&
        data[1] == 0xAA &&
        data[2] == 0xEB &&
        data[3] == 0x90)
    {
        frame_buffer_.clear();
    }

    frame_buffer_.insert(frame_buffer_.end(), data, data + length);

    if (frame_buffer_.size() > 320)
    {
        Serial.println("Frame overflow - reset buffer");
        frame_buffer_.clear();
        return;
    }
    if (frame_buffer_.size() == 300 || frame_buffer_.size() == 320)
    {
        // Serial.printf("FINAL FRAME SIZE = %u\n", frame_buffer_.size());
        
        uint8_t calc = crc(frame_buffer_.data(), 299);
        uint8_t recv = frame_buffer_[299];

        if (calc == recv)
        {
            //Serial.printf("CRC OK (%u bytes)\n", frame_buffer_.size());

            decodePacket(frame_buffer_.data(), 300);
        }
        else
        {
            Serial.printf("CRC ERROR calc=%02X recv=%02X\n", calc, recv);
        }

        frame_buffer_.clear();
    }
}

void jkProtocolBegin()
{
    bleSetNotifyCallback(jkNotify);
}

uint16_t jk_get_16bit(const uint8_t *data, uint16_t offset)
{
    return ((uint16_t)data[offset]) |
           ((uint16_t)data[offset + 1] << 8);
}

uint32_t jk_get_32bit(const uint8_t *data, uint16_t offset)
{
    return ((uint32_t)data[offset]) |
           ((uint32_t)data[offset + 1] << 8) |
           ((uint32_t)data[offset + 2] << 16) |
           ((uint32_t)data[offset + 3] << 24);
}

void decodePacket(const uint8_t *data, size_t length)
{
    //Serial.printf("Packet type = %02X, length=%u\n", data[4], length);

    uint8_t offset = 0;

    // JK02 32S frames start 32 bytes later than JK04.
    if (length >= 320)
        offset = 16;
    switch (data[4])
    {
    case 0x01:
        Serial.println("Settings frame");
        got_settings = true;

        // Some JK firmware never sends a 0x03,
        // so ensure both flags are true once settings arrive.
        if (!got_device_info)
            got_device_info = true;

        jk_state = 2;

        // capacity
        if (jkData.batteryCapacity == 0.0f)
        {
            jkData.batteryCapacity = data[130] | (data[131] << 8);
            jkData.batteryCapacity /= 1000.0f;
            
            Serial.printf("Capacity = %.1f Ah\n", jkData.batteryCapacity);
        }
        break;

    case 0x02:
    {
        Serial.println("CELL DATA FRAME");

        if (jkData.cellCount == 0)
        {
            int cellCount = 0;
            for (int i = 6; i < length - 2; i += 2)
            {
                uint16_t raw = data[i] | (data[i + 1] << 8);
            
                if (raw < 2000 || raw > 4200)
                    break;
            
                // cellVoltage[cellCount++] = raw * 0.001f;
                cellCount++;
            
                if (cellCount >= 24)
                    break;
            }
            jkData.cellCount = cellCount;
            Serial.printf("CELL COUNT = %d\n", cellCount);
        }

        // cellvoltage
        for (int i = 0; i < jkData.cellCount; i++)
        {
            jkData.cellVoltage[i] =
                ((uint16_t)data[6 + i * 2] |
                ((uint16_t)data[7 + i * 2] << 8))
                / 1000.0f;
            //Serial.printf("%2d : %.3f\n", i + 1, jkData.cellVoltage[i]);
        }
        // cell avarage + delta
        float minCell = 100.0f;
        float maxCell = 0.0f;
        
        for (int i = 0; i < jkData.cellCount; i++)
        {
            float v = jkData.cellVoltage[i];
        
            if (v < minCell)
                minCell = v;
        
            if (v > maxCell)
                maxCell = v;
        }
        
        jkData.cellMin  = minCell;
        jkData.cellMax  = maxCell;
        jkData.cellDiff = maxCell - minCell;
        
        Serial.printf("Cell Min  = %.3f V\n", jkData.cellMin);
        Serial.printf("Cell Max  = %.3f V\n", jkData.cellMax);
        Serial.printf("Cell Diff = %.3f V\n", jkData.cellDiff);

        // Total voltage (offset 70)
        uint16_t rawVoltage =
            data[234] |
            (data[235] << 8);
        
        float totalVoltage = rawVoltage / 100.0f;
        jkData.totalVoltage = rawVoltage / 100.0f;
        
        Serial.printf("TOTAL VOLTAGE = %.2f V\n", totalVoltage);

        jkData.averageCellVoltage = totalVoltage / jkData.cellCount;

        // amps 
        int32_t rawCurrent =
            (int32_t)(
                data[158] |
                (data[159] << 8) |
                (data[160] << 16) |
                (data[161] << 24));

        jkData.current = rawCurrent / 1000.0f;
        
        //Serial.printf("Raw current = %ld\n", rawCurrent);
        Serial.printf("Current = %.3f A\n", jkData.current);

        // power
        jkData.power = jkData.totalVoltage * jkData.current;

        // soc
        jkData.stateOfCharge = data[173];
        
        Serial.printf("SOC = %.0f %%\n", jkData.stateOfCharge);
        //
        jkData.remainingAh =
            jkData.batteryCapacity *
            jkData.stateOfCharge / 100.0f;

        // temp
        //jkData.mosTemperature =
        //    (int16_t)(data[144] | (data[145] << 8)) / 10.0f;
        
        jkData.temperature1 =
            (int16_t)(data[162] | (data[163] << 8)) / 10.0f;
        
        jkData.temperature2 =
            (int16_t)(data[164] | (data[165] << 8)) / 10.0f;
        
        // Serial.printf("MOS Temp   = %.1f C\n", jkData.mosTemperature);
        Serial.printf("Batt Temp1 = %.1f C\n", jkData.temperature1);
        Serial.printf("Batt Temp2 = %.1f C\n", jkData.temperature2);

        // errors
        uint16_t rawAlarms =
            ((uint16_t)data[136] << 8) |
            ((uint16_t)data[137]);
        
        jkData.alarms = rawAlarms;
        
        Serial.printf("ALARMS = %u (0x%04X)\n",
                      jkData.alarms,
                      jkData.alarms);
        
        uint16_t knownBits = 0;
        bool alarmFound = false;
        
        jkData.alarmText[0] = '\0';
        
        for (const auto &a : alarmTable)
        {
            knownBits |= a.bit;
        
            if (jkData.alarms & a.bit)
            {
                Serial.printf("  - %s\n", a.text);
        
                if (alarmFound)
                    strcat(jkData.alarmText, ", ");
        
                strcat(jkData.alarmText, a.text);
        
                alarmFound = true;
            }
        }
        
        if (!alarmFound)
        {
            Serial.println("  - None");
            strcpy(jkData.alarmText, "None");
        }
        
        uint16_t unknownBits = jkData.alarms & ~knownBits;
        
        if (unknownBits)
        {
            Serial.printf("Unknown alarm bits = 0x%04X\n", unknownBits);
        }

        break;
    }

    case 0x03:
        Serial.println("Device info frame");
        got_device_info = true;
        jk_state = 1;
        break;
    
    default:
        Serial.printf("Unknown frame %02X\n", data[4]);
        break;
    }
}

void jkProtocolLoop()
{
    static uint32_t n = 0;

    if (!bleConnected())
        return;
    if (got_device_info && got_settings)
    {
        jk_state = 2;        // explicit "READY" state
        return;
    }

    if (jk_state == 2)
        return;   // fully done, stop all polling

    if (millis() - lastRequest < 500)
        return;
    // STEP 1: handshake not complete → send requests
    if (!got_settings)
    {
        if (frame_buffer_.size() >= 300)
        {
            // allow decode to finish before sending new request
            return;
        }
        lastRequest = millis();

        uint8_t frame[20];

        frame[0] = 0xAA;
        frame[1] = 0x55;
        frame[2] = 0x90;
        frame[3] = 0xEB;

        if (jk_state == 0)
            frame[4] = 0x97;
        else
            frame[4] = 0x96;
        frame[5] = 0x00;

        for (int i = 6; i < 19; i++)
            frame[i] = 0x00;

        frame[19] = crc(frame, 19);

        Serial.printf("TX request = %02X  state=%u\n", frame[4], jk_state);
        bleWrite(frame, 20);
        return;
    }

    // STEP 2: handshake complete → DO NOTHING
    // IMPORTANT: no more TX after this point
}

uint8_t crc(const uint8_t data[], const uint16_t len) {
  uint8_t crc = 0;
  for (uint16_t i = 0; i < len; i++) {
    crc = crc + data[i];
  }
  return crc;
}

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef void (*BleNotifyCallback)(const uint8_t *data, size_t length);

bool bleBegin();
void bleLoop();
bool bleConnected();

bool bleWrite(const uint8_t *data, size_t length);
void bleSetNotifyCallback(BleNotifyCallback callback);
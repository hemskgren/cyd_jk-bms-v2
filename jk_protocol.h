#pragma once

#include <cstdint>
#include <cstddef>

void jkProtocolBegin();
void jkProtocolLoop();
void jkNotify(const uint8_t *data, size_t length);
void decodePacket(const uint8_t *data, size_t length);

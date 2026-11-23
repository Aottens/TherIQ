// SPDX-License-Identifier: GPL-3.0-or-later
//
// TQ-03: example host test. The EMS CRC against telegrams captured on real
// buses, with the CRC byte as received.
//
// Test vectors: EMS-ESP v3.9.1, src/test/test.cpp (uart_telegram_withCRC),
// https://github.com/emsesp/EMS-ESP32/blob/v3.9.1/src/test/test.cpp

// assert() is the whole test framework (ADR-016). NDEBUG would turn every
// assert into nothing, so make sure it is off whatever the build type.
#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <cstdio>

#include "ems_frame.h"

int main()
{
    // The last byte of each telegram is the CRC over all bytes before it.
    const uint8_t ems1[] = {0x90, 0x0B, 0x06, 0x00, 0x14, 0x06, 0x17, 0x08,
                            0x03, 0x22, 0x00, 0x01, 0x10, 0xFF, 0x00, 0x18};
    assert(ems::crc(ems1, sizeof(ems1) - 1) == 0x18);

    const uint8_t ems_plus[] = {0x90, 0x00, 0xFF, 0x19, 0x01, 0xA5, 0x01, 0x04, 0x00, 0x00,
                                0x00, 0x00, 0xFF, 0x64, 0x2A, 0x00, 0x3C, 0x01, 0xFF, 0x92};
    assert(ems::crc(ems_plus, sizeof(ems_plus) - 1) == 0x92);

    // A single flipped bit must change the CRC.
    uint8_t corrupted[sizeof(ems1)];
    for (size_t i = 0; i < sizeof(ems1); ++i) {
        corrupted[i] = ems1[i];
    }
    corrupted[5] ^= 0x01;
    assert(ems::crc(corrupted, sizeof(corrupted) - 1) != 0x18);

    std::puts("TQ-03 ems crc: ok");
    return 0;
}

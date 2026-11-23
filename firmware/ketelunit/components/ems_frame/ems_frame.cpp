// SPDX-License-Identifier: GPL-3.0-or-later
//
// EMS telegram layer.
//
// Derived from EMS-ESP (GPL-3.0-or-later), https://github.com/emsesp/EMS-ESP32,
// tag v3.9.1 (commit f20d939), file src/core/telegram.cpp:
// EMSbus::calculate_crc(). Copyright 2020-2026 emsesp.org.

#include "ems_frame.h"

namespace ems {

// EMS-ESP uses a 256-byte lookup table. Every entry of that table is
// (i << 1) ^ (i & 0x80 ? 0x19 : 0), i.e. one shift with reduction by
// x^8 + x^4 + x^3 + 1. We compute that step instead of storing the table: same
// result for all 256 inputs (checked when this was written), and a telegram is
// at most 32 bytes, so speed does not matter.
uint8_t crc(const uint8_t *bytes, size_t length)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc = static_cast<uint8_t>((crc << 1) ^ ((crc & 0x80) ? 0x19 : 0x00));
        crc ^= bytes[i];
    }
    return crc;
}

}  // namespace ems

// SPDX-License-Identifier: GPL-3.0-or-later
//
// EMS telegram layer: CRC, encode and decode. Pure logic, no ESP-IDF.
//
// Protocol knowledge from EMS-ESP (GPL-3.0), see ems_frame.cpp and NOTICE.

#pragma once

#include <cstddef>
#include <cstdint>

namespace ems {

// CRC over `length` bytes, as appended to every EMS telegram.
uint8_t crc(const uint8_t *bytes, size_t length);

}  // namespace ems

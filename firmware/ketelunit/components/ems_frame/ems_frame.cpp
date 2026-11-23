// SPDX-License-Identifier: GPL-3.0-or-later
//
// EMS telegram layer.
//
// Derived from EMS-ESP (GPL-3.0-or-later), https://github.com/emsesp/EMS-ESP32,
// tag v3.9.1 (commit f20d939), file src/core/telegram.cpp:
// EMSbus::calculate_crc(), RxService::add() (decode) and
// TxService::send_telegram() (encode). Copyright 2020-2026 emsesp.org.
// Rewritten without EMS-ESP's queues and logging; layout and type numbering
// are kept, so EMS-ESP's type ids can be used as they are.

#include "ems_frame.h"

#include <cstring>

namespace ems {

namespace {

constexpr uint8_t kReadFlag = 0x80;     // MSB of the dest byte
constexpr uint8_t kExtendedType = 0xFF;  // type byte that announces an EMS+ header
constexpr uint16_t kExtendedBase = 0x100;

}  // namespace

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

bool decode(const uint8_t *bytes, size_t length, Frame &frame)
{
    // src, dest, type, offset and CRC is the shortest telegram. A single byte
    // on the bus is a poll or a poll reply, not a telegram.
    if (length < 5 || length > kMaxTelegramLength) {
        return false;
    }
    const size_t body = length - 1;  // everything before the CRC
    if (crc(bytes, body) != bytes[body]) {
        return false;
    }

    frame.src = bytes[0];
    frame.dest = static_cast<uint8_t>(bytes[1] & ~kReadFlag);
    frame.read = (bytes[1] & kReadFlag) != 0;
    frame.offset = bytes[3];

    size_t data_start = 4;
    if (bytes[2] != kExtendedType || length < 6) {
        // EMS 1.0. Like EMS-ESP, a 5-byte telegram with type 0xFF also lands
        // here: that is how an EMS 1.0 device answers an EMS+ request (empty).
        frame.type = bytes[2];
    } else {
        // EMS+. A read request puts the requested length before the two type
        // bytes and has nothing after them; a write has its data after them.
        const size_t type_at = frame.read ? 5 : 4;
        if (frame.read ? length != 8 : length < 7) {
            return false;
        }
        if (bytes[type_at] == 0xFF) {
            return false;  // type + 0x100 would not fit in 16 bits
        }
        frame.type = static_cast<uint16_t>(((bytes[type_at] << 8) | bytes[type_at + 1]) + kExtendedBase);
        if (frame.read) {
            frame.data[0] = bytes[4];
            frame.data_length = 1;
            return true;
        }
        data_start = 6;
    }

    // At most 32 - 4 - 1 = 27 bytes, so it always fits in frame.data.
    frame.data_length = static_cast<uint8_t>(body - data_start);
    std::memcpy(frame.data, bytes + data_start, frame.data_length);
    return true;
}

size_t encode(const Frame &frame, uint8_t (&bytes)[kMaxTelegramLength])
{
    if ((frame.dest & kReadFlag) != 0 || frame.data_length > kMaxDataLength) {
        return 0;
    }

    bytes[0] = frame.src;
    bytes[1] = static_cast<uint8_t>(frame.dest | (frame.read ? kReadFlag : 0x00));
    bytes[3] = frame.offset;

    size_t n = 4;  // next byte to write
    if (frame.type < kExtendedBase) {
        if (frame.type == kExtendedType && frame.data_length > 0) {
            return 0;
        }
        bytes[2] = static_cast<uint8_t>(frame.type);
    } else {
        const uint16_t wire_type = frame.type - kExtendedBase;
        bytes[2] = kExtendedType;
        if (frame.read) {
            if (frame.data_length != 1) {
                return 0;
            }
            bytes[4] = frame.data[0];  // number of bytes asked
            bytes[5] = static_cast<uint8_t>(wire_type >> 8);
            bytes[6] = static_cast<uint8_t>(wire_type & 0xFF);
            bytes[7] = crc(bytes, 7);
            return 8;
        }
        bytes[4] = static_cast<uint8_t>(wire_type >> 8);
        bytes[5] = static_cast<uint8_t>(wire_type & 0xFF);
        n = 6;
    }

    if (n + frame.data_length + 1 > kMaxTelegramLength) {
        return 0;
    }
    std::memcpy(bytes + n, frame.data, frame.data_length);
    n += frame.data_length;
    bytes[n] = crc(bytes, n);
    return n + 1;
}

}  // namespace ems

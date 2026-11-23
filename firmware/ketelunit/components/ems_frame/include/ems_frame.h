// SPDX-License-Identifier: GPL-3.0-or-later
//
// EMS telegram layer: CRC, encode and decode. Pure logic, no ESP-IDF.
//
// A telegram on the wire (EMS-ESP docs, "The EMS bus explained"):
//
//   EMS 1.0 write/broadcast  src dest      type offset data...              CRC
//   EMS 1.0 read             src dest|80   type offset length               CRC
//   EMS+    write/broadcast  src dest      FF   offset typeH typeL data...  CRC
//   EMS+    read             src dest|80   FF   offset length typeH typeL   CRC
//
// This layer only checks and builds that layout. What the bytes mean (which
// type holds the flow temperature, polling, the break after each telegram,
// Junkers/HT3 address masks) belongs to the driver above it (TQ-14).
//
// Protocol knowledge from EMS-ESP (GPL-3.0), see ems_frame.cpp and NOTICE.

#pragma once

#include <cstddef>
#include <cstdint>

namespace ems {

// Longest telegram on the bus, CRC included (EMS-ESP: EMS_MAX_TELEGRAM_LENGTH).
constexpr size_t kMaxTelegramLength = 32;
// Most data an EMS 1.0 telegram can carry: 32 minus 4 header bytes and CRC.
constexpr size_t kMaxDataLength = 27;

// One telegram, decoded.
struct Frame {
    // Sender exactly as on the wire. Junkers/HT3 devices set the MSB
    // (0x90 = device 0x10); the driver decides what that means.
    uint8_t src = 0;
    // Receiver, 0x00..0x7F (0x00 = broadcast). The MSB on the wire is `read`.
    uint8_t dest = 0;
    // true: a read request, `data` holds one byte, the number of bytes asked.
    bool read = false;
    // Telegram type, numbered as in EMS-ESP: 0x00..0xFF is EMS 1.0 (one type
    // byte); 0x100 and up is EMS+, sent as two bytes (type - 0x100). EMS+ type
    // bytes 01 A5 are thus type 0x02A5. 0xF7 and 0xF9 are EMS 1.0 types.
    uint16_t type = 0;
    uint8_t offset = 0;
    uint8_t data[kMaxDataLength] = {};
    uint8_t data_length = 0;
};

// CRC over `length` bytes, as appended to every EMS telegram.
uint8_t crc(const uint8_t *bytes, size_t length);

// Decodes a whole telegram of `length` bytes, CRC included. Returns false, and
// leaves `frame` unspecified, if the CRC is wrong or the layout does not fit
// (too short, too long, EMS+ header cut off).
bool decode(const uint8_t *bytes, size_t length, Frame &frame);

// Builds the telegram for `frame` in `bytes`, CRC included, and returns its
// length. Returns 0 if the frame cannot be sent: data too long for the
// type's header, dest above 0x7F, an EMS+ read without exactly one data byte,
// or EMS 1.0 type 0xFF with data (it would read back as an EMS+ header).
size_t encode(const Frame &frame, uint8_t (&bytes)[kMaxTelegramLength]);

}  // namespace ems

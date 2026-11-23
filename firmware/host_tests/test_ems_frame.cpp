// SPDX-License-Identifier: GPL-3.0-or-later
//
// TQ-12: EMS telegram encode and decode.
//
// Test vectors are telegrams captured on real buses, CRC byte included, from
// EMS-ESP v3.9.1, src/test/test.cpp (uart_telegram_withCRC, lines 1921-1945),
// https://github.com/emsesp/EMS-ESP32/blob/v3.9.1/src/test/test.cpp
// The source bytes 0x90 and 0xC8 are devices 0x10 and 0x48 with the MSB set,
// as Junkers/HT3 devices send it; the frame layer keeps that byte as is.

// assert() is the whole test framework (ADR-016); keep it on in every build.
#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "ems_frame.h"

namespace {

struct Vector {
    const char *name;
    uint8_t bytes[ems::kMaxTelegramLength];
    size_t length;  // including the CRC
};

// RCTime (type 0x06) from a thermostat to the service key 0x0B: EMS 1.0.
const Vector kEms1Time = {
    "EMS 1.0 RCTime",
    {0x90, 0x0B, 0x06, 0x00, 0x14, 0x06, 0x17, 0x08, 0x03, 0x22, 0x00, 0x01, 0x10, 0xFF, 0x00, 0x18},
    16};

// Thermostat broadcast of EMS+ type 01 A5 (RC300 monitor) at offset 0x19.
const Vector kEmsPlusBroadcast = {
    "EMS+ broadcast",
    {0x90, 0x00, 0xFF, 0x19, 0x01, 0xA5, 0x01, 0x04, 0x00, 0x00,
     0x00, 0x00, 0xFF, 0x64, 0x2A, 0x00, 0x3C, 0x01, 0xFF, 0x92},
    20};

// 0x48 asks 0x10 for 2 bytes of EMS+ type 01 A6: dest MSB set = read.
const Vector kEmsPlusRead = {"EMS+ read", {0xC8, 0x90, 0xFF, 0x00, 0x02, 0x01, 0xA6, 0xD0}, 8};

// Type 0xF7 has the EMS 1.0 layout: only 0xFF in byte 2 marks EMS+.
const Vector kEms1F7Read = {"EMS 1.0 type F7", {0xC8, 0x90, 0xF7, 0x02, 0x01, 0xFF, 0x01, 0xA6, 0xBA}, 9};

bool same(const ems::Frame &a, const ems::Frame &b)
{
    return a.src == b.src && a.dest == b.dest && a.read == b.read && a.type == b.type && a.offset == b.offset &&
           a.data_length == b.data_length && std::memcmp(a.data, b.data, a.data_length) == 0;
}

ems::Frame decoded(const Vector &v)
{
    ems::Frame frame;
    const bool ok = ems::decode(v.bytes, v.length, frame);
    if (!ok) {
        std::printf("decode failed: %s\n", v.name);
    }
    assert(ok);
    return frame;
}

void check_data(const ems::Frame &frame, const uint8_t *expected, uint8_t length)
{
    assert(frame.data_length == length);
    assert(std::memcmp(frame.data, expected, length) == 0);
}

void test_decode_fields()
{
    const ems::Frame time = decoded(kEms1Time);
    assert(time.src == 0x90);
    assert(time.dest == 0x0B);
    assert(!time.read);
    assert(time.type == 0x06);
    assert(time.offset == 0x00);
    const uint8_t time_data[] = {0x14, 0x06, 0x17, 0x08, 0x03, 0x22, 0x00, 0x01, 0x10, 0xFF, 0x00};
    check_data(time, time_data, sizeof(time_data));

    // EMS+ types are numbered as in EMS-ESP: the two type bytes + 0x100.
    const ems::Frame broadcast = decoded(kEmsPlusBroadcast);
    assert(broadcast.src == 0x90);
    assert(broadcast.dest == 0x00);
    assert(!broadcast.read);
    assert(broadcast.type == 0x02A5);
    assert(broadcast.offset == 0x19);
    const uint8_t broadcast_data[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x64, 0x2A, 0x00, 0x3C, 0x01, 0xFF};
    check_data(broadcast, broadcast_data, sizeof(broadcast_data));

    // A read request carries one data byte: how many bytes are asked for.
    const ems::Frame read = decoded(kEmsPlusRead);
    assert(read.src == 0xC8);
    assert(read.dest == 0x10);
    assert(read.read);
    assert(read.type == 0x02A6);
    assert(read.offset == 0x00);
    const uint8_t read_data[] = {0x02};
    check_data(read, read_data, sizeof(read_data));

    const ems::Frame f7 = decoded(kEms1F7Read);
    assert(f7.src == 0xC8);
    assert(f7.dest == 0x10);
    assert(f7.read);
    assert(f7.type == 0xF7);
    assert(f7.offset == 0x02);
    const uint8_t f7_data[] = {0x01, 0xFF, 0x01, 0xA6};
    check_data(f7, f7_data, sizeof(f7_data));
}

// decode, then encode, gives the captured bytes back, CRC included.
void test_round_trip_captured()
{
    for (const Vector *v : {&kEms1Time, &kEmsPlusBroadcast, &kEmsPlusRead, &kEms1F7Read}) {
        uint8_t out[ems::kMaxTelegramLength];
        const size_t length = ems::encode(decoded(*v), out);
        if (length != v->length || std::memcmp(out, v->bytes, length) != 0) {
            std::printf("round trip failed: %s\n", v->name);
        }
        assert(length == v->length);
        assert(std::memcmp(out, v->bytes, length) == 0);
    }
}

// encode, then decode, gives the same frame: a read request as TherIQ will
// send it (0x0B asks the boiler 0x08 for 25 bytes of UBAMonitorFast, 0x18),
// and an EMS+ write.
void test_encode_then_decode()
{
    ems::Frame request;
    request.src = 0x0B;
    request.dest = 0x08;
    request.read = true;
    request.type = 0x18;
    request.data[0] = 25;
    request.data_length = 1;

    uint8_t out[ems::kMaxTelegramLength];
    const size_t length = ems::encode(request, out);
    const uint8_t expected[] = {0x0B, 0x88, 0x18, 0x00, 0x19};
    assert(length == sizeof(expected) + 1);
    assert(std::memcmp(out, expected, sizeof(expected)) == 0);
    assert(out[length - 1] == ems::crc(out, length - 1));

    ems::Frame back;
    assert(ems::decode(out, length, back));
    assert(same(back, request));

    ems::Frame write;
    write.src = 0x0B;
    write.dest = 0x10;
    write.type = 0x02B9;
    write.offset = 0x13;
    write.data[0] = 0x01;
    write.data_length = 1;
    const size_t write_length = ems::encode(write, out);
    const uint8_t write_expected[] = {0x0B, 0x10, 0xFF, 0x13, 0x01, 0xB9, 0x01};
    assert(write_length == sizeof(write_expected) + 1);
    assert(std::memcmp(out, write_expected, sizeof(write_expected)) == 0);
    assert(ems::decode(out, write_length, back));
    assert(same(back, write));
}

void test_decode_rejects()
{
    ems::Frame frame;
    uint8_t bytes[ems::kMaxTelegramLength + 1] = {};

    // Wrong CRC byte, and a flipped bit in the data.
    std::memcpy(bytes, kEms1Time.bytes, kEms1Time.length);
    bytes[kEms1Time.length - 1] ^= 0xFF;
    assert(!ems::decode(bytes, kEms1Time.length, frame));
    std::memcpy(bytes, kEms1Time.bytes, kEms1Time.length);
    bytes[6] ^= 0x04;
    assert(!ems::decode(bytes, kEms1Time.length, frame));

    // Too short: below src, dest, type, offset, CRC. A single byte is a poll,
    // not a telegram.
    const uint8_t header[] = {0x08, 0x0B, 0x18, 0x00};
    assert(!ems::decode(header, 0, frame));
    for (size_t length = 1; length < 5; ++length) {
        std::memcpy(bytes, header, length - 1);
        bytes[length - 1] = ems::crc(bytes, length - 1);  // valid CRC, still too short
        assert(!ems::decode(bytes, length, frame));
    }

    // EMS+ header cut off: 0xFF in byte 2 but no room for both type bytes.
    const uint8_t cut[] = {0x90, 0x00, 0xFF, 0x00, 0x01, 0x00};
    std::memcpy(bytes, cut, sizeof(cut));
    bytes[sizeof(cut) - 1] = ems::crc(bytes, sizeof(cut) - 1);
    assert(!ems::decode(bytes, sizeof(cut), frame));

    // EMS+ read request must be exactly src dest FF offset length typeH typeL CRC.
    const uint8_t long_read[] = {0xC8, 0x90, 0xFF, 0x00, 0x02, 0x01, 0xA6, 0x00, 0x00};
    std::memcpy(bytes, long_read, sizeof(long_read));
    bytes[sizeof(long_read) - 1] = ems::crc(bytes, sizeof(long_read) - 1);
    assert(!ems::decode(bytes, sizeof(long_read), frame));

    // Longer than the 32 bytes an EMS telegram can be, with a valid CRC.
    for (size_t i = 0; i < ems::kMaxTelegramLength; ++i) {
        bytes[i] = static_cast<uint8_t>(0x10 + i);
    }
    bytes[ems::kMaxTelegramLength] = ems::crc(bytes, ems::kMaxTelegramLength);
    assert(!ems::decode(bytes, ems::kMaxTelegramLength + 1, frame));
}

void test_encode_rejects()
{
    uint8_t out[ems::kMaxTelegramLength];

    ems::Frame too_long;
    too_long.src = 0x0B;
    too_long.dest = 0x08;
    too_long.type = 0x02A5;  // EMS+: 6 header bytes + CRC leave room for 25
    too_long.data_length = 26;
    assert(ems::encode(too_long, out) == 0);
    too_long.data_length = 25;
    assert(ems::encode(too_long, out) == ems::kMaxTelegramLength);

    ems::Frame bad_dest;
    bad_dest.dest = 0x88;  // the MSB is the read flag, not part of the address
    bad_dest.type = 0x18;
    assert(ems::encode(bad_dest, out) == 0);

    ems::Frame read_without_length;
    read_without_length.dest = 0x08;
    read_without_length.read = true;
    read_without_length.type = 0x02A5;
    assert(ems::encode(read_without_length, out) == 0);

    // Type 0xFF with data would be read back as an EMS+ header.
    ems::Frame ff;
    ff.dest = 0x08;
    ff.type = 0xFF;
    ff.data_length = 2;
    assert(ems::encode(ff, out) == 0);
}

}  // namespace

int main()
{
    test_decode_fields();
    test_round_trip_captured();
    test_encode_then_decode();
    test_decode_rejects();
    test_encode_rejects();
    std::puts("TQ-12 ems frame: ok");
    return 0;
}

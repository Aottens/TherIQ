// SPDX-License-Identifier: GPL-3.0-or-later
//
// TQ-22: on/off room controller with hysteresis (ADR-021).
//
// The controller is a pure function of its inputs plus one bit of memory, the
// heat demand it gave last time, like an SR flip-flop behind two comparators
// in a PLC. These tests drive it with a room temperature, a setpoint, a mode
// and a millisecond clock, and check the heat demand it answers.

// assert() is the whole test framework (ADR-016); keep it on in every build.
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "control.h"

namespace {

using boiler::Availability;
using control::Mode;
using control::OnOffController;
using control::OnOffParams;

constexpr float kSetpoint = 20.0f;

// A room temperature read at `at_ms`.
boiler::Reading<float> room(float celsius, uint32_t at_ms)
{
    return {Availability::kValid, celsius, at_ms};
}

// Default band (ADR-021): on at setpoint - 0.2, off at setpoint + 0.2.
void test_default_params()
{
    const OnOffParams p;
    assert(p.below_c == 0.2f);
    assert(p.above_c == 0.2f);
    assert(p.max_room_age_ms == 5u * 60u * 1000u);
}

// Starts without heat demand.
void test_starts_off()
{
    const OnOffController c;
    assert(!c.demand());
}

// Below the band: heat on. Above the band: heat off.
void test_switches_outside_band()
{
    OnOffController c;
    assert(c.update(room(19.5f, 0), kSetpoint, Mode::kHeat, 0));
    assert(c.demand());
    assert(!c.update(room(20.5f, 1000), kSetpoint, Mode::kHeat, 1000));
    assert(!c.demand());
}

// Inside the band the last decision holds, in both directions: no chatter.
void test_holds_inside_band()
{
    OnOffController c;

    // Coming from cold: stays on all the way up through the band.
    assert(c.update(room(19.5f, 0), kSetpoint, Mode::kHeat, 0));
    assert(c.update(room(19.9f, 1000), kSetpoint, Mode::kHeat, 1000));
    assert(c.update(room(20.0f, 2000), kSetpoint, Mode::kHeat, 2000));
    assert(c.update(room(20.1f, 3000), kSetpoint, Mode::kHeat, 3000));
    assert(!c.update(room(20.3f, 4000), kSetpoint, Mode::kHeat, 4000));

    // Cooling down: stays off all the way down through the band.
    assert(!c.update(room(20.1f, 5000), kSetpoint, Mode::kHeat, 5000));
    assert(!c.update(room(20.0f, 6000), kSetpoint, Mode::kHeat, 6000));
    assert(!c.update(room(19.9f, 7000), kSetpoint, Mode::kHeat, 7000));
    assert(c.update(room(19.7f, 8000), kSetpoint, Mode::kHeat, 8000));

    // Noise around the setpoint does not toggle it.
    for (uint32_t t = 9000; t < 20000; t += 1000) {
        const float noisy = (t / 1000) % 2 ? 19.95f : 20.05f;
        assert(c.update(room(noisy, t), kSetpoint, Mode::kHeat, t));
    }
}

// A fresh controller inside the band stays off: it only switches on below it.
void test_starts_off_inside_band()
{
    OnOffController c;
    assert(!c.update(room(19.9f, 0), kSetpoint, Mode::kHeat, 0));
}

// Off is always off, however cold, and drops a running demand at once.
void test_off_mode()
{
    OnOffController c;
    assert(!c.update(room(10.0f, 0), kSetpoint, Mode::kOff, 0));
    assert(c.update(room(19.0f, 1000), kSetpoint, Mode::kHeat, 1000));
    assert(!c.update(room(19.0f, 2000), kSetpoint, Mode::kOff, 2000));
    assert(!c.demand());
}

// A new setpoint moves the band with it.
void test_setpoint_change()
{
    OnOffController c;
    assert(!c.update(room(20.0f, 0), kSetpoint, Mode::kHeat, 0));
    assert(c.update(room(20.0f, 1000), 21.0f, Mode::kHeat, 1000));
    assert(!c.update(room(20.0f, 2000), 16.0f, Mode::kHeat, 2000));
}

// No usable room temperature: heat off (ADR-021), and back to normal once a
// fresh value arrives.
void test_lost_room_temperature()
{
    OnOffController c;
    assert(c.update(room(19.0f, 0), kSetpoint, Mode::kHeat, 0));

    // Never read, or lost.
    boiler::Reading<float> unknown;
    assert(!c.update(unknown, kSetpoint, Mode::kHeat, 1000));

    assert(c.update(room(19.0f, 2000), kSetpoint, Mode::kHeat, 2000));

    // Not supported (no sensor at all).
    boiler::Reading<float> unsupported;
    unsupported.availability = Availability::kUnsupported;
    assert(!c.update(unsupported, kSetpoint, Mode::kHeat, 3000));

    // Valid but not a number.
    assert(c.update(room(19.0f, 4000), kSetpoint, Mode::kHeat, 4000));
    assert(!c.update(room(NAN, 5000), kSetpoint, Mode::kHeat, 5000));
}

// A value older than max_room_age_ms counts as lost; exactly that old is
// still fresh (Reading::fresh uses <=).
void test_stale_room_temperature()
{
    OnOffController c;
    const uint32_t max_age = OnOffParams{}.max_room_age_ms;
    const boiler::Reading<float> cold = room(19.0f, 1000);

    assert(c.update(cold, kSetpoint, Mode::kHeat, 1000));
    assert(c.update(cold, kSetpoint, Mode::kHeat, 1000 + max_age));
    assert(!c.update(cold, kSetpoint, Mode::kHeat, 1000 + max_age + 1));

    // A fresh value brings it back.
    assert(c.update(room(19.0f, 1000 + max_age + 2), kSetpoint, Mode::kHeat,
                    1000 + max_age + 2));

    // Age across a wrap of the 32-bit millisecond counter.
    OnOffController w;
    const boiler::Reading<float> before_wrap = room(19.0f, 0xFFFFFFFFu - 999u);
    assert(w.update(before_wrap, kSetpoint, Mode::kHeat, 2000));
}

// The band is a parameter: asymmetric, here off as soon as the setpoint is
// reached, and a shorter max age.
void test_custom_params()
{
    OnOffParams p;
    p.below_c = 0.5f;
    p.above_c = 0.0f;
    p.max_room_age_ms = 1000;
    OnOffController c(p);

    assert(!c.update(room(19.6f, 0), kSetpoint, Mode::kHeat, 0));
    assert(c.update(room(19.4f, 0), kSetpoint, Mode::kHeat, 0));
    assert(c.update(room(19.9f, 0), kSetpoint, Mode::kHeat, 0));
    assert(!c.update(room(20.0f, 0), kSetpoint, Mode::kHeat, 0));

    assert(c.update(room(19.0f, 0), kSetpoint, Mode::kHeat, 1000));
    assert(!c.update(room(19.0f, 0), kSetpoint, Mode::kHeat, 1001));
}

}  // namespace

int main()
{
    test_default_params();
    test_starts_off();
    test_switches_outside_band();
    test_holds_inside_band();
    test_starts_off_inside_band();
    test_off_mode();
    test_setpoint_change();
    test_lost_room_temperature();
    test_stale_room_temperature();
    test_custom_params();
    std::printf("TQ-22 on/off controller: all tests passed\n");
    return 0;
}

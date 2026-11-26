// SPDX-License-Identifier: GPL-3.0-or-later
//
// Room control: decides whether the house needs heat (ADR-021). Pure logic,
// no ESP-IDF, so host tests can use it.
//
// On/off with hysteresis (TQ-22). In PLC terms: two comparators on the room
// temperature feed an SR flip-flop.
//
//   room <= setpoint - below_c   -> set   (heat on)
//   room >= setpoint + above_c   -> reset (heat off)
//   in between                   -> hold  (the flip-flop keeps its state)
//
// The band in between is what stops the boiler from chattering on and off
// when the room temperature hovers around the setpoint.

#pragma once

#include <cstdint>

#include "boiler_api.h"

namespace control {

enum class Mode : uint8_t {
    kOff,   // no heating at all, however cold
    kHeat,  // heat to the setpoint
};

// Tuning of the on/off controller. Defaults from ADR-021.
struct OnOffParams {
    float below_c = 0.2f;  // heat on at setpoint - below_c
    float above_c = 0.2f;  // heat off at setpoint + above_c
    // A room temperature older than this counts as lost. The room unit sends
    // far more often; five minutes leaves room for a few missed messages.
    uint32_t max_room_age_ms = 5 * 60 * 1000;
};

class OnOffController {
public:
    explicit OnOffController(OnOffParams params = {}) : params_(params) {}

    // One scan of the controller: takes the inputs, returns the new heat
    // demand and remembers it for the next call. `now_ms` is the same
    // monotonic clock as Reading::updated_ms.
    bool update(const boiler::Reading<float> &room_c, float setpoint_c, Mode mode,
                uint32_t now_ms);

    // The heat demand of the last update(); false before the first one.
    bool demand() const { return demand_; }

private:
    OnOffParams params_;
    bool demand_ = false;  // the flip-flop's state
};

}  // namespace control

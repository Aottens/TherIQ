// SPDX-License-Identifier: GPL-3.0-or-later
//
// Room control, see control.h.

#include "control.h"

#include <cmath>

namespace control {

bool OnOffController::update(const boiler::Reading<float> &room_c, float setpoint_c,
                             Mode mode, uint32_t now_ms)
{
    if (mode == Mode::kOff) {
        demand_ = false;
        return demand_;
    }

    // No usable room temperature: heat off (ADR-021). Off cannot overheat the
    // house; the boiler still protects itself against freezing. The house
    // cools down slowly, so the caller must report the lost sensor (TQ-24).
    // Falling back to the heating curve instead comes in phase 1.
    if (!room_c.fresh(now_ms, params_.max_room_age_ms) || std::isnan(room_c.value)) {
        demand_ = false;
        return demand_;
    }

    if (room_c.value <= setpoint_c - params_.below_c) {
        demand_ = true;  // set
    } else if (room_c.value >= setpoint_c + params_.above_c) {
        demand_ = false;  // reset
    }
    // Inside the band neither branch runs: demand_ holds its last value.
    return demand_;
}

}  // namespace control

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
//
// Week schedule (TQ-23), below the controller. Like a weekly time-switch
// clock: per weekday a list of switch moments, each with a setpoint. A manual
// setpoint holds until the next switch moment, then the schedule takes over.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

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

// ---------------------------------------------------------------------------
// Week schedule (TQ-23)
// ---------------------------------------------------------------------------

// 0 = Monday. Note: struct tm's tm_wday has 0 = Sunday; the time layer that
// fills LocalTime converts. Unscoped so the constants index a WeekSchedule
// directly: week[kSaturday].
enum Weekday : uint8_t { kMonday, kTuesday, kWednesday, kThursday, kFriday, kSaturday, kSunday };

// Local wall-clock time, daylight saving time included. This component has no
// clock: SNTP and the time zone live in the layer that provides the time.
struct LocalTime {
    uint8_t weekday;  // Weekday, 0 = Monday
    uint16_t minute;  // minute of the day, 0..1439
};

// From `minute` of the day on, the setpoint is `setpoint_c`.
struct SwitchPoint {
    uint16_t minute;  // minute of the day, 0..1439
    float setpoint_c;
};

// Precondition: sorted by minute, earliest first. An empty day keeps the last
// setpoint of the day before.
using DaySchedule = std::vector<SwitchPoint>;
using WeekSchedule = std::array<DaySchedule, 7>;  // indexed by Weekday

// How Aernout's house heats now (ADR-021): 18.5 °C from 16:30 on weekdays and
// from 10:00 on Saturday and Sunday, 15.5 °C from 21:30 every day. Before the
// first switch moment the 15.5 °C of the night before holds. The default in code until the schedule is stored in NVS and can be
// edited (IDEA-02).
WeekSchedule default_week();

// What the schedule says right now.
struct ScheduledSetpoint {
    float setpoint_c;
    LocalTime until;  // the next switch moment: the display's "tot 22:30"
    bool manual;      // true while a manual setpoint holds
};

class Schedule {
public:
    explicit Schedule(WeekSchedule week) : week_(std::move(week)) {}

    // The setpoint at `now`. std::nullopt when the week has no switch moment
    // at all; the caller then keeps its own setpoint. Not const: it clears a
    // manual setpoint once its switch moment has passed.
    //
    // A manual setpoint counts down like a PLC timer, by the time between
    // calls. A step back of up to two hours (DST ending, an SNTP correction)
    // counts back up, so it still ends at the switch moment on the wall
    // clock. A jump forward counts as time passed: feed it only once the
    // clock is set (TQ-24). The caller must call this at least once a week
    // minus two hours (the control loop does every few seconds), otherwise a
    // stale manual setpoint could look valid again.
    std::optional<ScheduledSetpoint> update(LocalTime now);

    // Overrides the schedule from `now` until the next switch moment. A newer
    // manual setpoint replaces the older one.
    void set_manual(float setpoint_c, LocalTime now);

private:
    WeekSchedule week_;
    // The manual setpoint; inactive when remaining_min_ == 0.
    float manual_c_ = 0.0f;
    LocalTime manual_until_{};
    uint32_t remaining_min_ = 0;  // minutes until manual_until_
    LocalTime last_{};            // time of the last set_manual/update
};

}  // namespace control

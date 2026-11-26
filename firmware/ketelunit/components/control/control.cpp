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

// ---------------------------------------------------------------------------
// Week schedule (TQ-23)
// ---------------------------------------------------------------------------

namespace {

// All time arithmetic is in minute of the week: Monday 00:00 = 0, Sunday
// 23:59 = 10079. One number per moment makes "over midnight" and "Sunday to
// Monday" plain modulo arithmetic instead of special cases.
constexpr int kMinutesPerDay = 24 * 60;
constexpr int kMinutesPerWeek = 7 * kMinutesPerDay;

// How far the clock may step back and still count as "no time passed".
constexpr int kMaxClockStepBackMin = 2 * 60;

int minute_of_week(LocalTime t)
{
    return t.weekday * kMinutesPerDay + t.minute;
}

LocalTime local_time(int minute_of_week)
{
    return {static_cast<uint8_t>(minute_of_week / kMinutesPerDay),
            static_cast<uint16_t>(minute_of_week % kMinutesPerDay)};
}

// Minutes from `from` forward to `to`, wrapping the week: 0..10079.
int minutes_forward(int from, int to)
{
    return ((to - from) % kMinutesPerWeek + kMinutesPerWeek) % kMinutesPerWeek;
}

// The switch point in force at `now` and the next switch moment after it.
struct Lookup {
    float setpoint_c;
    int next;      // minute of week of the next switch moment
    int distance;  // minutes from now to it, 1..10080
};

// One pass over every switch point of the week. The one in force is the one
// fewest minutes back (0 = it switches this very minute); the next one is the
// one fewest minutes ahead, never 0: a point at `now` is a whole week ahead,
// which is also the answer for a week with a single point.
std::optional<Lookup> lookup(const WeekSchedule &week, int now)
{
    std::optional<Lookup> found;
    int best_back = kMinutesPerWeek;
    for (int day = 0; day < 7; ++day) {
        for (const SwitchPoint &point : week[day]) {
            const int at = day * kMinutesPerDay + point.minute;
            const int back = minutes_forward(at, now);
            int ahead = minutes_forward(now, at);
            if (ahead == 0) {
                ahead = kMinutesPerWeek;
            }
            if (!found) {
                found = Lookup{point.setpoint_c, at, ahead};
                best_back = back;
                continue;
            }
            if (back < best_back) {
                best_back = back;
                found->setpoint_c = point.setpoint_c;
            }
            if (ahead < found->distance) {
                found->next = at;
                found->distance = ahead;
            }
        }
    }
    return found;
}

}  // namespace

WeekSchedule default_week()
{
    WeekSchedule week;
    for (int d = kMonday; d <= kSunday; ++d) {
        const uint16_t warm = d >= kSaturday ? 10 * 60 : 16 * 60 + 30;
        week[d] = {{warm, 18.5f}, {21 * 60 + 30, 15.5f}};
    }
    return week;
}

void Schedule::set_manual(float setpoint_c, LocalTime now)
{
    const std::optional<Lookup> found = lookup(week_, minute_of_week(now));
    if (!found) {
        remaining_min_ = 0;  // no switch moment to end at: nothing to hold
        return;
    }
    manual_c_ = setpoint_c;
    manual_until_ = local_time(found->next);
    remaining_min_ = static_cast<uint32_t>(found->distance);
    last_ = now;
}

std::optional<ScheduledSetpoint> Schedule::update(LocalTime now)
{
    const std::optional<Lookup> found = lookup(week_, minute_of_week(now));
    if (!found) {
        return std::nullopt;
    }

    if (remaining_min_ > 0) {
        // Count down by the minutes since the last call. A small step back
        // shows up as nearly a whole week forward: it is the clock going back
        // (summer time ends, an SNTP correction), so the switch moment on the
        // wall clock is that much further away again.
        const int passed = minutes_forward(minute_of_week(last_), minute_of_week(now));
        if (passed > kMinutesPerWeek - kMaxClockStepBackMin) {
            remaining_min_ += static_cast<uint32_t>(kMinutesPerWeek - passed);
        } else {
            const auto passed_u = static_cast<uint32_t>(passed);
            // Once at zero it stays there: the manual setpoint is gone for good.
            remaining_min_ = passed_u >= remaining_min_ ? 0 : remaining_min_ - passed_u;
        }
    }
    last_ = now;

    if (remaining_min_ > 0) {
        return ScheduledSetpoint{manual_c_, manual_until_, true};
    }
    return ScheduledSetpoint{found->setpoint_c, local_time(found->next), false};
}

}  // namespace control

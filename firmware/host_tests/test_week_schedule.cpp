// SPDX-License-Identifier: GPL-3.0-or-later
//
// TQ-23: week schedule with a manual setpoint that holds until the next
// switch moment (SPEC section 6, rule 4; ADR-021).
//
// The schedule works like a weekly time-switch clock: per weekday a list of
// switch moments, each with a setpoint. It has no clock of its own; the
// caller passes the local time (weekday + minute of the day), so these tests
// can walk through a week in a few lines.

// assert() is the whole test framework (ADR-016); keep it on in every build.
#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <optional>

#include "control.h"

namespace {

using control::LocalTime;
using control::Schedule;
using control::ScheduledSetpoint;
using control::WeekSchedule;
using namespace control;  // the Weekday constants: kMonday ... kSunday

constexpr uint16_t hm(int hours, int minutes)
{
    return static_cast<uint16_t>(hours * 60 + minutes);
}

bool same_time(LocalTime a, LocalTime b)
{
    return a.weekday == b.weekday && a.minute == b.minute;
}

// Checks one answer of the schedule: setpoint, until when, manual or not.
void expect(const std::optional<ScheduledSetpoint> &got, float setpoint_c,
            LocalTime until, bool manual)
{
    assert(got.has_value());
    assert(got->setpoint_c == setpoint_c);
    assert(same_time(got->until, until));
    assert(got->manual == manual);
}

// A fixed week for the tests below, independent of the default in code:
// every day 06:30 -> 20.0 °C, 22:30 -> 16.0 °C.
WeekSchedule test_week()
{
    WeekSchedule week;
    for (auto &day : week) {
        day = {{hm(6, 30), 20.0f}, {hm(22, 30), 16.0f}};
    }
    return week;
}

// The default schedule in code (ADR-021): weekdays 16:30 -> 18.5 °C,
// Saturday and Sunday from 10:00; every day 21:30 -> 15.5 °C. Before the first
// switch moment the night value of the day before holds.
void test_default_week()
{
    const WeekSchedule week = control::default_week();
    for (int d = kMonday; d <= kSunday; ++d) {
        const auto &day = week[d];
        const uint16_t warm = d >= kSaturday ? hm(10, 0) : hm(16, 30);
        assert(day.size() == 2);
        assert(day[0].minute == warm && day[0].setpoint_c == 18.5f);
        assert(day[1].minute == hm(21, 30) && day[1].setpoint_c == 15.5f);
    }
    Schedule s(week);
    expect(s.update({kTuesday, hm(12, 0)}), 15.5f, {kTuesday, hm(16, 30)}, false);
    expect(s.update({kTuesday, hm(16, 30)}), 18.5f, {kTuesday, hm(21, 30)}, false);
    expect(s.update({kFriday, hm(21, 30)}), 15.5f, {kSaturday, hm(10, 0)}, false);
    expect(s.update({kSaturday, hm(10, 0)}), 18.5f, {kSaturday, hm(21, 30)}, false);
    expect(s.update({kSunday, hm(21, 30)}), 15.5f, {kMonday, hm(16, 30)}, false);
}

// Night setback starts at 22:30 and ends at 06:30; the switch minute itself
// already has the new value.
void test_night_setback()
{
    Schedule s(test_week());
    expect(s.update({kMonday, hm(3, 0)}), 16.0f, {kMonday, hm(6, 30)}, false);
    expect(s.update({kMonday, hm(6, 29)}), 16.0f, {kMonday, hm(6, 30)}, false);
    expect(s.update({kMonday, hm(6, 30)}), 20.0f, {kMonday, hm(22, 30)}, false);
    expect(s.update({kMonday, hm(22, 29)}), 20.0f, {kMonday, hm(22, 30)}, false);
    expect(s.update({kMonday, hm(22, 30)}), 16.0f, {kTuesday, hm(6, 30)}, false);
}

// Over midnight the last value of the day before carries on.
void test_midnight()
{
    Schedule s(test_week());
    expect(s.update({kMonday, hm(23, 59)}), 16.0f, {kTuesday, hm(6, 30)}, false);
    expect(s.update({kTuesday, hm(0, 0)}), 16.0f, {kTuesday, hm(6, 30)}, false);
}

// Sunday to Monday: the week wraps around.
void test_sunday_to_monday()
{
    WeekSchedule week = test_week();
    week[kSunday] = {{hm(8, 0), 21.0f}, {hm(23, 0), 15.0f}};
    week[kMonday] = {{hm(7, 0), 19.0f}};
    Schedule s(week);

    expect(s.update({kSunday, hm(23, 30)}), 15.0f, {kMonday, hm(7, 0)}, false);
    expect(s.update({kMonday, hm(0, 10)}), 15.0f, {kMonday, hm(7, 0)}, false);
    expect(s.update({kMonday, hm(7, 0)}), 19.0f, {kTuesday, hm(6, 30)}, false);
}

// A day without switch moments keeps the last value of the day before, and
// so does a run of empty days.
void test_empty_days()
{
    WeekSchedule week = test_week();
    week[kSaturday].clear();
    Schedule s(week);
    expect(s.update({kSaturday, hm(12, 0)}), 16.0f, {kSunday, hm(6, 30)}, false);

    week[kFriday].clear();
    week[kSunday].clear();
    Schedule t(week);
    expect(t.update({kSunday, hm(12, 0)}), 16.0f, {kMonday, hm(6, 30)}, false);
}

// A manual setpoint holds until the next switch moment, exactly: the minute
// before it is still manual, the switch minute is the schedule again.
void test_manual_until_next_switch()
{
    Schedule s(test_week());
    s.set_manual(22.0f, {kMonday, hm(10, 0)});
    expect(s.update({kMonday, hm(10, 0)}), 22.0f, {kMonday, hm(22, 30)}, true);
    expect(s.update({kMonday, hm(22, 29)}), 22.0f, {kMonday, hm(22, 30)}, true);
    expect(s.update({kMonday, hm(22, 30)}), 16.0f, {kTuesday, hm(6, 30)}, false);
    // Gone for good, not back the next time the clock passes 10:00.
    expect(s.update({kTuesday, hm(10, 0)}), 20.0f, {kTuesday, hm(22, 30)}, false);
}

// A newer manual setpoint replaces the older one.
void test_manual_replaced()
{
    Schedule s(test_week());
    s.set_manual(22.0f, {kMonday, hm(10, 0)});
    s.set_manual(18.5f, {kMonday, hm(11, 0)});
    expect(s.update({kMonday, hm(11, 0)}), 18.5f, {kMonday, hm(22, 30)}, true);
}

// Over midnight and over Sunday to Monday, a manual setpoint still ends at
// the next switch moment.
void test_manual_over_midnight_and_week_end()
{
    Schedule s(test_week());
    s.set_manual(19.0f, {kMonday, hm(23, 0)});
    expect(s.update({kTuesday, hm(3, 0)}), 19.0f, {kTuesday, hm(6, 30)}, true);
    expect(s.update({kTuesday, hm(6, 30)}), 20.0f, {kTuesday, hm(22, 30)}, false);

    s.set_manual(19.0f, {kSunday, hm(23, 0)});
    expect(s.update({kMonday, hm(0, 0)}), 19.0f, {kMonday, hm(6, 30)}, true);
    expect(s.update({kMonday, hm(6, 30)}), 20.0f, {kMonday, hm(22, 30)}, false);
}

// One switch moment in the whole week: the next one is a week later, and a
// manual setpoint made at that very minute holds for that whole week.
void test_single_switch_moment()
{
    WeekSchedule week;
    week[kWednesday] = {{hm(12, 0), 18.0f}};
    Schedule s(week);
    expect(s.update({kMonday, hm(9, 0)}), 18.0f, {kWednesday, hm(12, 0)}, false);
    expect(s.update({kWednesday, hm(12, 0)}), 18.0f, {kWednesday, hm(12, 0)}, false);

    s.set_manual(21.0f, {kWednesday, hm(12, 0)});
    expect(s.update({kSunday, hm(12, 0)}), 21.0f, {kWednesday, hm(12, 0)}, true);
    expect(s.update({kWednesday, hm(11, 59)}), 21.0f, {kWednesday, hm(12, 0)}, true);
    expect(s.update({kWednesday, hm(12, 0)}), 18.0f, {kWednesday, hm(12, 0)}, false);
}

// The clock steps back an hour (summer time ends: 03:00 becomes 02:00). A
// manual setpoint still ends at the switch moment on the wall clock, not an
// hour early or late.
void test_manual_clock_steps_back()
{
    Schedule s(test_week());
    s.set_manual(21.0f, {kSunday, hm(1, 0)});
    expect(s.update({kSunday, hm(3, 0)}), 21.0f, {kSunday, hm(6, 30)}, true);
    expect(s.update({kSunday, hm(2, 0)}), 21.0f, {kSunday, hm(6, 30)}, true);
    expect(s.update({kSunday, hm(6, 29)}), 21.0f, {kSunday, hm(6, 30)}, true);
    expect(s.update({kSunday, hm(6, 30)}), 20.0f, {kSunday, hm(22, 30)}, false);
}

// No switch moments at all is no schedule: no answer, also not after a manual
// setpoint (it has no moment to end at; the caller keeps its own setpoint).
void test_empty_week()
{
    Schedule s(WeekSchedule{});
    assert(!s.update({kMonday, hm(12, 0)}).has_value());
    s.set_manual(21.0f, {kMonday, hm(12, 0)});
    assert(!s.update({kMonday, hm(12, 1)}).has_value());
}

}  // namespace

int main()
{
    test_default_week();
    test_night_setback();
    test_midnight();
    test_sunday_to_monday();
    test_empty_days();
    test_manual_until_next_switch();
    test_manual_replaced();
    test_manual_over_midnight_and_week_end();
    test_single_switch_moment();
    test_manual_clock_steps_back();
    test_empty_week();
    std::printf("TQ-23 week schedule: all tests passed\n");
    return 0;
}

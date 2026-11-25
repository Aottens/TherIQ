// SPDX-License-Identifier: GPL-3.0-or-later
//
// TQ-11: the boiler API contract (ADR-020), pinned down with a fake driver.
//
// FakeBoiler stands in for the EMS driver (TQ-14), the on/off relay and a
// future OpenTherm driver. It does what every driver does: publish a snapshot
// from its own task, and take commands through do_set_*(). The rules shared by
// all drivers (validity, range check, read-only default) live in the base
// class, so testing them once here covers every implementation.

// assert() is the whole test framework (ADR-016); keep it on in every build.
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "boiler_api.h"

namespace {

using boiler::Availability;
using boiler::WriteResult;

class FakeBoiler : public boiler::Boiler {
public:
    using Boiler::publish;  // the test plays the driver task

    int setpoint_calls = 0;
    float last_setpoint = 0.0f;
    int heating_calls = 0;
    bool last_heating = false;
    WriteResult answer = WriteResult::kAccepted;

protected:
    WriteResult do_set_flow_setpoint(float celsius) override
    {
        ++setpoint_calls;
        last_setpoint = celsius;
        return answer;
    }

    WriteResult do_set_heating(bool on) override
    {
        ++heating_calls;
        last_heating = on;
        return answer;
    }
};

// Nothing read yet: every value says so, and is not "0 °C" or "burner off".
void test_unread_is_not_zero()
{
    FakeBoiler fake;
    const boiler::State s = fake.state();
    assert(s.flow_c.availability == Availability::kUnknown);
    assert(!s.flow_c.valid());
    assert(!s.return_c.valid());
    assert(!s.outdoor_c.valid());
    assert(!s.pressure_bar.valid());
    assert(!s.modulation_pct.valid());
    assert(!s.burner_on.valid());
    assert(!s.heating_active.valid());
    assert(!s.fault.valid());
    assert(!s.fault_code.valid());

    // A real 0 (outdoor 0.0 °C on a frosty morning) is valid.
    boiler::State next = s;
    next.outdoor_c = {Availability::kValid, 0.0f, 1000};
    fake.publish(next);
    const boiler::State read = fake.state();
    assert(read.outdoor_c.valid());
    assert(read.outdoor_c.value == 0.0f);
    assert(!read.flow_c.valid());  // the rest is still unknown
}

// A boiler without an outdoor sensor differs from one not read yet.
void test_unsupported_differs_from_unknown()
{
    FakeBoiler fake;
    boiler::State s;
    s.outdoor_c.availability = Availability::kUnsupported;
    fake.publish(s);
    const boiler::State read = fake.state();
    assert(read.outdoor_c.availability == Availability::kUnsupported);
    assert(!read.outdoor_c.valid());
    assert(read.flow_c.availability == Availability::kUnknown);
}

// Age of a value, with the 32-bit millisecond counter wrapping around.
void test_freshness()
{
    boiler::Reading<float> flow{Availability::kValid, 55.0f, 10000};
    assert(flow.fresh(10000, 5000));
    assert(flow.fresh(15000, 5000));
    assert(!flow.fresh(15001, 5000));

    // Updated 1 s before the counter wraps, read 2 s later: 3 s old.
    boiler::Reading<float> wrapped{Availability::kValid, 55.0f, 0xFFFFFFFFu - 999u};
    assert(wrapped.fresh(2000, 5000));
    assert(!wrapped.fresh(2000, 2000));

    // Never fresh if not valid, however recent the timestamp.
    boiler::Reading<float> unknown;
    assert(!unknown.fresh(0, 5000));
}

// Read-only by default (hard rule 2): no command reaches the driver.
void test_read_only_by_default()
{
    FakeBoiler fake;
    assert(!fake.writes_enabled());
    assert(fake.set_flow_setpoint(45.0f) == WriteResult::kWritesDisabled);
    assert(fake.set_heating(true) == WriteResult::kWritesDisabled);
    assert(fake.setpoint_calls == 0);
    assert(fake.heating_calls == 0);

    fake.enable_writes(true);
    assert(fake.set_flow_setpoint(45.0f) == WriteResult::kAccepted);
    assert(fake.setpoint_calls == 1);
    assert(fake.last_setpoint == 45.0f);
    assert(fake.set_heating(true) == WriteResult::kAccepted);
    assert(fake.last_heating);

    fake.enable_writes(false);
    assert(fake.set_heating(false) == WriteResult::kWritesDisabled);
    assert(fake.heating_calls == 1);
}

// Out of range is refused, not clamped, and never reaches the driver.
void test_setpoint_range()
{
    FakeBoiler fake;
    fake.enable_writes(true);

    assert(fake.set_flow_setpoint(boiler::kFlowSetpointMinC) == WriteResult::kAccepted);
    assert(fake.set_flow_setpoint(boiler::kFlowSetpointMaxC) == WriteResult::kAccepted);
    assert(fake.setpoint_calls == 2);

    assert(fake.set_flow_setpoint(boiler::kFlowSetpointMinC - 0.1f) == WriteResult::kOutOfRange);
    assert(fake.set_flow_setpoint(boiler::kFlowSetpointMaxC + 0.1f) == WriteResult::kOutOfRange);
    assert(fake.set_flow_setpoint(NAN) == WriteResult::kOutOfRange);
    assert(fake.set_flow_setpoint(INFINITY) == WriteResult::kOutOfRange);
    assert(fake.setpoint_calls == 2);
    assert(fake.last_setpoint == boiler::kFlowSetpointMaxC);
}

// The driver's own answer comes back unchanged, e.g. the on/off relay that
// cannot take a flow setpoint.
void test_driver_answer_passes_through()
{
    FakeBoiler fake;
    fake.enable_writes(true);
    fake.answer = WriteResult::kNotSupported;
    assert(fake.set_flow_setpoint(45.0f) == WriteResult::kNotSupported);
}

// The fault code is a value like any other; a code is not a fault by itself
// (EMS shows a code such as "-H" during normal operation).
void test_fault_code()
{
    FakeBoiler fake;
    boiler::State s;
    s.fault = {Availability::kValid, false, 100};
    s.fault_code.availability = Availability::kValid;
    std::strcpy(s.fault_code.value.display, "-H");
    s.fault_code.value.number = 200;
    fake.publish(s);

    const boiler::State read = fake.state();
    assert(read.fault.valid() && !read.fault.value);
    assert(std::strcmp(read.fault_code.value.display, "-H") == 0);
    assert(read.fault_code.value.number == 200);
}

}  // namespace

int main()
{
    test_unread_is_not_zero();
    test_unsupported_differs_from_unknown();
    test_freshness();
    test_read_only_by_default();
    test_setpoint_range();
    test_driver_answer_passes_through();
    test_fault_code();
    std::printf("TQ-11 boiler_api: all tests passed\n");
    return 0;
}

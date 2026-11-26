// SPDX-License-Identifier: GPL-3.0-or-later
//
// Boiler API: the one interface between the control loop and the boiler bus
// (ADR-003, ADR-020). Pure C++17, no ESP-IDF, so host tests can use it.
//
// Three implementations, nothing else: the EMS driver (TQ-14), the on/off
// relay fallback and, once it comes back, OpenTherm (ADR-019).
//
// How it is used:
//
//   driver task                          control task, Matter, web
//   -----------                          -------------------------
//   reads the bus, keeps its own State
//   publish(state)  ---- snapshot ---->  state()       copy, any time
//   do_set_*()      <---- command -----  set_flow_setpoint(), set_heating()
//   sends it at its next turn on the bus
//
// Reads never wait on the bus: state() returns the latest snapshot. Commands
// return at once; whether the boiler follows shows up later in state().
//
// Two rules hold for every implementation, so they live here and not in the
// drivers: the API is read-only until enable_writes(true) (hard rule 2), and
// a flow setpoint outside [kFlowSetpointMinC, kFlowSetpointMaxC] is refused.
// The boiler keeps its own protection (hard rule 4): it may lower what we ask,
// and nothing here tries to get around that.

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace boiler {

// Flow setpoints the API passes on. A sanity range, not a safety function:
// the control loop clamps to its own T_min/T_max first (SPEC section 7), and
// the boiler clamps to its own maximum after us. Fixed for now; a lower limit
// per installation (floor heating) comes with the NVS configuration.
constexpr float kFlowSetpointMinC = 10.0f;
constexpr float kFlowSetpointMaxC = 80.0f;

enum class Availability : uint8_t {
    kUnknown,      // not read yet, or lost (start-up, bus down)
    kUnsupported,  // this boiler does not have it, e.g. no outdoor sensor
    kValid,        // `value` holds a value read from the boiler
};

// One value from the boiler, with whether it is there and when it was read.
template <typename T>
struct Reading {
    Availability availability = Availability::kUnknown;
    T value{};
    // When the driver read it, in ms of a monotonic clock (on the device:
    // esp_timer_get_time() / 1000, truncated). Wraps after 49.7 days.
    uint32_t updated_ms = 0;

    bool valid() const { return availability == Availability::kValid; }

    // Valid and at most `max_age_ms` old. Unsigned subtraction gives the right
    // age across a wrap of the counter.
    bool fresh(uint32_t now_ms, uint32_t max_age_ms) const
    {
        return valid() && now_ms - updated_ms <= max_age_ms;
    }
};

// The code the boiler reports, in the form its own display shows it.
// EMS: service code plus error number (e.g. "6A" and 227); EMS also reports a
// code in normal operation ("-H"), so a code alone is not a fault, see
// State::fault. OpenTherm: OEM fault code in `number`, `display` empty.
struct FaultCode {
    char display[4] = {};  // up to 3 characters, NUL-terminated
    uint16_t number = 0;
};

// Everything the control loop reads from the boiler, in one snapshot.
struct State {
    Reading<bool> burner_on;       // flame on
    Reading<bool> heating_active;  // central heating (CV) is being served
    Reading<bool> fault;           // the boiler reports a fault or lock-out
    Reading<FaultCode> fault_code;
    Reading<float> modulation_pct;  // 0..100 %
    Reading<float> flow_c;          // flow (aanvoer) temperature, °C
    Reading<float> return_c;        // return (retour) temperature, °C
    Reading<float> pressure_bar;    // CV water pressure, bar
    Reading<float> outdoor_c;       // outdoor sensor on the boiler, °C
};

enum class WriteResult : uint8_t {
    kAccepted,        // the driver took it and sends it at its next turn
    kWritesDisabled,  // read-only, see enable_writes()
    kOutOfRange,      // outside [kFlowSetpointMinC, kFlowSetpointMaxC], or NaN
    kNotSupported,    // this bus cannot do it (flow setpoint on the relay)
};

// Abstract: a driver derives from it and implements the two do_set_*().
class Boiler {
public:
    virtual ~Boiler() = default;

    // Latest snapshot, copied. Safe from any task.
    State state() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    // Asks the boiler for this flow temperature (°C). Refused while read-only
    // and outside the range above; never clamped, so a wrong value from the
    // control loop shows instead of being hidden.
    WriteResult set_flow_setpoint(float celsius)
    {
        if (!writes_enabled()) {
            return WriteResult::kWritesDisabled;
        }
        // Written so that NaN fails too: every comparison with NaN is false.
        if (!(celsius >= kFlowSetpointMinC && celsius <= kFlowSetpointMaxC)) {
            return WriteResult::kOutOfRange;
        }
        return do_set_flow_setpoint(celsius);
    }

    // Central heating on or off. Refused while read-only.
    WriteResult set_heating(bool on)
    {
        if (!writes_enabled()) {
            return WriteResult::kWritesDisabled;
        }
        return do_set_heating(on);
    }

    // Off at start-up. Writing to a real boiler needs Aernout's go-ahead per
    // test (hard rule 2); whoever switches this on is that go-ahead in code.
    void enable_writes(bool enabled) { writes_enabled_.store(enabled); }
    bool writes_enabled() const { return writes_enabled_.load(); }

protected:
    // Called by the driver task only: replaces the snapshot as a whole, so a
    // reader never sees half of one update.
    void publish(const State &state)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = state;
    }

    // The driver keeps the latest request and sends it on the bus from its
    // own task. Called from the caller's task: store, do not wait on the bus.
    virtual WriteResult do_set_flow_setpoint(float celsius) = 0;
    virtual WriteResult do_set_heating(bool on) = 0;

private:
    mutable std::mutex mutex_;  // guards state_ only, held for one copy
    State state_;
    std::atomic<bool> writes_enabled_{false};
};

}  // namespace boiler

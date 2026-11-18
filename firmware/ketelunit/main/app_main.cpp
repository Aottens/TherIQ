// SPDX-License-Identifier: GPL-3.0-or-later
//
// TherIQ boiler unit — Matter thermostat with a fake room sensor (WP0.2).
//
// The node has two endpoints:
//   endpoint 0  Root Node (mandatory, created by node::create)
//   endpoint 1  Thermostat, heating only (SystemMode Off/Heat)
// LocalTemperature comes from a fake sensor (slow sine around 20 °C) until the
// SHT45 driver replaces it (TQ-07). Nothing drives the boiler yet.

#include <cmath>
#include <cstdint>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <app/server/Server.h>

static const char *TAG = "theriq";

using namespace esp_matter;
using chip::DeviceLayer::ChipDeviceEvent;
namespace DeviceEventType = chip::DeviceLayer::DeviceEventType;
namespace thermostat_ids = esp_matter::cluster::thermostat;

// Matter enum values from the Thermostat cluster specification.
static constexpr uint8_t kControlSequenceHeatingOnly = 2;
static constexpr uint8_t kSystemModeOff = 0;
static constexpr uint8_t kSystemModeHeat = 4;

// Fake sensor: 20.00 °C ± 1.00 °C, one period per 10 minutes, new value every
// 10 s. Matter temperatures are int16 in 0.01 °C, so 2000 means 20.00 °C.
static constexpr double kFakeMeanCentiC = 2000.0;
static constexpr double kFakeAmplitudeCentiC = 100.0;
static constexpr double kFakePeriodS = 600.0;
static constexpr TickType_t kSensorInterval = pdMS_TO_TICKS(10 * 1000);

// Set once in app_main(), read by the sensor task. Written before the task
// starts, so no synchronisation is needed.
static uint16_t s_thermostat_endpoint_id = 0;

static int16_t fake_room_temperature()
{
    const double t_s = esp_timer_get_time() / 1e6;  // time since boot
    const double value = kFakeMeanCentiC + kFakeAmplitudeCentiC * std::sin(2.0 * M_PI * t_s / kFakePeriodS);
    return static_cast<int16_t>(std::lround(value));
}

// FreeRTOS task: pushes a new room temperature into the Matter data model.
// attribute::report() takes the Matter stack lock itself, so calling it from
// this (non-Matter) task is safe. report() instead of update(): the value is
// ours, so our own attribute callback should not fire for it, but subscribed
// controllers (Apple Home, HA) still get the change.
static void sensor_task(void * /*arg*/)
{
    for (;;) {
        vTaskDelay(kSensorInterval);
        esp_matter_attr_val_t val = esp_matter_nullable_int16(fake_room_temperature());
        attribute::report(s_thermostat_endpoint_id, thermostat_ids::Id,
                          thermostat_ids::attribute::LocalTemperature::Id, &val);
    }
}

// Called by the Matter stack (in its own task) for platform events.
// For now we only log what happens during commissioning, so the serial log
// shows whether pairing with Apple Home / Home Assistant progresses.
static void on_matter_event(const ChipDeviceEvent *event, intptr_t /*arg*/)
{
    switch (event->Type) {
    case DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;
    case DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    case DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGW(TAG, "Commissioning failed: fail-safe timer expired");
        break;
    case DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric added, fabrics now: %u",
                 static_cast<unsigned>(chip::Server::GetInstance().GetFabricTable().FabricCount()));
        break;
    case DeviceEventType::kFabricRemoved:
        ESP_LOGI(TAG, "Fabric removed");
        break;
    case DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "IP address changed");
        break;
    default:
        break;
    }
}

// Called before (PRE_UPDATE) and after (POST_UPDATE) an attribute changes, in
// the Matter task. Returning an error from PRE_UPDATE rejects a controller's
// write. We only look at the thermostat endpoint.
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val, void * /*priv_data*/)
{
    if (endpoint_id != s_thermostat_endpoint_id || cluster_id != thermostat_ids::Id) {
        return ESP_OK;
    }

    if (attribute_id == thermostat_ids::attribute::SystemMode::Id) {
        const uint8_t mode = val->val.u8;
        if (type == attribute::PRE_UPDATE && mode != kSystemModeOff && mode != kSystemModeHeat) {
            ESP_LOGW(TAG, "Rejected SystemMode %u: only Off (0) and Heat (4)", mode);
            return ESP_ERR_NOT_SUPPORTED;
        }
        if (type == attribute::POST_UPDATE) {
            ESP_LOGI(TAG, "SystemMode -> %s", mode == kSystemModeHeat ? "Heat" : "Off");
        }
    } else if (attribute_id == thermostat_ids::attribute::OccupiedHeatingSetpoint::Id &&
               type == attribute::POST_UPDATE) {
        const int setpoint = val->val.i16;
        ESP_LOGI(TAG, "OccupiedHeatingSetpoint -> %d.%02d C", setpoint / 100, setpoint % 100);
    }
    return ESP_OK;
}

// Called when a controller asks the device to identify itself ("which one is
// it?"). Later: blink the status LED.
static esp_err_t on_identify(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                             uint8_t /*effect_variant*/, void * /*priv_data*/)
{
    ESP_LOGI(TAG, "Identify: type %u, endpoint %u, effect %u", type, endpoint_id, effect_id);
    return ESP_OK;
}

// ESP-IDF calls app_main() from the "main" FreeRTOS task after boot.
// It is C code's entry point, hence extern "C" in this C++ file.
extern "C" void app_main()
{
    // NVS (non-volatile storage) holds Wi-Fi credentials and Matter fabrics.
    // If the layout changed (new IDF version) or it is full, erase and retry.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // The node is the whole device in Matter's data model. create() also adds
    // endpoint 0 (Root Node) with the mandatory clusters: Basic Information,
    // Access Control, General Commissioning, Network Commissioning, etc.
    node::config_t node_config;
    node_t *node = node::create(&node_config, on_attribute_update, on_identify);
    if (node == nullptr) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        abort();
    }

    // Thermostat endpoint. The config is read once by create(): it becomes the
    // initial attribute values. LocalTemperature gets a real reading here,
    // BEFORE esp_matter::start(): a thermostat that reports null during
    // commissioning can be remembered as "not supported" (SPEC §3).
    const int16_t first = fake_room_temperature();
    endpoint::thermostat::config_t thermostat_config;
    thermostat_config.thermostat.feature_flags = thermostat_ids::feature::heating::get_id();
    thermostat_config.thermostat.control_sequence_of_operation = kControlSequenceHeatingOnly;
    thermostat_config.thermostat.system_mode = kSystemModeHeat;
    thermostat_config.thermostat.local_temperature = first;
    thermostat_config.thermostat.features.heating.occupied_heating_setpoint = 2000;
    endpoint_t *thermostat = endpoint::thermostat::create(node, &thermostat_config, ENDPOINT_FLAG_NONE, nullptr);
    if (thermostat == nullptr) {
        ESP_LOGE(TAG, "Failed to create thermostat endpoint");
        abort();
    }
    s_thermostat_endpoint_id = endpoint::get_id(thermostat);
    ESP_LOGI(TAG, "Thermostat on endpoint %u, LocalTemperature %d.%02d C (fake sensor)",
             s_thermostat_endpoint_id, first / 100, first % 100);

    // Start the Matter stack. Its tasks run from here on; app_main() may return.
    // Without a fabric the device advertises over BLE and waits for pairing;
    // the setup QR code and manual code are printed in the boot log.
    err = esp_matter::start(on_matter_event);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
        abort();
    }

    // 4 KB stack: enough for logging and one attribute report.
    xTaskCreate(sensor_task, "fake_sensor", 4096, nullptr, 5, nullptr);

#if CONFIG_ENABLE_CHIP_SHELL
    // Serial console: `matter help`, `matter onboardingcodes ble`, `matter esp factoryreset`.
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
#endif

    ESP_LOGI(TAG, "TherIQ boiler unit started");
}

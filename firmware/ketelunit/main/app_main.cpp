// SPDX-License-Identifier: GPL-3.0-or-later
//
// TherIQ boiler unit — empty Matter application (WP0.1).
//
// This firmware only creates a Matter node with the mandatory Root Node
// endpoint (endpoint 0) and starts the Matter stack. It can be commissioned
// (paired) but exposes no thermostat yet; that is WP0.2.

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <app/server/Server.h>

static const char *TAG = "theriq";

using namespace esp_matter;
using chip::DeviceLayer::ChipDeviceEvent;
namespace DeviceEventType = chip::DeviceLayer::DeviceEventType;

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

// Called before (PRE_UPDATE) and after (POST_UPDATE) a controller writes an
// attribute. Returning an error from PRE_UPDATE rejects the write. Nothing to
// handle yet; WP0.2 uses this for the thermostat setpoint.
static esp_err_t on_attribute_update(attribute::callback_type_t /*type*/, uint16_t /*endpoint_id*/,
                                     uint32_t /*cluster_id*/, uint32_t /*attribute_id*/,
                                     esp_matter_attr_val_t * /*val*/, void * /*priv_data*/)
{
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

    // Start the Matter stack. Its tasks run from here on; app_main() may return.
    // Without a fabric the device advertises over BLE and waits for pairing;
    // the setup QR code and manual code are printed in the boot log.
    err = esp_matter::start(on_matter_event);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
        abort();
    }

#if CONFIG_ENABLE_CHIP_SHELL
    // Serial console: `matter help`, `matter onboardingcodes ble`, `matter esp factoryreset`.
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
#endif

    ESP_LOGI(TAG, "TherIQ boiler unit started");
}

# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(SRCS "app_main.cpp"
                       PRIV_REQUIRES esp_matter esp_matter_console nvs_flash)

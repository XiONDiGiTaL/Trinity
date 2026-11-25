// SPDX-License-Identifier: GPL-3.0-or-later
// A real NXP plugin: metadata, host/capability discovery, persisted settings,
// a native host panel, a frame callback and logging. It deliberately does not
// touch game memory yet; the phase-2 gameplay bridge is a separate interface.

#include <atomic>
#include <cstdio>
#include <cstring>

#include "nxp/nxp.h"

namespace {

const NxpHostApi* g_host = nullptr;
const NxpUiApi* g_ui = nullptr;
const NxpSettingsApi* g_settings = nullptr;
const NxpFrameApi* g_frame = nullptr;
const NxpFileApi* g_files = nullptr;

int g_enabled = 1;
int g_speed = 100;
std::atomic<uint64_t> g_frames{0};

constexpr const char* kPluginId = "org.xion.nxp.demo";
constexpr const char* kPluginName = "Unified Plugin Demo";
constexpr const char* kPluginVersion = "0.1.0";

void log_line(int level, const char* text) {
    if (g_host && g_host->log) g_host->log(g_host, level, text);
}

void logf_line(int level, const char* fmt, const char* a = nullptr, int b = 0) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, a ? a : "", b);
    log_line(level, buf);
}

void load_settings() {
    if (!g_settings) return;
    g_enabled = g_settings->get_int("demo.enabled", 1) != 0 ? 1 : 0;
    g_speed = g_settings->get_int("demo.speed", 100);
    if (g_speed < 50) g_speed = 50;
    if (g_speed > 200) g_speed = 200;
}

void save_settings() {
    if (!g_settings) return;
    g_settings->set_int("demo.enabled", g_enabled);
    g_settings->set_int("demo.speed", g_speed);
    g_settings->flush();
}

void on_frame(void*) {
    g_frames.fetch_add(1, std::memory_order_relaxed);
}

void draw_panel(const NxpUiCanvas* ui, void*) {
    if (!ui) return;

    ui->text("Unified Plugin Demo");
    if (g_host) {
        char host[256];
        std::snprintf(host, sizeof(host), "%s (%s)", g_host->host_name ? g_host->host_name : "?",
                      g_host->host_version ? g_host->host_version : "?");
        ui->text_disabled(host);
        ui->text_disabled(g_host->host_id ? g_host->host_id : "unknown host");
    }
    ui->separator("Settings");

    int enabled = g_enabled;
    if (ui->checkbox("Enabled##nxp-demo", &enabled)) {
        g_enabled = enabled ? 1 : 0;
        save_settings();
        log_line(NXP_LOG_INFO, g_enabled ? "enabled" : "disabled");
    }
    ui->tooltip("Whether the plugin's future gameplay bridge should run.");

    int speed = g_speed;
    if (ui->slider_int("Speed##nxp-demo", &speed, 50, 200, "%d%%")) {
        g_speed = speed;
        save_settings();
    }
    ui->tooltip("Stored through nxp.settings. Phase 2 will apply this to gameplay.");

    if (ui->button("Reset##nxp-demo")) {
        g_enabled = 1;
        g_speed = 100;
        save_settings();
        log_line(NXP_LOG_INFO, "settings reset");
    }
    ui->same_line();
    if (ui->button("Log state##nxp-demo")) {
        char status[256];
        std::snprintf(status, sizeof(status), "enabled=%d speed=%d frames=%llu", g_enabled, g_speed,
                      static_cast<unsigned long long>(g_frames.load(std::memory_order_relaxed)));
        log_line(NXP_LOG_INFO, status);
    }

    ui->separator(nullptr);
    char status[256];
    std::snprintf(status, sizeof(status), "enabled: %s   speed: %d%%   frames seen: %llu",
                  g_enabled ? "yes" : "no", g_speed,
                  static_cast<unsigned long long>(g_frames.load(std::memory_order_relaxed)));
    ui->text(status);

    if (!g_settings) ui->text_disabled("This host has no settings interface; changes are not persisted.");
    if (!g_frame) ui->text_disabled("This host has no frame interface; no per-frame work is running.");
    if (!g_files) ui->text_disabled("This host has no plugin-scoped file interface.");
}

int plugin_init(const NxpHostApi* host) {
    if (!host || host->abi_version != NXP_ABI_VERSION || host->size < NXP_HOST_API_V1_SIZE) return NXP_ERR_HOST_ABI;
    g_host = host;

    g_ui = static_cast<const NxpUiApi*>(nxp_get_interface(host, NXP_INTERFACE_UI, 1));
    if (g_ui && (g_ui->version < 1 || g_ui->size < NXP_UI_API_V1_SIZE || !g_ui->panel_add)) g_ui = nullptr;
    g_settings = static_cast<const NxpSettingsApi*>(nxp_get_interface(host, NXP_INTERFACE_SETTINGS, 1));
    if (g_settings && (g_settings->version < 1 || g_settings->size < NXP_SETTINGS_API_V1_SIZE)) g_settings = nullptr;
    g_files = static_cast<const NxpFileApi*>(nxp_get_interface(host, NXP_INTERFACE_FILES, 1));
    if (g_files && (g_files->version < 1 || g_files->size < NXP_FILE_API_V1_SIZE)) g_files = nullptr;
    g_frame = static_cast<const NxpFrameApi*>(nxp_get_interface(host, NXP_INTERFACE_FRAME, 1));
    if (g_frame && (g_frame->version < 1 || g_frame->size < NXP_FRAME_API_V1_SIZE)) g_frame = nullptr;

    char hello[512];
    std::snprintf(hello, sizeof(hello), "%s %s on %s (%s), capabilities 0x%08x", kPluginName, kPluginVersion,
                  host->host_id ? host->host_id : "?", host->host_version ? host->host_version : "?", host->capabilities);
    log_line(NXP_LOG_INFO, hello);
    if (g_host->plugin_dir) logf_line(NXP_LOG_DEBUG, "plugin directory: %s", g_host->plugin_dir);

    load_settings();
    save_settings();  // create the settings file with defaults on first run

    if (g_ui) {
        if (!g_ui->panel_add("Unified Plugin Demo", &draw_panel, nullptr)) {
            log_line(NXP_LOG_ERROR, "the host refused the plugin panel");
            return NXP_ERR_INIT;
        }
        log_line(NXP_LOG_INFO, "panel registered with the host");
    } else {
        log_line(NXP_LOG_WARN, "host has no nxp.ui interface; running without a panel");
    }

    if (g_frame) {
        if (!g_frame->on_frame(&on_frame, nullptr)) {
            log_line(NXP_LOG_WARN, "the host refused the frame callback");
            g_frame = nullptr;
        }
    }

    return NXP_OK;
}

void plugin_shutdown() {
    save_settings();
    log_line(NXP_LOG_INFO, "shutdown");
}

const NxpPluginDesc kDesc = {
    NXP_ABI_VERSION,
    sizeof(NxpPluginDesc),
    kPluginId,
    kPluginName,
    kPluginVersion,
    "A shared NXP plugin demonstrating host discovery, native UI, settings, frame callbacks and logging.",
    &plugin_init,
    &plugin_shutdown,
};

}  // namespace

NXP_EXPORT const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version) {
    (void)host_abi_version;
    return &kDesc;
}

// nxmod_init is supplied by src/nxmod_adapter.cpp. It exports the same plugin
// through the existing EmblemWeave / fefw-recomp / ObeliskChamber loader.

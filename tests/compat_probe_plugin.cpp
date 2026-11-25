// SPDX-License-Identifier: GPL-3.0-or-later
// Probe plugin used by the compatibility-adapter test. It records what the
// adapter exposed; it does not depend on a host or a game.

#include "nxp/nxp.h"

#include <cstring>

namespace probe {

NxpHostApi seen{};
const NxpSettingsApi* settings = nullptr;
const NxpFileApi* files = nullptr;
const NxpFrameApi* frame = nullptr;
const NxpUiApi* ui = nullptr;
int init_count = 0;
int shutdown_count = 0;
int panel_calls = 0;
int frame_calls = 0;

void panel(const NxpUiCanvas* canvas, void*) {
    if (canvas && canvas->size == sizeof(NxpUiCanvas) && canvas->version == 1) panel_calls += 1;
}

int init(const NxpHostApi* host) {
    if (!host) return NXP_ERR_ARGUMENT;
    seen = *host;
    init_count += 1;
    settings = static_cast<const NxpSettingsApi*>(nxp_get_interface(host, NXP_INTERFACE_SETTINGS, 1));
    files = static_cast<const NxpFileApi*>(nxp_get_interface(host, NXP_INTERFACE_FILES, 1));
    frame = static_cast<const NxpFrameApi*>(nxp_get_interface(host, NXP_INTERFACE_FRAME, 1));
    ui = static_cast<const NxpUiApi*>(nxp_get_interface(host, NXP_INTERFACE_UI, 1));
    if (ui && ui->panel_add) ui->panel_add("Probe panel", &panel, nullptr);
    return NXP_OK;
}

void shutdown() {
    shutdown_count += 1;
}

}  // namespace probe

extern "C" void probe_reset() {
    probe::seen = {};
    probe::settings = nullptr;
    probe::files = nullptr;
    probe::frame = nullptr;
    probe::ui = nullptr;
    probe::init_count = 0;
    probe::shutdown_count = 0;
    probe::panel_calls = 0;
    probe::frame_calls = 0;
}

extern "C" const NxpHostApi* probe_host() { return &probe::seen; }
extern "C" const NxpSettingsApi* probe_settings() { return probe::settings; }
extern "C" const NxpFileApi* probe_files() { return probe::files; }
extern "C" const NxpFrameApi* probe_frame() { return probe::frame; }
extern "C" const NxpUiApi* probe_ui() { return probe::ui; }
extern "C" int probe_init_count() { return probe::init_count; }
extern "C" int probe_shutdown_count() { return probe::shutdown_count; }
extern "C" int probe_panel_calls() { return probe::panel_calls; }
extern "C" int probe_frame_calls() { return probe::frame_calls; }

extern "C" void probe_frame_tick(void*) { probe::frame_calls += 1; }

NXP_EXPORT const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version) {
    static const NxpPluginDesc desc{
        NXP_ABI_VERSION,
        sizeof(NxpPluginDesc),
        "test.trinity.probe",
        "Compatibility probe",
        "1.0.0",
        "Records the host table built by the NXMod compatibility adapter.",
        &probe::init,
        &probe::shutdown
    };
    return host_abi_version == NXP_ABI_VERSION ? &desc : nullptr;
}

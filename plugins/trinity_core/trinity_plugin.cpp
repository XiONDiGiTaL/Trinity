// SPDX-License-Identifier: GPL-3.0-or-later
//
// Trinity Core: the reference NXP plugin. It proves the common lifecycle and
// the shared registry without touching game memory or gameplay state.
//
// The same source can be loaded through:
//   - nxp_get_plugin, by a native NXP host; or
//   - nxmod_init, through hosts/compat/nxmod_adapter.cpp, by the three existing
//     NxModApi runtimes.

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
int g_prefix_guard = 1;
std::atomic<uint64_t> g_frames{0};
char g_last_check[192] = "Registry not checked yet.";
char g_host_prefix[64] = "trinity.";
char g_host_label[64] = "Shared host";

constexpr const char* kPluginId = "org.xion.trinity.core";
constexpr const char* kPluginName = "Trinity Core";
constexpr const char* kPluginVersion = "0.1.0";

struct House {
    const char* id;
    const char* name;
    const char* prefix;
    const char* api;
};

constexpr House kHouses[] = {
    {"emblemweave", "EmblemWeave", "emblemweave.", "nxmod-api-4"},
    {"fefw-recomp", "fefw-recomp", "fefw-recomp.", "nxmod-api-3"},
    {"obelisk-chamber", "ObeliskChamber", "obeliskchamber.", "nxmod-api-5"},
};

constexpr const char* kNeutral[] = {"trinity.", "nxp."};

bool starts_with(const char* value, const char* prefix) {
    if (!value || !prefix) return false;
    const size_t n = std::strlen(prefix);
    return std::strncmp(value, prefix, n) == 0;
}

bool valid_segment(const char* begin, const char* end) {
    if (begin == end) return false;
    for (const char* p = begin; p != end; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    }
    return true;
}

// Returns the owning canonical prefix, "trinity." for portable names, or
// nullptr for a malformed or unregistered name. This mirrors
// shared/prefix-core.mjs and is deliberately dependency-free.
const char* prefix_owner(const char* name) {
    if (!name || !*name) return nullptr;
    const char* p = name;
    while (*p) {
        if (*p >= 'A' && *p <= 'Z') return nullptr;
        ++p;
    }
    const size_t len = std::strlen(name);
    if (name[len - 1] == '.') return nullptr;

    // Validate the shape before matching an owner, so a doubled dot inside a
    // known namespace is rejected rather than silently accepted.
    const char* segment = name;
    const char* dot = std::strchr(name, '.');
    if (!dot) return nullptr;
    if (!valid_segment(segment, dot)) return nullptr;
    while (dot) {
        segment = dot + 1;
        dot = std::strchr(segment, '.');
        if (!valid_segment(segment, dot ? dot : name + len)) return nullptr;
    }

    for (const char* neutral : kNeutral) {
        if (starts_with(name, neutral) && name[std::strlen(neutral)]) return neutral;
    }
    for (const House& house : kHouses) {
        if (starts_with(name, house.prefix) && name[std::strlen(house.prefix)]) return house.prefix;
    }
    return nullptr;
}

void log_line(int level, const char* text) {
    if (g_host && g_host->log) g_host->log(g_host, level, text);
}

void logf_line(int level, const char* fmt, const char* a = nullptr, const char* b = nullptr) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, a ? a : "", b ? b : "");
    log_line(level, buf);
}

void choose_host() {
    const char* id = g_host && g_host->host_id ? g_host->host_id : "";
    for (const House& house : kHouses) {
        if (std::strcmp(id, house.id) == 0 ||
            (std::strcmp(id, "obeliskchamber") == 0 && std::strcmp(house.id, "obelisk-chamber") == 0)) {
            std::snprintf(g_host_prefix, sizeof(g_host_prefix), "%s", house.prefix);
            std::snprintf(g_host_label, sizeof(g_host_label), "%s", house.name);
            return;
        }
    }
    std::snprintf(g_host_prefix, sizeof(g_host_prefix), "%s", "trinity.");
    std::snprintf(g_host_label, sizeof(g_host_label), "%s",
                  g_host && g_host->host_name ? g_host->host_name : "Shared host");
}

void load_settings() {
    if (!g_settings) return;
    g_enabled = g_settings->get_int("trinity.enabled", 1) != 0 ? 1 : 0;
    g_prefix_guard = g_settings->get_int("trinity.prefix_guard", 1) != 0 ? 1 : 0;
}

void save_settings() {
    if (!g_settings) return;
    g_settings->set_int("trinity.enabled", g_enabled);
    g_settings->set_int("trinity.prefix_guard", g_prefix_guard);
    g_settings->flush();
}

void check_registry() {
    constexpr const char* samples[] = {
        "trinity.core.panel",
        "nxp.ui",
        "emblemweave.ui.panel",
        "fefw-recomp.ui.panel",
        "obeliskchamber.ui.panel",
        "other.ui.panel",
    };
    int accepted = 0;
    int rejected = 0;
    for (const char* sample : samples) {
        const char* owner = prefix_owner(sample);
        if (owner) {
            ++accepted;
            logf_line(NXP_LOG_INFO, "prefix accepted: %s -> %s", sample, owner);
        } else {
            ++rejected;
            logf_line(NXP_LOG_WARN, "prefix rejected: %s", sample);
        }
    }
    std::snprintf(g_last_check, sizeof(g_last_check), "%d names accepted, %d rejected.", accepted, rejected);
    log_line(NXP_LOG_INFO, g_last_check);
}

void on_frame(void*) {
    const uint64_t frames = g_frames.fetch_add(1, std::memory_order_relaxed) + 1;
    if (frames % 600 == 0) {
        char line[96];
        std::snprintf(line, sizeof(line), "Trinity Core frame heartbeat: %llu",
                      static_cast<unsigned long long>(frames));
        log_line(NXP_LOG_DEBUG, line);
    }
}

void draw_panel(const NxpUiCanvas* ui, void*) {
    if (!ui) return;

    ui->text("TRINITY");
    ui->text_disabled("The Three Houses Of Innovation");
    if (g_host) {
        char host[320];
        std::snprintf(host, sizeof(host), "%s  /  %s  /  %s",
                      g_host->host_name ? g_host->host_name : "?",
                      g_host->host_id ? g_host->host_id : "?",
                      g_host->host_version ? g_host->host_version : "?");
        ui->text_disabled(host);
    }
    char current[160];
    std::snprintf(current, sizeof(current), "House of %s   %s", g_host_label, g_host_prefix);
    ui->text(current);

    ui->separator("Houses");
    for (const House& house : kHouses) {
        char line[256];
        std::snprintf(line, sizeof(line), "%s  %s  %s%s", house.name, house.prefix, house.api,
                      std::strcmp(g_host_prefix, house.prefix) == 0 ? "  [current]" : "");
        if (std::strcmp(g_host_prefix, house.prefix) == 0) ui->text(line);
        else ui->text_disabled(line);
    }
    ui->text_disabled("trinity.  portable shared services");
    ui->text_disabled("nxp.      versioned compatibility ABI");

    ui->separator("Settings");
    int enabled = g_enabled;
    if (ui->checkbox("Enabled##trinity", &enabled)) {
        g_enabled = enabled ? 1 : 0;
        save_settings();
        log_line(NXP_LOG_INFO, g_enabled ? "Trinity enabled" : "Trinity disabled");
    }
    ui->tooltip("Enables the shared registry and future shared services. It does not alter game state yet.");

    int guard = g_prefix_guard;
    if (ui->checkbox("Enforce host prefixes##trinity", &guard)) {
        g_prefix_guard = guard ? 1 : 0;
        save_settings();
        log_line(NXP_LOG_INFO, g_prefix_guard ? "prefix guard on" : "prefix guard off");
    }
    ui->tooltip("Reject interface names that do not belong to a registered host or neutral namespace.");

    if (ui->button("Check registry##trinity")) check_registry();
    ui->same_line();
    if (ui->button("Reset##trinity")) {
        g_enabled = 1;
        g_prefix_guard = 1;
        std::snprintf(g_last_check, sizeof(g_last_check), "Registry not checked yet.");
        save_settings();
        log_line(NXP_LOG_INFO, "Trinity settings reset");
    }

    ui->separator(nullptr);
    char status[320];
    std::snprintf(status, sizeof(status), "%s  |  frames seen: %llu  |  prefix guard: %s",
                  g_last_check, static_cast<unsigned long long>(g_frames.load(std::memory_order_relaxed)),
                  g_prefix_guard ? "on" : "off");
    ui->text(status);
    ui->text_disabled(g_enabled ? "Shared services: enabled" : "Shared services: paused");
    if (!g_ui) ui->text_disabled("This host has no panel interface.");
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

    choose_host();
    load_settings();
    save_settings();

    char line[512];
    std::snprintf(line, sizeof(line), "%s %s on %s (%s), house prefix %s, capabilities 0x%08x",
                  kPluginName, kPluginVersion, host->host_id ? host->host_id : "?",
                  host->host_version ? host->host_version : "?", g_host_prefix, host->capabilities);
    log_line(NXP_LOG_INFO, line);
    if (host->plugin_dir) logf_line(NXP_LOG_DEBUG, "plugin directory: %s", host->plugin_dir);

    check_registry();

    if (g_ui) {
        if (!g_ui->panel_add("Trinity", &draw_panel, nullptr)) {
            log_line(NXP_LOG_ERROR, "the host refused the Trinity panel");
            return NXP_ERR_INIT;
        }
        log_line(NXP_LOG_INFO, "panel registered");
    } else {
        log_line(NXP_LOG_WARN, "host has no nxp.ui interface; Trinity runs headless");
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
    "The reference plugin for the Trinity cross-host framework: host discovery, a native panel, settings, a frame hook and the project prefix registry.",
    &plugin_init,
    &plugin_shutdown,
};

}  // namespace

NXP_EXPORT const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version) {
    (void)host_abi_version;
    return &kDesc;
}

// nxmod_init is supplied by hosts/compat/nxmod_adapter.cpp.

// SPDX-License-Identifier: GPL-3.0-or-later
// NXP compatibility host for the existing nxrt code-mod ABI.
//
// This file is linked into an NXP plugin DLL. It exports nxmod_init, which the
// existing EmblemWeave / fefw-recomp / ObeliskChamber loaders already call, and
// builds the NXP host table from the NxModApi they pass in. A future native host
// adapter can implement the same NXP table directly and call nxp_get_plugin.
//
// The adapter is deliberately conservative: it exposes only capabilities that
// the runtime actually provides, and it never pretends that a missing interface
// exists. A plugin can therefore run unchanged on a v4 host without UI and on a
// v4 host with panels.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "nxp/nxp.h"
#include "nxrt/mod_api.h"

namespace {

constexpr const char* kSettingsFile = ".nxp-settings.ini";

const NxModApi* g_api = nullptr;
const NxpPluginDesc* g_desc = nullptr;
NxpHostApi g_host{};   // must outlive nxmod_init: the plugin may keep this pointer
std::string g_plugin_dir;
std::string g_host_id;
std::string g_host_name;
std::string g_host_version;

bool g_ui_available = false;
bool g_settings_available = false;
bool g_files_available = false;
bool g_frame_available = false;
bool g_initialized = false;

std::mutex g_settings_mu;
std::map<std::string, std::string> g_settings;

std::string join(const std::string& dir, const std::string& name) {
    if (dir.empty()) return name;
    const char last = dir.back();
    return dir + ((last == '/' || last == '\\') ? "" : "/") + name;
}

std::string module_path() {
    wchar_t buf[32768];
    const DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    if (n == 0 || n >= std::size(buf)) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, buf, static_cast<int>(n), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, buf, static_cast<int>(n), out.data(), bytes, nullptr, nullptr) != bytes) return {};
    return out;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

void detect_host() {
    const std::string path = lower(module_path());
    if (path.find("\\emblemweave\\") != std::string::npos || path.find("/emblemweave/") != std::string::npos) {
        g_host_id = "emblemweave";
        g_host_name = "EmblemWeave";
    } else if (path.find("obeliskchamber") != std::string::npos) {
        g_host_id = "obelisk-chamber";
        g_host_name = "ObeliskChamber";
    } else if (path.find("fefw-recomp") != std::string::npos) {
        g_host_id = "fefw-recomp";
        g_host_name = "fefw-recomp";
    } else {
        // A host path is not an ABI. Plugins must use capabilities, not this
        // fallback, for behaviour. It exists so diagnostics still identify the
        // process when the adapter is used from a development harness.
        g_host_id = "nxmod-compatible";
        g_host_name = "NXMod-compatible host";
    }
    g_host_version = "nxmod-api-" + std::to_string(g_api ? g_api->version : 0);
}

void host_log(const NxpHostApi*, int level, const char* message) {
    if (!g_api || !g_api->log) return;
    if (level < NXP_LOG_ERROR) level = NXP_LOG_ERROR;
    if (level > NXP_LOG_DEBUG) level = NXP_LOG_DEBUG;
    g_api->log(g_api, level, message ? message : "");
}

bool valid_key(const char* key) {
    if (!key || !*key) return false;
    for (const unsigned char c : std::string(key)) {
        if (!(std::isalnum(c) || c == '_' || c == '.' || c == '-')) return false;
    }
    return true;
}

std::string* find_setting(const char* key) {
    if (!key) return nullptr;
    const auto it = g_settings.find(key);
    return it == g_settings.end() ? nullptr : &it->second;
}

void load_settings() {
    if (!g_api || !g_api->read_file) return;
    const std::string path = join(g_plugin_dir, kSettingsFile);
    const int64_t size = g_api->read_file(path.c_str(), nullptr, 0);
    if (size <= 0) return;
    std::vector<char> data(static_cast<size_t>(size));
    if (g_api->read_file(path.c_str(), data.data(), data.size()) != size) return;
    std::string text(data.begin(), data.end());
    size_t at = 0;
    while (at <= text.size()) {
        const size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() + 1 : end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        if (key.empty() || key[0] == '#' || key[0] == ';') continue;
        if (!valid_key(key.c_str())) continue;
        g_settings.emplace(std::move(key), std::move(value));
    }
}

int flush_locked() {
    if (!g_api || !g_api->write_file) return 0;
    std::string text;
    for (const auto& [key, value] : g_settings) {
        text += key;
        text += '=';
        text += value;
        text += '\n';
    }
    const std::string path = join(g_plugin_dir, kSettingsFile);
    return g_api->write_file(path.c_str(), text.data(), text.size()) ? 1 : 0;
}

int settings_get_int(const char* key, int fallback) {
    if (!valid_key(key)) return fallback;
    std::lock_guard<std::mutex> lock(g_settings_mu);
    const std::string* v = find_setting(key);
    if (!v || v->empty()) return fallback;
    char* end = nullptr;
    const long n = std::strtol(v->c_str(), &end, 10);
    if (end == v->c_str() || *end != '\0' || n < INT_MIN || n > INT_MAX) return fallback;
    return static_cast<int>(n);
}

int settings_set_int(const char* key, int value) {
    if (!valid_key(key)) return 0;
    std::lock_guard<std::mutex> lock(g_settings_mu);
    g_settings[key] = std::to_string(value);
    return flush_locked();
}

int settings_get_string(const char* key, char* out, uint32_t cap, const char* fallback) {
    if (!valid_key(key)) return -1;
    std::lock_guard<std::mutex> lock(g_settings_mu);
    const std::string* v = find_setting(key);
    const std::string value = v ? *v : (fallback ? fallback : "");
    if (out && cap) {
        const size_t n = std::min<size_t>(value.size(), cap - 1);
        std::memcpy(out, value.data(), n);
        out[n] = '\0';
    }
    return value.size() > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(value.size());
}

int settings_set_string(const char* key, const char* value) {
    if (!valid_key(key) || !value) return 0;
    const std::string v(value);
    if (v.find('\n') != std::string::npos || v.find('\r') != std::string::npos) return 0;
    std::lock_guard<std::mutex> lock(g_settings_mu);
    g_settings[key] = v;
    return flush_locked();
}

int settings_flush() {
    std::lock_guard<std::mutex> lock(g_settings_mu);
    return flush_locked();
}

std::string resolve_plugin_path(const char* path) {
    if (!path || !*path) return {};
    std::string p = path;
    std::replace(p.begin(), p.end(), '\\', '/');
    if (p[0] == '/' || (p.size() > 1 && p[1] == ':')) return {};
    size_t at = 0;
    while (at <= p.size()) {
        const size_t slash = p.find('/', at);
        const std::string part = p.substr(at, slash == std::string::npos ? std::string::npos : slash - at);
        if (part.empty() || part == "." || part == "..") return {};
        if (slash == std::string::npos) break;
        at = slash + 1;
    }
    return join(g_plugin_dir, p);
}

int64_t file_read(const char* path, void* buf, uint64_t cap) {
    if (!g_api || !g_api->read_file) return -1;
    const std::string full = resolve_plugin_path(path);
    if (full.empty()) return -1;
    return g_api->read_file(full.c_str(), buf, cap);
}

int file_write(const char* path, const void* data, uint64_t size) {
    if (!g_api || !g_api->write_file) return 0;
    const std::string full = resolve_plugin_path(path);
    if (full.empty()) return 0;
    return g_api->write_file(full.c_str(), data, size) ? 1 : 0;
}

struct PanelBinding {
    NxpPanelFn fn;
    void* user;
};

void panel_trampoline(const NxModUi* host_ui, void* user) {
    auto* binding = static_cast<PanelBinding*>(user);
    if (!binding || !binding->fn) return;
    if (!host_ui) return;
    NxpUiCanvas canvas{};
    std::memcpy(&canvas, host_ui, sizeof(canvas));
    canvas.size = sizeof(NxpUiCanvas);
    canvas.version = 1;
    binding->fn(&canvas, binding->user);
}

int panel_add(const char* title, NxpPanelFn fn, void* user) {
    if (!g_api || !g_api->add_panel || !title || !*title || !fn) return 0;
    auto* binding = new PanelBinding{fn, user};
    if (!g_api->add_panel(title, &panel_trampoline, binding)) {
        delete binding;
        return 0;
    }
    return 1;
}

int panel_visible() {
    return g_api && g_api->panel_visible ? g_api->panel_visible() : 0;
}

struct FrameBinding {
    NxpFrameFn fn;
    void* user;
};

void frame_trampoline(::Ctx*, void* user) {
    auto* binding = static_cast<FrameBinding*>(user);
    if (binding && binding->fn) binding->fn(binding->user);
}

int frame_on_frame(NxpFrameFn fn, void* user) {
    if (!g_api || !g_api->on_frame || !fn) return 0;
    auto* binding = new FrameBinding{fn, user};
    g_api->on_frame(&frame_trampoline, binding);
    return 1;
}

NxpUiApi g_ui{};
NxpSettingsApi g_settings_api{};
NxpFileApi g_files_api{};
NxpFrameApi g_frame_api{};

const void* host_get_interface(const NxpHostApi*, const char* name, uint32_t version) {
    if (!name || version > 1) return nullptr;
    if (std::strcmp(name, NXP_INTERFACE_UI) == 0) return g_ui_available ? &g_ui : nullptr;
    if (std::strcmp(name, NXP_INTERFACE_SETTINGS) == 0) return g_settings_available ? &g_settings_api : nullptr;
    if (std::strcmp(name, NXP_INTERFACE_FILES) == 0) return g_files_available ? &g_files_api : nullptr;
    if (std::strcmp(name, NXP_INTERFACE_FRAME) == 0) return g_frame_available ? &g_frame_api : nullptr;
    return nullptr;
}

void build_host(NxpHostApi* host) {
    detect_host();

    g_ui_available = g_api->version >= 4 && g_api->add_panel && g_api->panel_visible;
    g_settings_available = g_api->version >= 3 && g_api->read_file && g_api->write_file;
    g_files_available = g_api->version >= 3 && g_api->read_file && g_api->write_file;
    g_frame_available = g_api->on_frame != nullptr;

    g_ui = {sizeof(NxpUiApi), 1, &panel_add, &panel_visible};
    g_settings_api = {sizeof(NxpSettingsApi), 1, &settings_get_int, &settings_set_int,
                      &settings_get_string, &settings_set_string, &settings_flush};
    g_files_api = {sizeof(NxpFileApi), 1, &file_read, &file_write};
    g_frame_api = {sizeof(NxpFrameApi), 1, &frame_on_frame};

    *host = {};
    host->abi_version = NXP_ABI_VERSION;
    host->size = sizeof(NxpHostApi);
    host->host_id = g_host_id.c_str();
    host->host_name = g_host_name.c_str();
    host->host_version = g_host_version.c_str();
    host->plugin_dir = g_plugin_dir.c_str();
    host->capabilities = NXP_CAP_HOST_INFO | NXP_CAP_LOG;
    if (g_ui_available) host->capabilities |= NXP_CAP_UI_PANEL | NXP_CAP_UI_WIDGETS;
    if (g_settings_available) host->capabilities |= NXP_CAP_SETTINGS;
    if (g_files_available) host->capabilities |= NXP_CAP_FILES;
    if (g_frame_available) host->capabilities |= NXP_CAP_FRAME;
    host->log = &host_log;
    host->get_interface = &host_get_interface;

    if (g_settings_available) load_settings();
}

void shutdown_once() {
    if (!g_initialized) return;
    g_initialized = false;
    if (g_desc && g_desc->shutdown) g_desc->shutdown();
}

// The layout of NxpUiCanvas v1 is intentionally a copy of NxModUi v4. These
// checks make a host-ABI change fail at compile time instead of at the first
// widget call.
static_assert(sizeof(NxpUiCanvas) == sizeof(NxModUi), "NxpUiCanvas v1 must match NxModUi v4");
static_assert(offsetof(NxpUiCanvas, text) == offsetof(NxModUi, text), "text offset");
static_assert(offsetof(NxpUiCanvas, text_disabled) == offsetof(NxModUi, text_disabled), "text_disabled offset");
static_assert(offsetof(NxpUiCanvas, text_colored) == offsetof(NxModUi, text_colored), "text_colored offset");
static_assert(offsetof(NxpUiCanvas, checkbox) == offsetof(NxModUi, checkbox), "checkbox offset");
static_assert(offsetof(NxpUiCanvas, slider_float) == offsetof(NxModUi, slider_float), "slider_float offset");
static_assert(offsetof(NxpUiCanvas, slider_int) == offsetof(NxModUi, slider_int), "slider_int offset");
static_assert(offsetof(NxpUiCanvas, button) == offsetof(NxModUi, button), "button offset");
static_assert(offsetof(NxpUiCanvas, same_line) == offsetof(NxModUi, same_line), "same_line offset");
static_assert(offsetof(NxpUiCanvas, separator) == offsetof(NxModUi, separator), "separator offset");
static_assert(offsetof(NxpUiCanvas, begin_disabled) == offsetof(NxModUi, begin_disabled), "begin_disabled offset");
static_assert(offsetof(NxpUiCanvas, end_disabled) == offsetof(NxModUi, end_disabled), "end_disabled offset");
static_assert(offsetof(NxpUiCanvas, tree_node) == offsetof(NxModUi, tree_node), "tree_node offset");
static_assert(offsetof(NxpUiCanvas, tree_pop) == offsetof(NxModUi, tree_pop), "tree_pop offset");
static_assert(offsetof(NxpUiCanvas, tooltip) == offsetof(NxModUi, tooltip), "tooltip offset");

}  // namespace

extern "C" const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version);

extern "C" __declspec(dllexport) int nxmod_init(const NxModApi* api, const char* mod_dir) {
    if (!api || !mod_dir || !*mod_dir) return 1;

    g_api = api;
    g_plugin_dir = mod_dir;
    detect_host();

    const NxpPluginDesc* desc = nxp_get_plugin(NXP_ABI_VERSION);
    if (!desc || desc->abi_version != NXP_ABI_VERSION || desc->size < NXP_PLUGIN_DESC_V1_SIZE || !desc->init) {
        host_log(nullptr, NXP_LOG_ERROR, "plugin does not export a compatible nxp_get_plugin descriptor");
        return 1;
    }
    g_desc = desc;

    build_host(&g_host);

    if (api->version < 4) {
        host_log(nullptr, NXP_LOG_WARN, "host has no NXP UI capability (nxmod API below 4); plugin will run headless");
    }

    const int rc = desc->init(&g_host);
    if (rc != NXP_OK) {
        host_log(nullptr, NXP_LOG_ERROR, "plugin initialization failed");
        return 1;
    }

    g_initialized = true;
    std::atexit(&shutdown_once);
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
//
// A host harness for CI and for checking the adapter without a game. It
// implements NxpHostApi directly, loads a plugin DLL, and exercises discovery,
// init, settings, the panel, the frame callback and shutdown.
//
// Usage: nxp_host_test <plugin.dll> [trinity|demo]
// The optional mode only selects which persisted keys the harness inspects.

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "nxp/nxp.h"

namespace {

#ifdef _WIN32
using ModuleHandle = HMODULE;
ModuleHandle load_module(const char* path) { return LoadLibraryA(path); }
void close_module(ModuleHandle module) { FreeLibrary(module); }
void* module_symbol(ModuleHandle module, const char* name) { return reinterpret_cast<void*>(GetProcAddress(module, name)); }
std::string load_error_text() { return std::to_string(static_cast<unsigned long>(GetLastError())); }
#else
using ModuleHandle = void*;
ModuleHandle load_module(const char* path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
void close_module(ModuleHandle module) { if (module) dlclose(module); }
void* module_symbol(ModuleHandle module, const char* name) { return dlsym(module, name); }
std::string load_error_text() { const char* error = dlerror(); return error ? error : "unknown dlopen error"; }
#endif

struct MockState {
    std::map<std::string, std::string> settings;
    bool settings_flushed = false;
    int writes = 0;
    int panel_adds = 0;
    bool panel_visible = false;
    NxpPanelFn panel = nullptr;
    void* panel_user = nullptr;
    NxpFrameFn frame = nullptr;
    void* frame_user = nullptr;
    std::vector<std::string> logs;
    std::vector<std::string> text;
    bool toggle_checkbox = false;
    bool toggle_slider = false;
    bool press_reset = false;
} g;

void host_log(const NxpHostApi*, int level, const char* message) {
    char prefix[16];
    std::snprintf(prefix, sizeof(prefix), "[%d] ", level);
    g.logs.push_back(prefix + std::string(message ? message : ""));
}

int settings_get_int(const char* key, int fallback) {
    const auto it = g.settings.find(key ? key : "");
    if (it == g.settings.end()) return fallback;
    return std::atoi(it->second.c_str());
}

int settings_set_int(const char* key, int value) {
    if (!key) return 0;
    g.settings[key] = std::to_string(value);
    ++g.writes;
    return 1;
}

int settings_get_string(const char* key, char* out, uint32_t cap, const char* fallback) {
    const auto it = g.settings.find(key ? key : "");
    const std::string value = it == g.settings.end() ? (fallback ? fallback : "") : it->second;
    if (out && cap) {
        const size_t n = value.size() < cap - 1 ? value.size() : cap - 1;
        std::memcpy(out, value.data(), n);
        out[n] = '\0';
    }
    return static_cast<int>(value.size());
}

int settings_set_string(const char* key, const char* value) {
    if (!key || !value) return 0;
    g.settings[key] = value;
    ++g.writes;
    return 1;
}

int settings_flush() {
    g.settings_flushed = true;
    return 1;
}

int64_t file_read(const char*, void*, uint64_t) { return -1; }
int file_write(const char*, const void*, uint64_t) { return 0; }

int ui_panel_add(const char* title, NxpPanelFn fn, void* user) {
    if (!title || !*title || !fn) return 0;
    ++g.panel_adds;
    g.panel = fn;
    g.panel_user = user;
    return 1;
}

int ui_panel_visible() { return g.panel_visible ? 1 : 0; }

void ui_text(const char* text) { g.text.push_back(text ? text : ""); }
void ui_text_disabled(const char* text) { g.text.push_back(text ? text : ""); }
void ui_text_colored(uint32_t, const char* text) { g.text.push_back(text ? text : ""); }

int ui_checkbox(const char*, int* value) {
    if (!value) return 0;
    if (g.toggle_checkbox) {
        *value = *value ? 0 : 1;
        g.toggle_checkbox = false;
        return 1;
    }
    return 0;
}

int ui_slider_float(const char*, float*, float, float, const char*) { return 0; }

int ui_slider_int(const char*, int* value, int min, int, const char*) {
    if (!value) return 0;
    if (g.toggle_slider) {
        *value = min + 10;
        g.toggle_slider = false;
        return 1;
    }
    return 0;
}

int ui_button(const char* label) {
    if (g.press_reset && label && std::strncmp(label, "Reset", 5) == 0) {
        g.press_reset = false;
        return 1;
    }
    return 0;
}

void ui_same_line() {}
void ui_separator(const char*) {}
void ui_begin_disabled(int) {}
void ui_end_disabled() {}
int ui_tree_node(const char*, int) { return 0; }
void ui_tree_pop() {}
void ui_tooltip(const char*) {}

NxpUiApi g_ui{};
NxpSettingsApi g_settings{};
NxpFileApi g_files{};
NxpFrameApi g_frame{};

const void* host_get_interface(const NxpHostApi*, const char* name, uint32_t version) {
    if (!name || version > 1) return nullptr;
    if (std::strcmp(name, NXP_INTERFACE_UI) == 0) return &g_ui;
    if (std::strcmp(name, NXP_INTERFACE_SETTINGS) == 0) return &g_settings;
    if (std::strcmp(name, NXP_INTERFACE_FILES) == 0) return &g_files;
    if (std::strcmp(name, NXP_INTERFACE_FRAME) == 0) return &g_frame;
    return nullptr;
}

int frame_on(NxpFrameFn fn, void* user) {
    g.frame = fn;
    g.frame_user = user;
    return 1;
}

bool contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

std::string find_key_with_suffix(const char* suffix) {
    for (const auto& [key, value] : g.settings) {
        (void)value;
        if (key.size() >= std::strlen(suffix) && key.compare(key.size() - std::strlen(suffix), std::strlen(suffix), suffix) == 0) {
            return key;
        }
    }
    return {};
}

bool any_log_contains(const char* needle) {
    for (const auto& line : g.logs) {
        if (contains(line, needle)) return true;
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::fprintf(stderr, "usage: nxp_host_test <plugin> [trinity|demo]\n");
        return 2;
    }
    const std::string mode = argc == 3 ? argv[2] : "trinity";
    if (mode != "trinity" && mode != "demo") {
        std::fprintf(stderr, "unknown mode: %s\n", mode.c_str());
        return 2;
    }

    ModuleHandle module = load_module(argv[1]);
    if (!module) {
        std::fprintf(stderr, "load failed: %s\n", load_error_text().c_str());
        return 1;
    }
    auto entry = reinterpret_cast<NxpGetPluginFn>(module_symbol(module, NXP_PLUGIN_ENTRY));
    if (!entry) {
        std::fprintf(stderr, "plugin does not export %s\n", NXP_PLUGIN_ENTRY);
        close_module(module);
        return 1;
    }

    const NxpPluginDesc* desc = entry(NXP_ABI_VERSION);
    if (!desc || desc->abi_version != NXP_ABI_VERSION || desc->size < NXP_PLUGIN_DESC_V1_SIZE || !desc->init ||
        !desc->shutdown) {
        std::fprintf(stderr, "plugin descriptor is not compatible\n");
        close_module(module);
        return 1;
    }

    g_ui = {sizeof(NxpUiApi), 1, &ui_panel_add, &ui_panel_visible};
    g_settings = {sizeof(NxpSettingsApi), 1, &settings_get_int, &settings_set_int,
                  &settings_get_string, &settings_set_string, &settings_flush};
    g_files = {sizeof(NxpFileApi), 1, &file_read, &file_write};
    g_frame = {sizeof(NxpFrameApi), 1, &frame_on};

    NxpHostApi host{};
    host.abi_version = NXP_ABI_VERSION;
    host.size = sizeof(host);
    host.host_id = "nxp-test-host";
    host.host_name = "NXP test host";
    host.host_version = "1.0";
    host.plugin_dir = ".";
    host.capabilities = NXP_CAP_HOST_INFO | NXP_CAP_LOG | NXP_CAP_UI_PANEL | NXP_CAP_UI_WIDGETS |
                        NXP_CAP_SETTINGS | NXP_CAP_FILES | NXP_CAP_FRAME;
    host.log = &host_log;
    host.get_interface = &host_get_interface;

    if (desc->init(&host) != NXP_OK) {
        std::fprintf(stderr, "init failed\n");
        close_module(module);
        return 1;
    }
    if (!g.panel || g.panel_adds != 1 || !g.frame) {
        std::fprintf(stderr, "discovery, panel or frame registration failed\n");
        close_module(module);
        return 1;
    }
    if (!any_log_contains("panel registered")) {
        std::fprintf(stderr, "the plugin did not report panel registration\n");
        close_module(module);
        return 1;
    }
    if (g.settings.empty()) {
        std::fprintf(stderr, "the plugin did not create any settings\n");
        close_module(module);
        return 1;
    }

    const std::string enabled_key = find_key_with_suffix(".enabled");
    if (enabled_key.empty() || g.settings[enabled_key] != "1") {
        std::fprintf(stderr, "the plugin did not persist an enabled default\n");
        close_module(module);
        return 1;
    }
    const std::string speed_key = find_key_with_suffix(".speed");

    NxpUiCanvas canvas{};
    canvas.size = sizeof(canvas);
    canvas.version = 1;
    canvas.text = &ui_text;
    canvas.text_disabled = &ui_text_disabled;
    canvas.text_colored = &ui_text_colored;
    canvas.checkbox = &ui_checkbox;
    canvas.slider_float = &ui_slider_float;
    canvas.slider_int = &ui_slider_int;
    canvas.button = &ui_button;
    canvas.same_line = &ui_same_line;
    canvas.separator = &ui_separator;
    canvas.begin_disabled = &ui_begin_disabled;
    canvas.end_disabled = &ui_end_disabled;
    canvas.tree_node = &ui_tree_node;
    canvas.tree_pop = &ui_tree_pop;
    canvas.tooltip = &ui_tooltip;

    g.text.clear();
    g.panel(&canvas, g.panel_user);
    if (g.text.empty()) {
        std::fprintf(stderr, "the panel drew no text\n");
        close_module(module);
        return 1;
    }

    g.toggle_checkbox = true;
    g.panel(&canvas, g.panel_user);
    if (g.settings[enabled_key] != "0") {
        std::fprintf(stderr, "checkbox change was not persisted (got %s)\n", g.settings[enabled_key].c_str());
        close_module(module);
        return 1;
    }

    if (!speed_key.empty()) {
        g.toggle_slider = true;
        g.panel(&canvas, g.panel_user);
        if (g.settings[speed_key] != "60") {
            std::fprintf(stderr, "slider change was not persisted (got %s)\n", g.settings[speed_key].c_str());
            close_module(module);
            return 1;
        }
    }

    g.press_reset = true;
    g.panel(&canvas, g.panel_user);
    if (g.settings[enabled_key] != "1" || (!speed_key.empty() && g.settings[speed_key] != "100")) {
        std::fprintf(stderr, "reset did not restore defaults\n");
        close_module(module);
        return 1;
    }

    g.frame(g.frame_user);
    g.text.clear();
    g.panel(&canvas, g.panel_user);
    if (!any_log_contains("Trinity") && mode == "trinity") {
        std::fprintf(stderr, "the Trinity host log does not identify the plugin\n");
        close_module(module);
        return 1;
    }

    desc->shutdown();
    if (!g.settings_flushed) {
        std::fprintf(stderr, "shutdown did not flush settings\n");
        close_module(module);
        return 1;
    }

    std::printf("PASS: %s loaded, discovered host %s, added a panel, persisted settings, ran a frame, logged, and shut down\n",
                desc->name ? desc->name : "plugin", host.host_id);
    close_module(module);
    return 0;
}

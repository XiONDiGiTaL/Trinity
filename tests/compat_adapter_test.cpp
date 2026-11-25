// SPDX-License-Identifier: GPL-3.0-or-later
// Exercises the NXMod compatibility adapter with simulated API 3, 4 and 5
// host tables. This checks the mapping, not a game runtime.

#include "nxp/nxp.h"
#include "nxrt/mod_api.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

extern "C" int nxmod_init(const NxModApi* api, const char* mod_dir);

extern "C" void probe_reset();
extern "C" const NxpHostApi* probe_host();
extern "C" const NxpSettingsApi* probe_settings();
extern "C" const NxpFileApi* probe_files();
extern "C" const NxpFrameApi* probe_frame();
extern "C" const NxpUiApi* probe_ui();
extern "C" int probe_init_count();
extern "C" int probe_panel_calls();
extern "C" int probe_frame_calls();
extern "C" void probe_frame_tick(void*);

namespace {

std::map<std::string, std::string> g_files;
NxModPanelFn g_panel_fn = nullptr;
void* g_panel_user = nullptr;
NxModFrameFn g_frame_fn = nullptr;
void* g_frame_user = nullptr;

void fake_log(const NxModApi*, int, const char*) {}

int64_t fake_read(const char* path, void* buf, uint64_t cap) {
    const auto it = g_files.find(path ? path : "");
    if (it == g_files.end()) return -1;
    if (buf && cap) {
        const size_t n = it->second.size() < cap ? it->second.size() : static_cast<size_t>(cap);
        std::memcpy(buf, it->second.data(), n);
    }
    return static_cast<int64_t>(it->second.size());
}

int fake_write(const char* path, const void* data, uint64_t size) {
    if (!path) return 0;
    g_files[path] = std::string(static_cast<const char*>(data), static_cast<size_t>(size));
    return 1;
}

void fake_on_frame(NxModFrameFn fn, void* user) {
    g_frame_fn = fn;
    g_frame_user = user;
}

int fake_add_panel(const char* title, NxModPanelFn fn, void* user) {
    if (!title || !*title || !fn) return 0;
    g_panel_fn = fn;
    g_panel_user = user;
    return 1;
}

int fake_panel_visible() { return 1; }

bool check(bool condition, const char* message) {
    if (!condition) std::printf("FAIL: %s\n", message);
    return condition;
}

bool run_version(uint32_t version) {
    g_files.clear();
    g_panel_fn = nullptr;
    g_panel_user = nullptr;
    g_frame_fn = nullptr;
    g_frame_user = nullptr;
    probe_reset();

    NxModApi api{};
    api.version = version;
    api.size = sizeof(NxModApi);
    api.game_id = "fefw";
    api.log = &fake_log;
    api.on_frame = &fake_on_frame;
    api.read_file = &fake_read;
    api.write_file = &fake_write;
    if (version >= 4) {
        api.add_panel = &fake_add_panel;
        api.panel_visible = &fake_panel_visible;
    }

    constexpr const char* kModDir = "C:/trinity-test/probe";
    if (!check(nxmod_init(&api, kModDir) == 0, "nxmod_init should succeed")) return false;
    if (!check(probe_init_count() == 1, "plugin init should run once")) return false;

    const NxpHostApi* host = probe_host();
    if (!check(host && host->abi_version == NXP_ABI_VERSION, "NXP ABI version")) return false;
    if (!check(host->host_version == std::string("nxmod-api-") + std::to_string(version), "host version should name the NXMod API")) return false;
    if (!check(probe_settings() != nullptr, "settings interface should exist on API 3 and later")) return false;
    if (!check(probe_files() != nullptr, "files interface should exist on API 3 and later")) return false;
    if (!check(probe_frame() != nullptr, "frame interface should exist when on_frame is present")) return false;

    const bool expect_ui = version >= 4;
    if (!check((probe_ui() != nullptr) == expect_ui, "UI should exist only on API 4 and later")) return false;
    if (!check(((host->capabilities & NXP_CAP_UI_PANEL) != 0) == expect_ui, "UI capability should follow the table")) return false;
    if (!check((g_panel_fn != nullptr) == expect_ui, "panel registration should follow the table")) return false;

    if (expect_ui) {
        if (!check(g_panel_fn != nullptr && g_panel_user != nullptr, "panel callback should be registered")) return false;
        NxModUi canvas{};
        canvas.size = sizeof(NxModUi);
        g_panel_fn(&canvas, g_panel_user);
        if (!check(probe_panel_calls() == 1, "panel callback should run through the trampoline")) return false;
        if (!check(probe_ui()->panel_visible() == 1, "panel_visible should pass through")) return false;
    }

    const NxpSettingsApi* settings = probe_settings();
    if (!check(settings && settings->set_int("probe.count", 42) == 1, "settings write")) return false;
    if (!check(settings->get_int("probe.count", -1) == 42, "settings read")) return false;
    char text[8]{};
    if (!check(settings->get_string("probe.count", text, sizeof(text), "") == 2 && std::strcmp(text, "42") == 0, "settings string")) return false;
    if (!check(g_files[std::string(kModDir) + "/.nxp-settings.ini"].find("probe.count=42\n") != std::string::npos, "settings file should be written")) return false;

    const NxpFileApi* files = probe_files();
    if (!check(files && files->write_file("note.txt", "hello", 5) == 1, "file write")) return false;
    char note[6]{};
    if (!check(files->read_file("note.txt", note, sizeof(note)) == 5 && std::strcmp(note, "hello") == 0, "file read")) return false;
    if (!check(files->read_file("../escape.txt", note, sizeof(note)) == -1, "file traversal should be rejected")) return false;

    const NxpFrameApi* frame = probe_frame();
    if (!check(frame && frame->on_frame(&probe_frame_tick, nullptr) == 1, "frame registration")) return false;
    if (!check(g_frame_fn != nullptr && g_frame_user != nullptr, "frame callback should be stored")) return false;
    g_frame_fn(nullptr, g_frame_user);
    if (!check(probe_frame_calls() == 1, "frame callback should run through the trampoline")) return false;

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: compat_adapter_test <3|4|5>\n");
        return 2;
    }
    const uint32_t version = static_cast<uint32_t>(std::atoi(argv[1]));
    if (version < 3 || version > 5) return 2;
    if (!run_version(version)) return 1;
    std::printf("PASS: NXMod API %u adapter mapping\n", version);
    return 0;
}

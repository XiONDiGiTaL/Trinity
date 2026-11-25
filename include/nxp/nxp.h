/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * NXP: the unified plugin ABI shared by EmblemWeave, fefw-recomp and
 * ObeliskChamber.
 *
 * A plugin exports exactly one discovery function:
 *
 *     NXP_EXPORT const NxpPluginDesc* nxp_get_plugin(uint32_t host_abi_version);
 *
 * The host calls it once, checks the descriptor's abi_version and size, then
 * calls descriptor->init(host). The host calls descriptor->shutdown() once if
 * it supports orderly unload; a plugin must also tolerate process exit without
 * shutdown.
 *
 * Every structure that crosses the boundary starts with `size` and `version`.
 * Members are appended, never reordered or removed. A host or plugin must check
 * the size of a table before reading a member added after the version it was
 * written for. Optional capabilities are discovered with host->get_interface.
 *
 * All strings are UTF-8. Host-owned strings and interface tables remain valid
 * until the plugin's shutdown (or process exit). Plugin-owned strings passed to
 * the host remain valid for the duration of the call; the host must copy any
 * string it keeps. No allocation crosses the boundary unless the interface says
 * so explicitly.
 */
#ifndef NXP_NXP_H
#define NXP_NXP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NXP_ABI_VERSION 1u

#define NXP_INTERFACE_UI "nxp.ui"
#define NXP_INTERFACE_SETTINGS "nxp.settings"
#define NXP_INTERFACE_FILES "nxp.files"
#define NXP_INTERFACE_FRAME "nxp.frame"

#define NXP_PLUGIN_ENTRY "nxp_get_plugin"

/* Result codes for plugin initialization. A host logs the code and the
 * plugin's own diagnostic message; zero is success. */
#define NXP_OK 0
#define NXP_ERR_ARGUMENT 1
#define NXP_ERR_UNSUPPORTED 2
#define NXP_ERR_INIT 3
#define NXP_ERR_HOST_ABI 4

/* Capability bits. get_interface is authoritative; these bits are a cheap
 * first look and may be a superset of what a particular interface version
 * supports. */
#define NXP_CAP_HOST_INFO (1u << 0)
#define NXP_CAP_LOG (1u << 1)
#define NXP_CAP_UI_PANEL (1u << 2)
#define NXP_CAP_UI_WIDGETS (1u << 3)
#define NXP_CAP_SETTINGS (1u << 4)
#define NXP_CAP_FILES (1u << 5)
#define NXP_CAP_FRAME (1u << 6)

typedef enum NxpLogLevel {
    NXP_LOG_ERROR = 0,
    NXP_LOG_WARN = 1,
    NXP_LOG_INFO = 2,
    NXP_LOG_DEBUG = 3
} NxpLogLevel;

typedef struct NxpHostApi NxpHostApi;
typedef struct NxpUiCanvas NxpUiCanvas;

typedef void (*NxpLogFn)(const NxpHostApi* host, int level, const char* message);
/* Names are ASCII and case-sensitive. A host returns the requested interface
 * version or a compatible later version, or NULL. The returned table starts
 * with size and version; check both before reading a member. */
typedef const void* (*NxpGetInterfaceFn)(const NxpHostApi* host, const char* name, uint32_t version);

/* The host-provided root table. `plugin_dir` is the absolute UTF-8 directory
 * the host loaded this plugin from. The plugin may use it for diagnostics, but
 * should use the files interface for relative asset access. */
struct NxpHostApi {
    uint32_t abi_version;
    uint32_t size;
    const char* host_id;       /* "emblemweave", "fefw-recomp", "obelisk-chamber", or a compatible id */
    const char* host_name;     /* human-readable */
    const char* host_version;  /* host release or host API description */
    const char* plugin_dir;
    uint32_t capabilities;
    NxpLogFn log;
    NxpGetInterfaceFn get_interface;
};

#define NXP_HOST_API_V1_SIZE (offsetof(NxpHostApi, get_interface) + sizeof(NxpGetInterfaceFn))

/* ---- nxp.ui, version 1 -------------------------------------------------- */

/* The immediate-mode canvas passed to a panel callback. The host owns it for
 * the duration of the callback. Widget labels are UTF-8; make them unique with
 * a "##suffix" that the host hides. Functions returning int return 1 when the
 * player changed the value or pressed the button this frame.
 *
 * The host controls fonts, colours, spacing, scaling, controller navigation and
 * clipping. A plugin must not assume a particular widget size or draw outside
 * the callback. */
struct NxpUiCanvas {
    uint32_t size;
    uint32_t version;
    void (*text)(const char* text);
    void (*text_disabled)(const char* text);
    void (*text_colored)(uint32_t rgba, const char* text);
    int (*checkbox)(const char* label, int* value);
    int (*slider_float)(const char* label, float* value, float min, float max, const char* format);
    int (*slider_int)(const char* label, int* value, int min, int max, const char* format);
    int (*button)(const char* label);
    void (*same_line)(void);
    void (*separator)(const char* label);
    void (*begin_disabled)(int disabled);
    void (*end_disabled)(void);
    int (*tree_node)(const char* label, int default_open);
    void (*tree_pop)(void);
    void (*tooltip)(const char* text);
};

typedef void (*NxpPanelFn)(const NxpUiCanvas* ui, void* user);

typedef struct NxpUiApi {
    uint32_t size;
    uint32_t version;
    /* Adds a section to the host's Mods/Plugins surface. Returns 1 on success.
     * The title is copied. The callback runs on the host's presenting thread,
     * not the game's main thread. The panel lives until shutdown. */
    int (*panel_add)(const char* title, NxpPanelFn fn, void* user);
    /* 1 while the host's plugin surface is on screen. Safe from any thread. */
    int (*panel_visible)(void);
} NxpUiApi;

#define NXP_UI_API_V1_SIZE (offsetof(NxpUiApi, panel_visible) + sizeof(int (*)(void)))

/* ---- nxp.settings, version 1 ------------------------------------------- */

/* Persisted key/value settings owned by the host. Keys are ASCII identifiers
 * ([A-Za-z0-9_.-]); values are UTF-8 without embedded NULs. Implementations
 * must be thread-safe. get_string returns the full value length excluding the
 * terminating NUL, or -1 on error; it truncates the output safely. set_*
 * returns 1 on success. flush returns 1 when the current values reached durable
 * storage. A host may implement this in memory and still return 1. */
typedef struct NxpSettingsApi {
    uint32_t size;
    uint32_t version;
    int (*get_int)(const char* key, int fallback);
    int (*set_int)(const char* key, int value);
    int (*get_string)(const char* key, char* out, uint32_t cap, const char* fallback);
    int (*set_string)(const char* key, const char* value);
    int (*flush)(void);
} NxpSettingsApi;

#define NXP_SETTINGS_API_V1_SIZE (offsetof(NxpSettingsApi, flush) + sizeof(int (*)(void)))

/* ---- nxp.files, version 1 ---------------------------------------------- */

/* File access scoped to the plugin's directory. Relative paths use '/' and may
 * not contain "." or ".." components. read_file returns the whole file size
 * (even when cap is smaller) or -1. write_file replaces the file and returns 1
 * on success. The host must not let a plugin escape plugin_dir through this
 * interface. */
typedef struct NxpFileApi {
    uint32_t size;
    uint32_t version;
    int64_t (*read_file)(const char* path, void* buf, uint64_t cap);
    int (*write_file)(const char* path, const void* data, uint64_t size);
} NxpFileApi;

#define NXP_FILE_API_V1_SIZE (offsetof(NxpFileApi, write_file) + sizeof(int (*)(const char*, const void*, uint64_t)))

/* ---- nxp.frame, version 1 ---------------------------------------------- */

typedef void (*NxpFrameFn)(void* user);

/* Calls fn(user) once per presented frame on the game's main thread, in
 * registration order. Used for work that must not run on the panel thread.
 * Returns 1 on success. The callback lives until shutdown. */
typedef struct NxpFrameApi {
    uint32_t size;
    uint32_t version;
    int (*on_frame)(NxpFrameFn fn, void* user);
} NxpFrameApi;

#define NXP_FRAME_API_V1_SIZE (offsetof(NxpFrameApi, on_frame) + sizeof(int (*)(NxpFrameFn, void*)))

/* ---- plugin descriptor -------------------------------------------------- */

/* Metadata is static and remains valid for the lifetime of the loaded module.
 * id should be a reverse-DNS style stable identifier. version is the plugin's
 * own semantic version; it is not the ABI version. */
typedef struct NxpPluginDesc {
    uint32_t abi_version;
    uint32_t size;
    const char* id;
    const char* name;
    const char* version;
    const char* description;
    int (*init)(const NxpHostApi* host);
    void (*shutdown)(void);
} NxpPluginDesc;

typedef const NxpPluginDesc* (*NxpGetPluginFn)(uint32_t host_abi_version);

#define NXP_PLUGIN_DESC_V1_SIZE (offsetof(NxpPluginDesc, shutdown) + sizeof(void (*)(void)))

#if defined(_WIN32)
#  ifdef __cplusplus
#    define NXP_EXPORT extern "C" __declspec(dllexport)
#  else
#    define NXP_EXPORT __declspec(dllexport)
#  endif
#elif defined(__cplusplus)
#  define NXP_EXPORT extern "C" __attribute__((visibility("default")))
#else
#  define NXP_EXPORT __attribute__((visibility("default")))
#endif

/* Convenience helpers that keep the ABI checks visible at call sites. */
static inline int nxp_host_has(const NxpHostApi* host, uint32_t capability) {
    return host && (host->capabilities & capability) != 0;
}

static inline const void* nxp_get_interface(const NxpHostApi* host, const char* name, uint32_t version) {
    if (!host || !host->get_interface || !name) return NULL;
    return host->get_interface(host, name, version);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* NXP_NXP_H */

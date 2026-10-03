#pragma once

/// Registers the tree-sitter parsers linked into this binary with libuv's
/// static uv_dlopen (AmigaOS, or a host libuv built with UV_STATIC_DL).
/// A no-op elsewhere.
void os_static_dl_init(void);

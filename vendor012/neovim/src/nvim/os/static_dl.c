// Tree-sitter parsers linked statically, for systems without a dynamic
// linker (AmigaOS with ixemul; a host libuv built with UV_STATIC_DL tests
// the same path). libuv's uv_dlopen there serves what is registered here:
// Neovim's own parser search (vim.treesitter.language.add) finds
// runtime/parser/<lang>.so -- an empty marker file -- and loads it through
// uv_dlopen/uv_dlsym as on any other system. Which parsers are linked is
// the build's choice: -DNVIM_STATIC_TS_<LANG> and -ltsparser_<lang>.

#include <tree_sitter/api.h>
#include <uv.h>

#include "nvim/os/static_dl.h"

#if defined(__amigaos__) || defined(UV_STATIC_DL)

# define TS_PARSER(lang) \
  const TSLanguage *tree_sitter_##lang(void); \
  static const uv_static_sym_t syms_##lang[] = { \
    { "tree_sitter_" #lang, __extension__(void *)tree_sitter_##lang }, { NULL, NULL } };

# ifdef NVIM_STATIC_TS_C
TS_PARSER(c)
# endif
# ifdef NVIM_STATIC_TS_LUA
TS_PARSER(lua)
# endif
# ifdef NVIM_STATIC_TS_VIM
TS_PARSER(vim)
# endif
# ifdef NVIM_STATIC_TS_VIMDOC
TS_PARSER(vimdoc)
# endif
# ifdef NVIM_STATIC_TS_QUERY
TS_PARSER(query)
# endif
# ifdef NVIM_STATIC_TS_MARKDOWN
TS_PARSER(markdown)
# endif
# ifdef NVIM_STATIC_TS_MARKDOWN_INLINE
TS_PARSER(markdown_inline)
# endif

static const uv_static_lib_t static_libs[] = {
# ifdef NVIM_STATIC_TS_C
  { "c.so", syms_c },
# endif
# ifdef NVIM_STATIC_TS_LUA
  { "lua.so", syms_lua },
# endif
# ifdef NVIM_STATIC_TS_VIM
  { "vim.so", syms_vim },
# endif
# ifdef NVIM_STATIC_TS_VIMDOC
  { "vimdoc.so", syms_vimdoc },
# endif
# ifdef NVIM_STATIC_TS_QUERY
  { "query.so", syms_query },
# endif
# ifdef NVIM_STATIC_TS_MARKDOWN
  { "markdown.so", syms_markdown },
# endif
# ifdef NVIM_STATIC_TS_MARKDOWN_INLINE
  { "markdown_inline.so", syms_markdown_inline },
# endif
  { NULL, NULL },
};

void os_static_dl_init(void)
{
  uv_static_dl_register(static_libs);
}

#else

void os_static_dl_init(void)
{
}

#endif

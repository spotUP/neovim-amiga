/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <errno.h>
#include <string.h>
#include <locale.h>

#if defined(__amigaos__) || defined(UV_STATIC_DL)
/* ixemul.library has no dynamic linker (an AmigaOS shared library is not
 * an object with a symbol table). Instead the program registers the
 * "libraries" it links statically (uv_static_dl_register, uv/nothreads.h):
 * uv_dlopen of a path whose file name is registered ("vimdoc.so") opens
 * that entry, and uv_dlsym looks the name up in its table. Any other path
 * fails with a message saying why. UV_STATIC_DL selects this on a host, to
 * test the same code. */
static const uv_static_lib_t* uv__static_libs;

void uv_static_dl_register(const uv_static_lib_t* libs) {
  uv__static_libs = libs;
}


static const char* uv__basename(const char* path) {
  const char* p;
  const char* base = path;

  for (p = path; *p != '\0'; p++)
    if (*p == '/' || *p == ':')
      base = p + 1;
  return base;
}


static int uv__dl_fail(uv_lib_t* lib, const char* what, const char* name) {
  size_t n = strlen(what) + strlen(name) + 3;

  uv__free(lib->errmsg);
  lib->errmsg = uv__malloc(n);
  if (lib->errmsg != NULL)
    snprintf(lib->errmsg, n, "%s: %s", what, name);
  return -1;
}


int uv_dlopen(const char* filename, uv_lib_t* lib) {
  const uv_static_lib_t* l;
  const char* base;

  lib->handle = NULL;
  lib->errmsg = NULL;
  if (filename == NULL)
    return uv__dl_fail(lib, "no dynamic loading on this system", "(null)");
  base = uv__basename(filename);
  for (l = uv__static_libs; l != NULL && l->file != NULL; l++) {
    if (strcmp(l->file, base) == 0) {
      lib->handle = (void*) l;
      return 0;
    }
  }
  return uv__dl_fail(lib,
                     "no dynamic loading on this system, and not linked in",
                     base);
}


void uv_dlclose(uv_lib_t* lib) {
  uv__free(lib->errmsg);
  lib->errmsg = NULL;
  lib->handle = NULL;
}


int uv_dlsym(uv_lib_t* lib, const char* name, void** ptr) {
  const uv_static_lib_t* l = lib->handle;
  const uv_static_sym_t* s;

  *ptr = NULL;
  if (l != NULL)
    for (s = l->syms; s->name != NULL; s++)
      if (strcmp(s->name, name) == 0) {
        *ptr = s->addr;
        uv__free(lib->errmsg);
        lib->errmsg = NULL;
        return 0;
      }
  return uv__dl_fail(lib, "symbol not linked in", name);
}


const char* uv_dlerror(const uv_lib_t* lib) {
  return lib->errmsg ? lib->errmsg : "no error";
}

#else
#include <dlfcn.h>

static int uv__dlerror(uv_lib_t* lib);


int uv_dlopen(const char* filename, uv_lib_t* lib) {
  dlerror(); /* Reset error status. */
  lib->errmsg = NULL;
  lib->handle = dlopen(filename, RTLD_LAZY);
  return lib->handle ? 0 : uv__dlerror(lib);
}


void uv_dlclose(uv_lib_t* lib) {
  uv__free(lib->errmsg);
  lib->errmsg = NULL;

  if (lib->handle) {
    /* Ignore errors. No good way to signal them without leaking memory. */
    dlclose(lib->handle);
    lib->handle = NULL;
  }
}


int uv_dlsym(uv_lib_t* lib, const char* name, void** ptr) {
  dlerror(); /* Reset error status. */
  *ptr = dlsym(lib->handle, name);
  return *ptr ? 0 : uv__dlerror(lib);
}


const char* uv_dlerror(const uv_lib_t* lib) {
  return lib->errmsg ? lib->errmsg : "no error";
}


static int uv__dlerror(uv_lib_t* lib) {
  const char* errmsg;

  uv__free(lib->errmsg);

  errmsg = dlerror();

  if (errmsg) {
    lib->errmsg = uv__strdup(errmsg);
    return -1;
  }
  else {
    lib->errmsg = NULL;
    return 0;
  }
}

#endif /* __amigaos__ */

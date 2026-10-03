#!/bin/sh
# Package a Neovim user config for the Amiga (ledger section U): vim-plug's
# plugins are installed HERE (git, curl) with the host Neovim 0.12.5, then
# the tree is copied to the Amiga's $HOME, where vim-plug only has to add
# them to 'runtimepath'.
#
#   tools/user-config.sh [repo-url [commit]]
#     default: https://github.com/tomviljo/dotfiles at c36d479
#   -> build/v012/userconf/home/{.config/nvim,.local/share/nvim}
#      (copy its contents into the Amiga user's $HOME)
#
# Then checks, on the host, that the config starts with no errors with
# python3 hidden (as on the Amiga: coq_nvim needs it) and writes the
# messages to build/v012/userconf/startup-messages.txt.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
REPO=${1:-https://github.com/tomviljo/dotfiles}
REV=${2:-c36d479e3de70a23f96319ba35b2c5d24b686051}
OUT=$ROOT/build/v012/userconf
NVIM=$ROOT/build/v012/host/nvim/bin/nvim
RT=$ROOT/build/v012/host/stage/usr/local/share/nvim/runtime   # source Lua (dist has m68k bytecode)
PLUG_URL=https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
[ -x "$NVIM" ] && [ -d "$RT" ] || { echo "make -f Makefile.v012 dist first"; exit 1; }

rm -rf "$OUT" && mkdir -p "$OUT"
git clone -q "$REPO" "$OUT/src"
git -C "$OUT/src" checkout -q "$REV"
H=$OUT/home
CFG=$H/.config/nvim
DATA=$H/.local/share/nvim
mkdir -p "$CFG/colors" "$DATA/site/autoload"
# coq_nvim needs CPython 3.8+ (a venv, sqlite3, asyncio, C extensions),
# which 68k AmigaOS does not have (MicroPython lacks all four): the Amiga
# copy drops its three Plug lines and uses Neovim 0.12's own completion,
# set in init-local.vim -- the config's machine-local file, sourced at the
# end of init.vim. The owner chose this (2026-10-03, "do as you
# recommend"); the friend's own init.vim elsewhere is unchanged.
grep -v "Plug 'ms-jpq/coq" "$OUT/src/nvim/init.vim" > "$CFG/init.vim"
[ -d "$OUT/src/nvim/colors" ] && cp "$OUT/src/nvim/colors/"* "$CFG/colors/"
cat > "$CFG/init-local.vim" <<'VIM'
" This Amiga (UP-Term kit): coq_nvim needs CPython 3.8 or newer, which 68k
" AmigaOS does not have, so Neovim 0.12's built-in completion is used here:
" a menu as you type, from this buffer, the other windows and buffers.
set autocomplete
set autocompletedelay=150
set complete=.,w,b,u
set completeopt=menuone,noselect,popup
VIM
curl -fsSL -o "$DATA/site/autoload/plug.vim" "$PLUG_URL"

run_nvim() {   # run_nvim <PATH> args...
	P=$1; shift
	env -i HOME="$H" PATH="$P" TERM=xterm-256color VIMRUNTIME="$RT" \
	  XDG_CONFIG_HOME="$H/.config" XDG_DATA_HOME="$H/.local/share" \
	  XDG_STATE_HOME="$H/.local/state" XDG_CACHE_HOME="$H/.cache" \
	  "$NVIM" --headless "$@"
}
# install (git on PATH)
cd "$H"
run_nvim "/usr/bin:/bin:/opt/homebrew/bin" "+PlugInstall --sync" "+qa!" > "$OUT/pluginstall.log" 2>&1 || true
find "$DATA" -name .git -prune -exec rm -rf {} +
ls "$DATA/plugged"
# start the way the Amiga will: no git, no python3, from $HOME (not inside
# a git work tree: lualine then watches nothing; see the ledger for what
# happens inside one)
mkdir -p "$OUT/bin"
CHK=$(mktemp -d "${TMPDIR:-/tmp}/nvim-userconf.XXXXXX")   # outside any work tree
cp -R "$H/." "$CHK/"
H_SAVE=$H; H=$CHK
cd "$H"
run_nvim "$OUT/bin" "+redir! > $OUT/startup-messages.txt | silent messages | redir END" "+qa!" \
  > "$OUT/startup.log" 2>&1 || true
H=$H_SAVE; rm -rf "$CHK"
echo "--- startup messages (empty = clean):"
cat "$OUT/startup-messages.txt"; cat "$OUT/startup.log"
du -sh "$CFG" "$DATA"

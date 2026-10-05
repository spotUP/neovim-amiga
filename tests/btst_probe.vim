" Probe for the code bebbo's gcc 6.5 cc1 miscompiled before 2026-10-05
" (one-bit tests of an int in memory at bit >= 8: `btst #-16,(a0)` tested
" bit 24 instead of bit 8). Ledger section W. Each line of the output is
" "ok"/"FAIL" + the check; a binary built with the faulty cc1 fails them.
"   nvim --headless -u NONE -i NONE -S tests/btst_probe.vim
" Writes $BTST_OUT (default btst_probe.out in the current directory).
let s:out = []
function! s:check(name, got, want) abort
  call add(s:out, (a:got ==# a:want ? 'ok   ' : 'FAIL ') . a:name
        \ . (a:got ==# a:want ? '' : ' (got ' . string(a:got) . ', want ' . string(a:want) . ')'))
endfunction

" ex_eval.c ex_endtry: the enclosing :try is found below an unclosed :if
" (cs_flags[idx] & CSF_TRY). Faulty: the search runs to level 0, the outer
" :if is unwound too and the closing :endif gives E580.
let s:body = tempname()
call writefile(['if 1', '  try', '    if 1', '  endtry', 'endif'], s:body)
let v:errmsg = ''
silent! execute 'source' fnameescape(s:body)
call delete(s:body)
" the last error: E171 at the :endtry, nothing after it
call s:check('ex_endtry: E171 is the last error (no E580 at the outer :endif)',
      \ matchstr(v:errmsg, 'E\d\+'), 'E171')

if has('nvim-0.10')
  " option.c get_option_default: `:setlocal all&` gives a global-local
  " option back its unset local value (opt_flags & OPT_LOCAL). Faulty: the
  " default is copied into the local value.
  setlocal makeprg=probe-local
  setlocal all&
  call s:check('setlocal all& unsets the local makeprg', &l:makeprg, '')

  " window.c win_init: the quickfix window gets no copy of the current
  " window's location list (flags & WSP_NEWLOC). Faulty: it gets one.
  call setloclist(0, [{'filename': 'probe', 'lnum': 1, 'text': 'x'}])
  copen
  call s:check(':copen window has no location list', getloclist(0, {'id': 0}).id, 0)
  cclose
endif

call writefile(s:out, empty($BTST_OUT) ? 'btst_probe.out' : $BTST_OUT)
qall!

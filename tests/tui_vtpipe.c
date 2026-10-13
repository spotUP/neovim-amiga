/* tui_vtpipe COLS ROWS PERSONALITY(amiga|xterm) DUMPFILE
 * The UP-Term engine (vtcon/engine) as the terminal behind a pty, for
 * tests/tui_eager_wrap.py. stdin: the program's output; stdout: the engine's
 * answers (DA, DSR, ...), so nvim gets what UP-Term would send. At EOF the
 * screen goes to DUMPFILE: ROWS lines of text, then ROWS lines of the cells'
 * background values in hex. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vtengine.h"

static void on_reply(void *user, const vt_u8 *b, long n)
{
    (void)user;
    if (write(1, b, (size_t)n) < 0)
        exit(3);
}

int main(int argc, char **argv)
{
    static vt_u8 buf[65536];
    vt_callbacks cb;
    vt_term *t;
    long n;
    int y, x, cols, rows;
    FILE *d;
    if (argc < 5)
        return 2;
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);
    memset(&cb, 0, sizeof cb);
    cb.reply = on_reply;
    t = vt_new(cols, rows, 0, &cb, 0);
    if (!t)
        return 1;
    if (!strcmp(argv[3], "amiga"))
        vt_set_personality(t, VT_AMIGA);
    while ((n = read(0, buf, sizeof buf)) > 0)
        vt_write(t, buf, n);
    d = fopen(argv[4], "w");
    if (!d)
        return 1;
    for (y = 0; y < rows; y++) {
        int nc;
        const vt_cell *c = vt_row(t, y, &nc);
        char line[8192];
        int len = 0;
        for (x = 0; x < nc; x++)
            if (c[x].width)
                len += vt_cell_utf8(t, &c[x], line + len);
        line[len] = 0;
        fprintf(d, "%s\n", line);
    }
    for (y = 0; y < rows; y++) {
        int nc;
        const vt_cell *c = vt_row(t, y, &nc);
        for (x = 0; x < nc; x++)
            fprintf(d, "%s%lx", x ? " " : "", (unsigned long)c[x].bg);
        fprintf(d, "\n");
    }
    fclose(d);
    return 0;
}

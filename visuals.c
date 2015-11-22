#include <ncurses.h>
#include <stdlib.h>

#include "visuals.h"

// unicode bytes for eighth bars.
static const char *outc[8] = {"\xe2\x96\x81", "\xe2\x96\x82",
                              "\xe2\x96\x83", "\xe2\x96\x84",
                              "\xe2\x96\x85", "\xe2\x96\x86",
                              "\xe2\x96\x87", "\xe2\x96\x88"};

struct {
    int limit;
    int dynamic;
    int fgcolor;
    int bgcolor;
    double *old_data;
    int datasize;
    FILE *logfile;
} info;

/* Draws a bar on the ncurses screen. Index is the bar number.
 * top and bottom range from 0 to 1. These assume a lowerleft origin,
 * but output is in an upperleft origin.
 */
void draw_bar(int width, int index, int row, double bottom, double top) {
    double bottom_edge = (row) - (bottom * (row));
    double top_edge = (row) - (top * (row));
    int bottom_i = (int) bottom_edge;
    int top_i = (int) top_edge;
    int start_col = width * index;

    // Special edge case.
    if (bottom_i > 79)
        bottom_i = 79;

    // top edge case.
    if (top_i < 0)
        top_i = 0;

    // Origin is in upperleft
    for (int i = bottom_i; i >= top_i; i--) {
        move(i, start_col);
        for (int j = 0; j < width; j++) {
            printw(outc[7]);
        }
    }

    // Draw fractional section of the bar
    move(top_i, start_col);

    // Loosely similar to an fmod by .125 to find eighth sectional
    int eighth_index = (int)((top_edge - top_i) * 1000) / 125;
    if (eighth_index != 0) {
        for (int j = 0; j < width; j++) {
            // backwards because origin is at top
            printw(outc[7 - eighth_index]);
        }
    }
    fprintf(info.logfile, "%f -> %d, i%d\n", top_edge, top_i, eighth_index);
}

/* Expects a normalized array of doubles.
 */
int ncviz_draw_data(double *new_data, int size)
{
    static int prev_row, prev_col;

    // Check for dataset resize or window resize
    int row, col;
    getmaxyx(stdscr, row, col);
    if (size != info.datasize || prev_row != row || prev_col != col) {
        fprintf(info.logfile, "Data Reset.\n");
        free(info.old_data);
        info.old_data = calloc(size, sizeof(double));
        clear();
    }



    if (size > col) {
        fprintf(info.logfile, "Cannot fit data on screen.\n");
        return -1;
    }

    int bar_width = col / size;
    fprintf(info.logfile, "r: %d, c: %d\n", row, col);
    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i];
        double old_datum = info.old_data[i];
        if (new_datum < 0 || new_datum > 1 || old_datum < 0 || old_datum > 1) {
            fprintf(info.logfile, "Received non-normalized data.\n");
            return -1;
        }

        // May the floating point gods have mercy on this equality check.
        if (new_datum == old_datum) {
            continue;
        } else if (new_datum > old_datum) {
            draw_bar(bar_width, i, row, old_datum, new_datum);
        } else {
            // attron(COLOR_PAIR(2))
            draw_bar(bar_width, i, row, new_datum, old_datum);
            // attron(COLOR_PAIR(1))
        }
    }

    // Save this new data.
    info.datasize = size;
    prev_row = row;
    prev_col = col;
    for (int i = 0; i < size; i++)
        info.old_data[i] = new_data[i];

    refresh();
    return 0;
}

void ncviz_end()
{
    endwin();
    free(info.old_data);
    fprintf(info.logfile, "Closing log.");
    fclose(info.logfile);
}

int init_ncurse() {
    initscr();
    curs_set(0);
    timeout(0);
    keypad(stdscr, TRUE);
    noecho();

    if (has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "Your terminal doesn't support colors\n");
        return -1;
    }

    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    init_pair(2, COLOR_BLACK, COLOR_BLACK);

    return 0;
}

int init_data() {
    info.limit = 100;
    info.dynamic = 1;
    info.fgcolor = COLOR_RED;
    info.bgcolor = COLOR_BLACK;

    info.old_data = NULL;
    info.datasize = 0;

    info.logfile = fopen("ncviz.log", "w");
    return 0;
}

/* Initialize ncurses screen */
int ncviz_init()
{
    if (init_ncurse() != 0)
        return -1;
    if (init_data() != 0)
        return -1;
    return 0;
}

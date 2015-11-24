#include <ncurses.h>
#include <stdlib.h>

#include "visuals.h"

// unicode bytes for eighth bars.
static const char *outc[8] = {"\xe2\x96\x81", "\xe2\x96\x82",
                              "\xe2\x96\x83", "\xe2\x96\x84",
                              "\xe2\x96\x85", "\xe2\x96\x86",
                              "\xe2\x96\x87", "\xe2\x96\x88"};

struct prev_data {
    double *data;
    int datasize;
    int row;
    int col;
};

/**
 * Options for how to display bars on the screen.
 *
 * dynamic_limit: Determines if the limit will increase if the
 *                current data exceeds the current limit
 * limit: max value for data. Values larger than this will take up one column
 *        or, if dynamic_limit=1 generate a new value.
 * width: the amount of characters each bar takes.
 * align: determines where the whitespace for a bar graph will be.
 */
struct {
    int limit;
    int dynamic_limit;
    int width;
    enum alignment align;
    int fgcolor;
    int bgcolor;
    struct prev_data *prev;
    FILE *logfile;
} option;

/**
 * Resets the prev_data set if the window is resized or datasize changes.
 */
void check_datareset(int row, int col, int datasize) {
    if (datasize != option.prev->datasize ||
            row != option.prev->row ||
            col != option.prev->col) {
        fprintf(option.logfile, "Data Reset.\n");
        free(option.prev->data);
        option.prev->data = calloc(datasize, sizeof(double));
        clear();
    }
}

/**
 * Draws a bar on the ncurses screen.
 * top and bottom range from 0 to 1. These assume a lowerleft origin,
 * but output is in an upperleft origin.
 */
void draw_bar(int width, int start_col, int row, double bottom, double top) {
    double bottom_edge = (row) - (bottom * (row));
    double top_edge = (row) - (top * (row));
    int bottom_i = (int) bottom_edge;
    int top_i = (int) top_edge;

    // Special edge case.
    if (bottom_i >= row)
        bottom_i = row - 1;

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
}

void save_data(double *data, int size, int row, int col) {
    option.prev->datasize = size;
    option.prev->row = row;
    option.prev->col = col;
    for (int i = 0; i < size; i++)
        option.prev->data[i] = data[i];
}

void update_colorpair() {
    init_pair(2, option.bgcolor, option.bgcolor);
    bkgd(COLOR_PAIR(2));
    init_pair(1, option.fgcolor, option.bgcolor);
    attron(COLOR_PAIR(1));
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

    return 0;
}

/**
 * Initializes internal datastructures.
 */
int init_option() {
    option.limit = 100;
    option.dynamic_limit = 0;
    option.width = 0;
    option.fgcolor = COLOR_RED;
    option.bgcolor = COLOR_BLACK;
    update_colorpair();

    option.prev = malloc(sizeof(*option.prev));
    option.prev->data = NULL;
    option.prev->datasize = 0;

    option.logfile = fopen("ncviz.log", "w");
    return 0;
}

/* Initialize ncurses screen */
int ncviz_init()
{
    if (init_ncurse() != 0)
        return -1;
    if (init_option() != 0)
        return -1;
    return 0;
}

void ncviz_end()
{
    endwin();
    free(option.prev->data);

    fprintf(option.logfile, "Closing log.");
    fclose(option.logfile);
}

/**
 * Expects a normalized array of doubles. Uses option.prev to prevent
 * redrawing of the entire dataset.
 */
int ncviz_draw_data_static(double *new_data, int size)
{
    int row, col;
    getmaxyx(stdscr, row, col);

    check_datareset(row, col, size);

    if (size > col) {
        fprintf(option.logfile, "Cannot fit data on screen.\n");
        return -1;
    }

    int bar_width;
    if (option.width == 0)
        bar_width = col / size;
    else
        bar_width = option.width;

    fprintf(option.logfile, "r: %d, c: %d\n", row, col);
    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i];
        double old_datum = option.prev->data[i];
        if (new_datum < 0 || new_datum > 1 || old_datum < 0 || old_datum > 1) {
            fprintf(option.logfile, "Received non-normalized data.\n");
            return -1;
        }

        // May the floating point gods have mercy on this equality check.
        if (new_datum == old_datum) {
            continue;
        } else if (new_datum > old_datum) {
            draw_bar(bar_width, i * bar_width, row, old_datum, new_datum);
        } else {
            attron(COLOR_PAIR(2));
            draw_bar(bar_width, i * bar_width, row, new_datum, old_datum);
            attron(COLOR_PAIR(1));
        }
    }

    // Save this new data.
    save_data(new_data, size, row, col);

    refresh();
    return 0;
}


/*************************************************
 * Option setting code.
 *************************************************/
int ncviz_width(int width)
{
    if (width < 0)
        return -1;
    option.width = width;
    return 0;
}

void ncviz_limit(int lim) { option.limit = lim; }
void ncviz_dynamic(int dynamic) {option.dynamic_limit = dynamic; }
void ncviz_align(enum alignment align) {option.align = align; }
void ncviz_fgcolor(int color)
{
    option.fgcolor = color;
    update_colorpair();
}

void ncviz_bgcolor(int color)
{
    option.bgcolor = color;
    update_colorpair();
}

void ncviz_color(int fgcolor, int bgcolor)
{
    option.fgcolor = fgcolor;
    option.bgcolor = bgcolor;
    update_colorpair();
}

int ncviz_set_option(struct ncviz_option *options)
{
    if (options->width < 0)
        return -1;

    option.width = options->width;
    option.limit = options->limit;
    option.dynamic_limit = options->dynamic_limit;
    option.align = options->align;
    option.fgcolor = options->fgcolor;
    option.bgcolor = options->bgcolor;
    update_colorpair();

    return 0;
}

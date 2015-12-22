#include <ncurses.h>
#include <stdlib.h>
#include <locale.h>

#include "ncviz.h"


// unicode bytes for eighth bars. Starts at one eighth.
static const char *outc[8] = {"\xe2\x96\x81", "\xe2\x96\x82",
                              "\xe2\x96\x83", "\xe2\x96\x84",
                              "\xe2\x96\x85", "\xe2\x96\x86",
                              "\xe2\x96\x87", "\xe2\x96\x88"};

/* Stores information of previous data point. Used to prevent redraw of certain
 * parts of the screen.
 */
struct prev_data {
    double *data;
    int datasize;
    int row;
    int col;
};

/* Options for how to display bars on the screen.
 *
 * is_dynamic_limit: Determines if the limit will increase if the
 *                current data exceeds the current limit
 * limit: max value for data. Values larger than this will take up one column
 *        or, if is_dynamic_limit=1 generate a new value.
 * width: the amount of characters each bar takes. 0 fits screen.
 * alignment: determines where the whitespace for a bar graph will be.
 *
 */
struct option {
    int limit;
    int is_dynamic_limit;
    int width;
    enum alignment alignment;
    int fgcolor;
    int bgcolor;
    struct prev_data *prev;
    FILE *logfile;
} option;

void print_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(option.logfile, format, args);

    va_end(args);
}

void update_colorpair() {
    init_pair(2, option.bgcolor, option.bgcolor);
    bkgd(COLOR_PAIR(2));
    init_pair(1, option.fgcolor, option.bgcolor);
    attron(COLOR_PAIR(1));
}

int init_ncurse()
{
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

/* Initializes internal datastructure. */
int init_option() {
    option.limit = 100;
    option.is_dynamic_limit = 0;
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


/* Initialize ncurses screen. */
int ncviz_init()
{
    setlocale(LC_ALL, "");
    if (init_ncurse() != 0)
        return -1;
    if (init_option() != 0)
        return -1;
    return 0;
}

/* Ends ncurses screen and frees internal datastructure. */
void ncviz_end()
{
    endwin();
    free(option.prev->data);

    print_log("Closing log.\n");
    fclose(option.logfile);
}

/* Resets the prev_data set if the window is resized or datasize changes.
 */
void check_datareset(int row, int col, int datasize) {
    if (datasize != option.prev->datasize ||
            row != option.prev->row ||
            col != option.prev->col) {
        print_log("Data Reset.\n");
        free(option.prev->data);
        option.prev->data = calloc(datasize, sizeof(double));
        clear();
    }
}

/* Draws a bar on the ncurses screen.
 * Input expects origin in bottomleft.
 * Ncurses expects coordinates with upperleft origin.
 */
void draw_bar(int width, int start_col, int row, int bottom, int top) {
    // Draw bars. Origin is in upperleft
    for (int i = bottom; i >= top; i--) {
        move(i, start_col);
        for (int j = 0; j < width; j++) {
            printw(outc[7]);
        }
    }
}

/* Draws an eighth sectinoal on the ncurses screen. */
void draw_eighth(int width, int start_col, double top_edge, int top_i) {
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

/* Updates previous data point.
 */
void save_data(double *data, int size, int row, int col) {
    option.prev->datasize = size;
    option.prev->row = row;
    option.prev->col = col;
    for (int i = 0; i < size; i++)
        option.prev->data[i] = data[i];
}

/* Expects an array of doubles in the range 0-1.
 */
int ncviz_draw_data_normalized(double *new_data, int size)
{
    int row, col;
    getmaxyx(stdscr, row, col);
    check_datareset(row, col, size);

    int bar_width;
    if (option.width == 0)
        bar_width = col / size;
    else
        bar_width = option.width;

    if (bar_width * size > col) {
        print_log("Cannot fit data on screen.\n");
        return -1;
    }

    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i];
        double old_datum = option.prev->data[i];
        if (new_datum < 0 || new_datum > 1 || old_datum < 0 || old_datum > 1) {
            print_log("Received non-normalized data.\n");
            return -1;
        }

        double bottom_edge = row - (old_datum * row);
        double top_edge = row - (new_datum * row);
        int bottom_i = (int) bottom_edge;
        int top_i = (int) top_edge;

        // Special edge cases.
        if (bottom_i >= row) bottom_i = row - 1;
        if (top_i < 0) top_i = 0;

        // May the floating point gods have mercy on this equality check.
        if (new_datum == old_datum) {
            continue;
        } else if (new_datum > old_datum) {
            draw_bar(bar_width, i * bar_width, row, bottom_i, top_i);
            draw_eighth(bar_width, i * bar_width, top_edge, top_i);
        } else {
            // Switch to background color to erase bars.
            attron(COLOR_PAIR(2));
            draw_bar(bar_width, i * bar_width, row, bottom_i, top_i - 1);
            attron(COLOR_PAIR(1));
        }
    }

    // Save this new data.
    save_data(new_data, size, row, col);

    refresh();
    return 0;
}


/*************************************************
 * Option setting functions.
 *************************************************/

/* Sets the width of each bar displayed
 */
int ncviz_width(int width)
{
    if (width < 0)
        return -1;
    option.width = width;
    return 0;
}

/* Specifies whether the data is dynamic, ignores the value of limit otherwise
 */
void ncviz_dynamic(int dynamic, int limit) {
    option.is_dynamic_limit = dynamic;
    if (!dynamic) {
        option.limit = limit;
    }
}
void ncviz_align(enum alignment align) {option.alignment = align; }
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
    struct ncviz_option old_options = {
        .limit = option.limit,
        .is_dynamic_limit = option.is_dynamic_limit,
        .width = option.width,
        .alignment = option.alignment,
        .fgcolor = option.fgcolor,
        .bgcolor = option.bgcolor,
    };

    if (ncviz_width(options->width) == -1) goto error;
    ncviz_dynamic(options->is_dynamic_limit, options->limit);
    ncviz_align(options->alignment);
    ncviz_color(options->fgcolor, options->bgcolor);

    // guarantee reset of currently drawn data.
    check_datareset(-1, -1, -1);

    return 0;
error:
    // Revert changes on failure.
    option.limit = old_options.limit;
    option.is_dynamic_limit = old_options.is_dynamic_limit;
    option.width = old_options.width;
    option.alignment = old_options.alignment;
    option.fgcolor = old_options.fgcolor;
    option.bgcolor = old_options.bgcolor;

    return -1;
}

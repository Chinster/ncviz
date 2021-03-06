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
    double limit;
    int is_dynamic_limit;
    int width;
    enum alignment alignment;
    int fgcolor;
    int bgcolor;
    struct prev_data *prev;
    FILE *logfile;
    int debug;
} option;

void print_log(const char *format, ...)
{
    if (option.debug) {
        va_list args;
        va_start(args, format);

        vfprintf(option.logfile, format, args);

        va_end(args);
    }
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
    option.limit = 1;
    option.is_dynamic_limit = 0;
    option.width = 0;
    option.fgcolor = COLOR_RED;
    option.bgcolor = COLOR_BLACK;
    update_colorpair();

    option.prev = malloc(sizeof(*option.prev));
    option.prev->data = NULL;
    option.prev->datasize = 0;

    option.logfile = NULL;
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
    if (option.logfile);
        fclose(option.logfile);
}

/* Updates previous data point.
 */
void save_data(double *data, int row, int col, int size) {
    option.prev->datasize = size;
    option.prev->row = row;
    option.prev->col = col;
    for (int i = 0; i < size; i++)
        option.prev->data[i] = data[i];
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
        option.prev->datasize = datasize;
        option.prev->row = row;
        option.prev->col = col;

        clear();
    }
}

/* Resets the prev_data set. */
void datareset(void) {
    check_datareset(-1, -1, -1);
}

/* Draws a bar on the ncurses screen.
 * Bottom is lowest row to fill, top is highest.
 * Origin is upperleft so bottom is larger than top.
 */
void draw_bar(int width, int start_col, int row, int bottom, int top)
{
    // Special edge cases.
    if (bottom >= row) bottom = row - 1;
    if (top < 0) top = 0;

    // Draw bars. Origin is in upperleft
    for (int i = bottom; i >= top; i--) {
        move(i, start_col);
        for (int j = 0; j < width; j++) {
            printw(outc[7]);
        }
    }
}

/* Draws an eighth sectinoal on the ncurses screen. */
void draw_eighth(int width, int start_col, double top_edge, int top_i)
{
    // Ignore out of bounds.
    if (top_edge < 0) return;

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

/* Linear search for max value. */
static double find_max(double *arr, int size)
{
    double max = arr[0];
    for (int i = 1; i < size; i++)
        if (arr[i] > max)
            max = arr[i];
    return max;
}

/* Returns a bar width given width of ncurses screen, and data size */
static int get_bar_width(int col, int size)
{
    if (size == 0)
        return 1;
    int bar_width;
    if (option.width == 0) {
        bar_width = col / size;
        if (bar_width == 0)
            bar_width = 1;
    } else {
        bar_width = option.width;
    }
    return bar_width;
}

/* Draws a singular data point. */
static void draw_datum(double new, double old, int i, int bar_width, int row)
{
    if (new > old) {
        double bottom = row - (old * row);
        double top = row - (new * row);

        draw_bar(bar_width, i * bar_width, row, (int)bottom, (int)top);
        draw_eighth(bar_width, i * bar_width, top, (int)top);
    } else if (new < old) {
        double bottom = row - (new * row);
        double top = row - (old * row);

        // Switch to background color to erase bars.
        attron(COLOR_PAIR(2));
        draw_bar(bar_width, i * bar_width, row, (int)bottom - 1, (int)top - 1);
        attron(COLOR_PAIR(1));
        draw_eighth(bar_width, i * bar_width, bottom, (int)bottom);
    }
}


/* Draws the data where values are within the range of 0 and option.limit.
 * If is_dynamic_limit is set draws the graph with max value being max of
 * dataset. If using a limit of 1.0 use ncviz_draw_data_normalized.
 *
 * Returns 0 on success. -1 when data is too wide. -2 when given non-normalized
 * data.
 */
int ncviz_draw_data(double *new_data, int size)
{
    if (size == 0)
        return -2;

    int err = 0;
    int row, col;
    getmaxyx(stdscr, row, col);
    check_datareset(row, col, size);
    int bar_width = get_bar_width(col, size);

    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i] / option.limit;
        double old_datum = option.prev->data[i] / option.limit;

        if (new_datum > 1 || new_datum < 0) {
            if (option.is_dynamic_limit) {
                option.limit = find_max(new_data, size);
                print_log("Limit reached. New limit %f\n", option.limit);
                datareset();
                return ncviz_draw_data(new_data, size);
            }
        } else {
            err = -2;
        }

        if (i * bar_width + bar_width > col) {
            print_log("Datum at index %d will not fit widthwise.\n");
            err = -1;
            break;
        }

        draw_datum(new_datum, old_datum, i, bar_width, row);
    }

    // Save this new data.
    save_data(new_data, row, col, size);
    refresh();
    return err;
}

/* Data points represent number of characters to use.
 * Returns 0 on success. -1 when data is too wide. -2 when too tall.
 */
int ncviz_draw_data_static(double *new_data, int size)
{
    int err = 0;
    int row, col;
    getmaxyx(stdscr, row, col);
    check_datareset(row, col, size);
    int bar_width = get_bar_width(col, size);

    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i];
        double old_datum = option.prev->data[i];

        if (new_datum > row || new_datum < 0)
            err = -2;
        if (i * bar_width + bar_width > col) {
            print_log("Datum at index %d will not fit widthwise.\n");
            err = -1;
            break;
        }

        // Draw datum function below with modified bottom/top variables
        if (new_datum == old_datum) {
            continue;
        } else if (new_datum > old_datum) {
            double bottom = row - old_datum;
            double top = row - new_datum;

            draw_bar(bar_width, i * bar_width, row, (int)bottom, (int)top);
            draw_eighth(bar_width, i * bar_width, top, (int)top);
        } else {
            double bottom = row - new_datum;
            double top = row - old_datum;

            // Switch to background color to erase bars.
            attron(COLOR_PAIR(2));
            draw_bar(bar_width, i * bar_width, row, (int)bottom - 1, (int)top - 1);
            attron(COLOR_PAIR(1));
            draw_eighth(bar_width, i * bar_width, bottom, (int)bottom);
        }
    }

    // Save this new data.
    save_data(new_data, row, col, size);
    refresh();
    return err;
}

/* Expects normalized data points.
 * Returns 0 on success. -1 when data is too wide. -2 when too tall.
 */
int ncviz_draw_data_normalized(double *new_data, int size)
{
    int err = 0;
    int row, col;
    getmaxyx(stdscr, row, col);
    check_datareset(row, col, size);
    int bar_width = get_bar_width(col, size);

    for (int i = 0; i < size; i++) {
        double new_datum = new_data[i];
        double old_datum = option.prev->data[i];

        if (new_datum > row || new_datum < 0)
            err = -2;
        if (i * bar_width + bar_width > col) {
            print_log("Datum at index %d will not fit widthwise.\n");
            err = -1;
            break;
        }

        draw_datum(new_datum, old_datum, i, bar_width, row);
    }

    // Save this new data.
    save_data(new_data, row, col, size);
    refresh();
    return err;
}

/*************************************************
 * Option setting functions.
 *************************************************/

/* Sets the width of each bar displayed. If width == 0, bars will attempt to
 * fit on screen.
 */
int ncviz_width(int width)
{
    if (width < 0)
        return -1;
    option.width = width;

    datareset();
    return 0;
}

/* Specifies whether the data is dynamic, ignores the value of limit otherwise.
 * Dynamic data resizes entire graph when a value larger than the limit is
 * found.
 *
 * limit will be ignored if less than 0.
 */
void ncviz_dynamic(int dynamic, double limit) {
    option.is_dynamic_limit = dynamic;
    if (!dynamic && limit > 0) {
        option.limit = limit;
    }

    datareset();
}

void ncviz_align(enum alignment align) {
    option.alignment = align;

    datareset();
}
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

// Set all options at once.
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

    return 0;
error:
    print_log("Option error\n");
    // Revert changes on failure.
    option.limit = old_options.limit;
    option.is_dynamic_limit = old_options.is_dynamic_limit;
    option.width = old_options.width;
    option.alignment = old_options.alignment;
    option.fgcolor = old_options.fgcolor;
    option.bgcolor = old_options.bgcolor;

    return -1;
}

void ncviz_debug(int mode, char *logfile)
{
    option.debug = mode;
    if (option.debug) {
        if (option.logfile) {
            fclose(option.logfile);
            option.logfile = NULL;
        }
        option.logfile = fopen(logfile, "w");
    } else {
        if (option.logfile) {
            fclose(option.logfile);
            option.logfile = NULL;
        }
    }
}

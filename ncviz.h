#ifndef NCVIZ_VISUALS_H
#define NCVIZ_VISUALS_H

// ncurses color defines
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

enum alignment { ALIGN_LEFT, ALIGN_MIDDLE, ALIGN_RIGHT };

/* Options for how to display bars on the screen.
 *
 * is_dynamic_limit: Determines if the limit will increase if the
 *                current data exceeds the current limit
 * limit: max value for data. Values larger than this will take up entire
 * column or, if is_dynamic_limit=1 generate a new value.
 * width: the amount of characters each bar takes.
 * align: determines where the whitespace for a bar graph will be.
 */
struct ncviz_option {
    int limit;
    int is_dynamic_limit;
    int width;
    enum alignment alignment;
    int fgcolor;
    int bgcolor;
};

int ncviz_init();
void ncviz_end();
int ncviz_draw_data(double *, int);
int ncviz_draw_data_static(double *, int);
int ncviz_draw_data_static_normalized(double *, int);


int ncviz_width(int);
void ncviz_dynamic(int, double);
void ncviz_align(enum alignment);
void ncviz_fgcolor(int);
void ncviz_bgcolor(int);
void ncviz_color(int, int);
int ncviz_normalized(int);
int ncviz_set_option(struct ncviz_option *);

#endif

#ifndef NCVIZ_VISUALS_H
#define NCVIZ_VISUALS_H

enum alignment { LEFT, MIDDLE, RIGHT };

struct ncviz_option {
    int limit;
    int dynamic_limit;
    int width;
    enum alignment align;
    int fgcolor;
    int bgcolor;
};

int ncviz_init();
void ncviz_end();
int ncviz_draw_data_static(double *, int);

/* ncurses color defines
 * COLOR_BLACK   O
 * COLOR_RED     1
 * COLOR_GREEN   2
 * COLOR_YELLOW  3
 * COLOR_BLUE    4
 * COLOR_MAGENTA 5
 * COLOR_CYAN    6
 * COLOR_WHITE   7
 */

int ncviz_width(int);
void ncviz_limit(int);
void ncviz_dynamic(int);
void ncviz_align(enum alignment);
void ncviz_fgcolor(int);
void ncviz_bgcolor(int);
void ncviz_color(int, int);
int ncviz_set_option(struct ncviz_option *);

#endif

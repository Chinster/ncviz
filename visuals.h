#ifdef PPVIZ_VISUALS_H
#define PPVIZ_VISUALS_H

int init_ncurses(int, int);
int draw_pkts_ncurses(double *, double *, int);

void end_ncurses();
#endif

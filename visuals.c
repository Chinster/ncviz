#include <ncurses.h>
#include <wchar.h>

#include "visuals.h"

/* Initialize ncurses screen */
int init_ncurses(int color, int bgcolor)
{
    initscr();
    curs_set(0);
    timeout(0);
    keypad(stdscr, TRUE);
    noecho();

    if (has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "Your terminal doesn't support colors\n");
        return 1;
    }

    start_color();
    init_pair(1, color, bgcolor);
    attron(COLOR_PAIR(1));

    return 0;
}

/* Expects a normalized array of doubles. old_data should represent the
 * output currently on the screen, though an array initialized to zero
 * will redraw all necessary output.
 */
int draw_pkts_ncurses(double *old_data, double *new_data, int size)
{
   static const wchar_t* outc[] = {L"\u2581", L"\u2582", L"\u2583", L"\u2584",
                                   L"\u2585", L"\u2586", L"\u2587", L"\u2588"};

   /*
   for (int i = 0; i < size; i++) {
       if (new_data[i] > old_data[i]) {

       }
   }
   */

   return 0;
}

void end_ncurses()
{
    endwin();
}

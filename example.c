/**
 * Some usage examples for the ncviz library.
 */
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "ncviz.h"

#define DATA_SIZE 50

double *random_array(int size)
{
    double *arr = malloc(sizeof(double) * size);

    for (int i = 0; i < size; i++) {
        arr[i] = (double) rand() / (double) RAND_MAX;
    }

    return arr;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    struct timespec delay = { .tv_sec = 0, .tv_nsec = 20000000 };

    double *data = random_array(DATA_SIZE);
    struct ncviz_option opts = {
        .limit = 1.0,
        .width = 3,
        .is_dynamic_limit = 0,
        .alignment = ALIGN_LEFT,
        .fgcolor = COLOR_YELLOW,
        .bgcolor = COLOR_RED,
    };
    ncviz_init();
    ncviz_set_option(&opts);

    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < DATA_SIZE; j++) {
            data[j] += 0.005;
            if (data[j] >= 1.0) data[j] = 1.0;
        }
        ncviz_draw_data_static_normalized(data, DATA_SIZE);
        nanosleep(&delay, NULL);
    }
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < DATA_SIZE; j++) {
            data[j] -= .005;
            if (data[j] <= 0.0) data[j] = 0.0;
        }
        ncviz_draw_data_static_normalized(data, DATA_SIZE);
        nanosleep(&delay, NULL);
    }


    ncviz_end();
    free(data);
}

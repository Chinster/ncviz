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
    double *data = random_array(DATA_SIZE);
    struct ncviz_option opts = {
        .limit = 100,
        .is_dynamic_limit = 0,
        .width = 2,
        .alignment = ALIGN_LEFT,
        .fgcolor = COLOR_YELLOW,
        .bgcolor = COLOR_RED,
    };
    ncviz_init();
    ncviz_set_option(&opts);


    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < DATA_SIZE; j++) {
            data[j] -= 0.01;
            if (data[j] <= 0.0) data[j] = 0.0;
        }
        ncviz_draw_data_normalized(data, 10);
        sleep(1);
    }


    ncviz_end();
    free(data);
}

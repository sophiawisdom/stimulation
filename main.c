#include "stdbool.h"
#include <stdlib.h>
#include <stdio.h>

typedef enum PolicyResult {
    Right, // go right, even if it means waiting
    Top, // go top, even if it means waiting.
} PolicyResult;

struct simul;

typedef PolicyResult (*PolicyFunc)(struct simul * test);

struct simul {
    int block_height;
    int block_width;
    int blocks_high;
    int blocks_wide;
    int street_width;

    int **times; // times[3][6] is block (x=3, y=6)

    int current_x; // start at 0, 0
    bool x_right; // true: we're at the right of the block. false: we're at the left of the block
    int current_y;
    bool y_top; // true: we're at the top of the block. false: we're at the bottom of the block

    int cur_t;

    int stoplight_time;

    PolicyFunc policy;
};

int stoplight_wait(struct simul *simulation, PolicyResult direction) {
    int effective_x = simulation -> current_x - !simulation->x_right + 1;
    int effective_y = simulation -> current_y - !simulation->y_top + 1;
    #ifdef DEBUG
    printf("effective_x is %d, effective_y is %d\n", effective_x, effective_y);
    #endif
    int stoplight_time = simulation->times[effective_x][effective_y];

    int current_time = simulation -> cur_t % (simulation -> stoplight_time * 3);
    int cycle_time = simulation -> stoplight_time * 3;
    if (direction == Top) {
        if (current_time <= stoplight_time) {
            return 0;
        } else {
            return cycle_time - current_time;
        }
    } else if (direction == Right) {
        if (current_time <= stoplight_time) {
            return stoplight_time - current_time;
        } else {
            return 0;
        }
    } else {
#ifdef DEBUG
        printf("Was asked for stoplight_wait on non top/right direction %d\n", direction);
#endif
        return -1;
    }
}

bool step_simul(struct simul *simulation) {
    PolicyResult response = simulation->policy(simulation);
#ifdef DEBUG
    printf("Got policy response %d.\n", response);
#endif

    if (response == Right) {
#ifdef DEBUG
        printf("Handling right response\n");
#endif
        if (simulation -> x_right) { // we're at the right, so if we go right now we're crossing the street
            simulation -> cur_t += simulation->street_width + stoplight_wait(simulation, response);
            simulation -> x_right = false;
            simulation -> current_x += 1;
        } else { // here we're crossing the block
            simulation->cur_t += simulation->block_width;
            simulation->x_right = true;
        }
    } else if (response == Top) {
#ifdef DEBUG
        printf("Handling top response\n");
#endif
        if (simulation -> y_top) { // we're at the top, so we're crossing the street here
            simulation->cur_t += simulation->street_width + stoplight_wait(simulation, response);
            simulation -> y_top = false;
            simulation -> current_y += 1;
        } else {
            simulation->cur_t += simulation->block_height;
            simulation->y_top = true;
        }
    } else {
        fprintf(stderr, "Erroneous policy function %p, returned response %d\n", simulation->policy, response);
    }

    if ((simulation -> current_x + 1) == simulation -> blocks_wide &&
        (simulation -> current_y + 1) == simulation -> blocks_high &&
        simulation -> x_right &&
        simulation -> y_top) {
        return false; // we've reached our destination
    }

    return true;
}

PolicyResult default_policy(struct simul * simulation) {
    if (simulation -> current_y+1 < simulation -> blocks_high || !simulation->y_top) {
        return Top;
    }
    return Right;
}

struct simul * init_simul(int blocks_wide, int blocks_high, int block_height, int block_width, int stoplight_time, int street_width, PolicyFunc policy) {
    struct simul *simulation = malloc(sizeof(struct simul));

    simulation -> cur_t = 0;
    simulation -> current_x = 0;
    simulation -> current_y = 0;

    simulation -> blocks_wide = blocks_wide;
    simulation -> blocks_high = blocks_high;
    simulation -> block_width = block_width;
    simulation -> block_height = block_height;
    simulation -> street_width = street_width;

    simulation -> stoplight_time = stoplight_time;

    simulation -> x_right = false;
    simulation -> y_top = false;

    srandomdev();

    simulation -> times = malloc(sizeof(int *) * blocks_wide);
    for (int i = 0; i < blocks_wide+1; i++) {
        simulation -> times[i] = malloc(sizeof(int) * blocks_high);
        for (int j = 0; j < blocks_high+1; j++) {
            // total times is stoplight_time*3, variance is from 1/3-2/3 of time...
            simulation -> times[i][j] = stoplight_time + (random() % stoplight_time);
        }
    }

    if (!policy) {
        policy = default_policy;
    }
    simulation -> policy = policy;

    return simulation;
}

int main() {
    int s = clock();
    struct simul *simulation = init_simul(30, 30, 5, 5, 10, 2, NULL);
    while (step_simul(simulation)){}

    int j = clock();

    printf("Took %d seconds (%d seconds irl)!\n", simulation->cur_t, j-s);
    return 0;
}

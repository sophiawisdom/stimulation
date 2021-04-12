#include "stdbool.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef enum PolicyResult {
    Right, // go right, even if it means waiting
    Top, // go top, even if it means waiting.
} PolicyResult;

struct simul;

typedef PolicyResult (*PolicyFunc)(struct simul * test);

struct simul {
    int time_waiting;
    char move_sequence[128]; // stored as an *inline* sequence of bools. this means max width+height is 1024
    int cur_move;

    int block_height;
    int block_width;
    int blocks_high;
    int blocks_wide;
    int street_width;

    // simulation -> times[effective_x * simulation -> blocks_high + effective_y]

    int *times; // times[3 * simulation -> blocks_high + 6] is block (x=3, y=6)

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
    int stoplight_time = simulation -> times[effective_x * simulation -> blocks_high + effective_y];

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
            int wait_time = stoplight_wait(simulation, response);
            simulation -> time_waiting += wait_time;
            simulation -> cur_t += simulation->street_width + wait_time;
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
            int wait_time = stoplight_wait(simulation, response);
            simulation -> time_waiting += wait_time;
            simulation->cur_t += simulation->street_width + wait_time;
            simulation -> y_top = false;
            simulation -> current_y += 1;
        } else {
            simulation->cur_t += simulation->block_height;
            simulation->y_top = true;
        }
    } else {
        fprintf(stderr, "Erroneous policy function %p, returned response %d\n", simulation->policy, response);
        return false;
    }

    if (response == Top) {
        simulation -> move_sequence[simulation -> cur_move /8] |= 1 << (simulation -> cur_move % 8);
    }
    simulation -> cur_move += 1;

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

    // diagnostics...
    simulation -> cur_move = 0;
    simulation -> time_waiting = 0;

    simulation -> stoplight_time = stoplight_time;

    simulation -> x_right = false;
    simulation -> y_top = false;

    // POSSIBLE OPTIMIZATION:
    // currently this is O(width*height), but could be improved to O(width+height) if we just generate the numbers on-demand...
    // this could make the policy network more annoying though
    // ANOTHER POSSIBLY OPTIMIZATION:
    // alloca() this array

    // This is >half the overall time at 300x300 array

    simulation -> times = malloc(sizeof(int *) * blocks_wide * blocks_high); // allocing as a single array vs a series of arrays shaves off 16% of the total time
    for (int i = 0; i < blocks_wide+1; i++) {
        int x = i * simulation -> blocks_high;
        for (int j = 0; j < blocks_high+1; j++) {
            // total times is stoplight_time*3, variance is from 1/3-2/3 of time...
            simulation -> times[x+j] = stoplight_time + (random() % stoplight_time);
        }
    }

    if (!policy) {
        policy = default_policy;
    }
    simulation -> policy = policy;

    return simulation;
}

int main() {
    srandomdev();
    int s = clock();

    unsigned long long total = 0;
    for (int i = 0; i < 1000000; i++) {
        // simulation is currently at ~100,000x faster than the putative time it measures
        struct simul *simulation = init_simul(300, 300, 5, 5, 10, 2, NULL);
        while (step_simul(simulation)){}
        total += simulation->cur_t;
    }

    int j = clock();

    printf("Took %llu seconds (%d seconds irl)!\n", total/1000, j-s);
    return 0;
}
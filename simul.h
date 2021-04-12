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
struct simul * init_simul(int blocks_wide, int blocks_high, int block_height, int block_width, int stoplight_time, int street_width, PolicyFunc policy);
bool step_simul(struct simul *simulation);
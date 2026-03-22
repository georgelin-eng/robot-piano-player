#define MOVE 0
#define PLAY 1

const int INITIAL_MOTOR_POSITION_MM = 240;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

struct command schedule[] = {
    {PLAY, 16, 0.500f, 0.687f},
    {MOVE, 240, 0.286f, 1.000f},
    {PLAY, 6272, 1.000f, 1.124f},
    {PLAY, 16, 1.500f, 1.687f},
    {PLAY, 513, 2.000f, 2.124f},
    {PLAY, 16, 2.500f, 2.687f},
    {PLAY, 9218, 3.000f, 3.124f},
    {PLAY, 16, 3.500f, 3.687f},
    {PLAY, 521, 4.000f, 4.124f},
    {PLAY, 16, 4.500f, 4.687f},
    {PLAY, 6272, 5.000f, 5.124f},
    {PLAY, 16, 5.500f, 5.687f},
    {PLAY, 513, 6.000f, 6.124f},
    {PLAY, 16, 6.500f, 6.687f},
    {MOVE, 200, 6.886f, 7.000f},
    {PLAY, 5122, 7.000f, 7.124f},
    {PLAY, 16, 7.500f, 7.687f},
    {PLAY, 521, 8.000f, 8.124f},
    {PLAY, 16, 8.500f, 8.687f},
    {MOVE, 100, 8.714f, 9.000f},
    {PLAY, 10368, 9.000f, 9.124f},
    {PLAY, 4096, 9.250f, 9.312f},
    {PLAY, 8208, 9.500f, 9.687f},
    {PLAY, 513, 10.000f, 10.124f},
    {PLAY, 16, 10.500f, 10.687f},
    {PLAY, 5122, 11.000f, 11.124f},
    {MOVE, 120, 11.443f, 11.500f},
    {PLAY, 4112, 11.500f, 11.687f},
    {PLAY, 4096, 11.750f, 11.874f},
    {PLAY, 4609, 12.000f, 12.124f},
    {PLAY, 16, 12.500f, 12.687f},
    {PLAY, 1026, 13.000f, 13.124f},
    {MOVE, 140, 13.443f, 13.500f},
    {PLAY, 4112, 13.500f, 13.687f},
    {PLAY, 4096, 13.750f, 13.874f},
    {PLAY, 4609, 14.000f, 14.124f},
    {MOVE, 160, 14.443f, 14.500f},
    {PLAY, 6288, 14.500f, 14.624f},
    {MOVE, 140, 14.693f, 14.750f},
    {PLAY, 8192, 14.750f, 14.874f},
    {PLAY, 4625, 15.000f, 15.124f},
    {PLAY, 8192, 15.250f, 15.374f},
    {MOVE, 160, 15.443f, 15.500f},
    {PLAY, 4112, 15.500f, 15.624f},
    {PLAY, 4, 15.750f, 15.874f},
    {PLAY, 1, 16.000f, 16.124f},
    {PLAY, 2048, 16.250f, 16.374f},
    {PLAY, 1024, 16.500f, 16.624f},
};

const int SCHEDULE_LENGTH = 48;

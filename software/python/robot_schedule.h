#define MOVE 0
#define PLAY 1

const int INITIAL_MOTOR_POSITION_MM = 182.4;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

struct command schedule[] = {
    {MOVE, 182.4, 4.669f, 5.000f},
    {PLAY, 4096, 5.000f, 5.748f},
    {PLAY, 4096, 5.750f, 6.311f},
    {PLAY, 4096, 6.312f, 6.498f},
    {PLAY, 65536, 6.500f, 7.811f},
    {PLAY, 65536, 7.812f, 7.998f},
    {MOVE, 159.6, 7.952f, 8.052f},
    {PLAY, 65536, 8.052f, 8.613f},
    {PLAY, 65536, 8.614f, 8.800f},
    {MOVE, 159.6, 8.792f, 8.892f},
    {PLAY, 4096, 8.892f, 9.453f},
    {MOVE, 136.8, 9.406f, 9.506f},
    {PLAY, 16384, 9.506f, 9.692f},
    {MOVE, 114.0, 9.646f, 9.746f},
    {PLAY, 65536, 9.746f, 11.244f},
    {MOVE, 182.4, 11.122f, 11.246f},
    {PLAY, 65536, 11.246f, 11.807f},
    {PLAY, 65536, 11.808f, 11.994f},
    {PLAY, 4096, 11.996f, 12.557f},
    {MOVE, 228.0, 12.472f, 12.572f},
    {PLAY, 16384, 12.572f, 12.758f},
    {PLAY, 4096, 12.760f, 13.508f},
    {MOVE, 205.2, 13.462f, 13.562f},
    {PLAY, 16384, 13.562f, 14.123f},
    {PLAY, 65536, 14.124f, 14.310f},
    {MOVE, 182.4, 14.264f, 14.364f},
    {PLAY, 65536, 14.364f, 15.112f},
    {MOVE, 205.2, 15.066f, 15.166f},
    {PLAY, 65536, 15.166f, 15.727f},
    {PLAY, 4096, 15.728f, 15.914f},
    {MOVE, 228.0, 15.868f, 15.968f},
    {PLAY, 4096, 15.968f, 18.216f},
};

const int SCHEDULE_LENGTH = 32;

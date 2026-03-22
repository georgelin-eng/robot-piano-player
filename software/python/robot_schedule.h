#define MOVE 0
#define PLAY 1

const int INITIAL_MOTOR_POSITION_MM = 140;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

struct command schedule[] = {
    {PLAY, 16, 0.500f, 0.700f},
    {MOVE, 140, 0.553f, 0.982f},
    {PLAY, 10368, 0.982f, 1.182f},
    {PLAY, 16, 1.520f, 1.720f},
    {PLAY, 513, 2.001f, 2.201f},
    {PLAY, 16, 2.539f, 2.739f},
    {MOVE, 200, 2.850f, 3.050f},
    {PLAY, 5122, 3.050f, 3.250f},
    {PLAY, 16, 3.588f, 3.788f},
    {PLAY, 521, 4.069f, 4.269f},
    {PLAY, 16, 4.607f, 4.807f},
    {MOVE, 140, 4.918f, 5.118f},
    {PLAY, 10368, 5.118f, 5.318f},
    {PLAY, 16, 5.656f, 5.856f},
    {PLAY, 513, 6.137f, 6.337f},
    {PLAY, 16, 6.675f, 6.875f},
    {MOVE, 200, 6.986f, 7.186f},
    {PLAY, 5122, 7.186f, 7.386f},
    {PLAY, 16, 7.724f, 7.924f},
    {PLAY, 521, 8.205f, 8.405f},
    {PLAY, 16, 8.743f, 8.943f},
    {MOVE, 100, 8.939f, 9.225f},
    {PLAY, 10368, 9.225f, 9.425f},
    {PLAY, 4096, 9.538f, 9.738f},
    {PLAY, 8208, 9.907f, 10.107f},
    {PLAY, 513, 10.389f, 10.589f},
    {PLAY, 16, 10.927f, 11.127f},
    {PLAY, 5122, 11.409f, 11.609f},
    {MOVE, 120, 11.889f, 12.089f},
    {PLAY, 4112, 12.089f, 12.289f},
    {PLAY, 4096, 12.346f, 12.546f},
    {PLAY, 4609, 12.659f, 12.859f},
    {PLAY, 16, 13.197f, 13.397f},
    {PLAY, 1026, 13.679f, 13.879f},
    {MOVE, 140, 14.160f, 14.360f},
    {PLAY, 4112, 14.360f, 14.560f},
    {PLAY, 4096, 14.616f, 14.816f},
    {PLAY, 4609, 14.929f, 15.129f},
    {MOVE, 160, 15.410f, 15.610f},
    {PLAY, 6288, 15.610f, 15.810f},
    {MOVE, 140, 15.866f, 16.066f},
    {PLAY, 8192, 16.066f, 16.266f},
    {PLAY, 4625, 16.379f, 16.579f},
    {PLAY, 8192, 16.692f, 16.892f},
    {MOVE, 160, 16.948f, 17.148f},
    {PLAY, 4112, 17.148f, 17.348f},
    {PLAY, 4, 17.461f, 17.661f},
    {PLAY, 1, 17.774f, 17.974f},
    {PLAY, 2048, 18.087f, 18.287f},
    {PLAY, 1024, 18.400f, 18.600f},
};

const int SCHEDULE_LENGTH = 50;

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
    {PLAY, 67712, 0.982f, 1.182f},
    {PLAY, 16, 1.520f, 1.720f},
    {MOVE, 220, 1.773f, 2.001f},
    {PLAY, 8705, 2.001f, 2.201f},
    {PLAY, 16, 2.539f, 2.739f},
    {MOVE, 200, 2.964f, 3.164f},
    {PLAY, 5122, 3.164f, 3.364f},
    {PLAY, 16, 3.702f, 3.902f},
    {PLAY, 521, 4.184f, 4.384f},
    {PLAY, 16, 4.722f, 4.922f},
    {MOVE, 140, 5.032f, 5.232f},
    {PLAY, 67712, 5.232f, 5.432f},
    {PLAY, 16, 5.770f, 5.970f},
    {MOVE, 220, 6.023f, 6.252f},
    {PLAY, 8705, 6.252f, 6.452f},
    {PLAY, 16, 6.790f, 6.990f},
    {MOVE, 200, 7.214f, 7.414f},
    {PLAY, 5122, 7.414f, 7.614f},
    {PLAY, 16, 7.952f, 8.152f},
    {PLAY, 521, 8.434f, 8.634f},
    {PLAY, 16, 8.972f, 9.172f},
    {MOVE, 100, 9.168f, 9.454f},
    {PLAY, 67712, 9.454f, 9.654f},
    {PLAY, 4096, 9.766f, 9.966f},
    {PLAY, 65552, 10.136f, 10.336f},
    {PLAY, 33281, 10.617f, 10.817f},
    {MOVE, 120, 11.098f, 11.298f},
    {PLAY, 8208, 11.298f, 11.498f},
    {PLAY, 17410, 11.780f, 11.980f},
    {PLAY, 4112, 12.318f, 12.518f},
    {PLAY, 4096, 12.575f, 12.775f},
    {PLAY, 4609, 12.888f, 13.088f},
    {MOVE, 140, 13.368f, 13.568f},
    {PLAY, 8208, 13.568f, 13.768f},
    {PLAY, 8192, 13.825f, 14.025f},
    {PLAY, 9218, 14.138f, 14.338f},
    {PLAY, 4112, 14.676f, 14.876f},
    {PLAY, 4096, 14.933f, 15.133f},
    {PLAY, 4609, 15.246f, 15.446f},
    {MOVE, 160, 15.727f, 15.927f},
    {PLAY, 6288, 15.927f, 16.127f},
    {MOVE, 140, 16.182f, 16.382f},
    {PLAY, 65536, 16.382f, 16.582f},
    {PLAY, 4625, 16.695f, 16.895f},
    {PLAY, 65536, 17.008f, 17.208f},
    {MOVE, 160, 17.264f, 17.464f},
    {PLAY, 4112, 17.464f, 17.664f},
    {PLAY, 4, 17.777f, 17.977f},
    {PLAY, 1, 18.090f, 18.290f},
    {PLAY, 2048, 18.403f, 18.603f},
    {PLAY, 1024, 18.716f, 18.916f},
};

const int SCHEDULE_LENGTH = 53;

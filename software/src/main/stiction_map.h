typedef struct {
    float  measured_rad;
    double stiction_pwm;
} stiction_measurements;


// arrange small to large 
stiction_measurements lut[] = {
    {0.0f, 1},
    {10.0f, 2},
    {20.0f, 3},
    {30.0f, 4}
};

const int LUT_SIZE = sizeof(lut) / sizeof(lut[0]);


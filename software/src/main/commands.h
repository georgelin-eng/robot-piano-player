#ifndef COMMANDS_H
#define COMMANDS_H

#define MOVE 0
#define SOLENOID_ON 1
#define SOLENOID_OFF 2

<<<<<<< HEAD
const float INITIAL_MOTOR_POSITION_MM = 178.6f;
=======
const float INITIAL_MOTOR_POSITION_MM = 179.2f;
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7

struct command {
    float time;
    uint8_t action;
    float position_mm;
    uint32_t solenoid_mask;
};

struct command schedule[] = {
    {0.500f, SOLENOID_ON, 0.0f, 64},
    {0.748f, SOLENOID_OFF, 0.0f, 64},
    {0.750f, SOLENOID_ON, 0.0f, 128},
    {0.998f, SOLENOID_OFF, 0.0f, 128},
    {1.000f, SOLENOID_ON, 0.0f, 4},
    {1.248f, SOLENOID_OFF, 0.0f, 4},
    {1.250f, SOLENOID_ON, 0.0f, 2048},
    {1.498f, SOLENOID_OFF, 0.0f, 2048},
    {1.500f, SOLENOID_ON, 0.0f, 128},
    {1.748f, SOLENOID_OFF, 0.0f, 128},
    {1.750f, SOLENOID_ON, 0.0f, 4},
    {1.998f, SOLENOID_OFF, 0.0f, 4},
    {2.000f, SOLENOID_ON, 0.0f, 2048},
    {2.248f, SOLENOID_OFF, 0.0f, 2048},
    {2.250f, SOLENOID_ON, 0.0f, 128},
    {2.498f, SOLENOID_OFF, 0.0f, 128},
    {2.500f, SOLENOID_ON, 0.0f, 4},
    {2.748f, SOLENOID_OFF, 0.0f, 4},
    {2.750f, SOLENOID_ON, 0.0f, 2048},
    {2.998f, SOLENOID_OFF, 0.0f, 2048},
    {3.000f, SOLENOID_ON, 0.0f, 128},
    {3.248f, SOLENOID_OFF, 0.0f, 128},
    {3.250f, SOLENOID_ON, 0.0f, 4},
    {3.498f, SOLENOID_OFF, 0.0f, 4},
    {3.500f, SOLENOID_ON, 0.0f, 64},
    {3.748f, SOLENOID_OFF, 0.0f, 64},
    {3.750f, SOLENOID_ON, 0.0f, 128},
    {3.998f, SOLENOID_OFF, 0.0f, 128},
    {4.000f, SOLENOID_ON, 0.0f, 4},
    {4.248f, SOLENOID_OFF, 0.0f, 4},
    {4.250f, SOLENOID_ON, 0.0f, 2048},
    {4.498f, SOLENOID_OFF, 0.0f, 2048},
    {4.500f, SOLENOID_ON, 0.0f, 128},
    {4.748f, SOLENOID_OFF, 0.0f, 128},
    {4.750f, SOLENOID_ON, 0.0f, 4},
    {4.998f, SOLENOID_OFF, 0.0f, 4},
    {5.000f, SOLENOID_ON, 0.0f, 6144},
    {5.248f, SOLENOID_OFF, 0.0f, 2048},
    {5.250f, SOLENOID_ON, 0.0f, 128},
    {5.498f, SOLENOID_OFF, 0.0f, 128},
    {5.500f, SOLENOID_ON, 0.0f, 4},
    {5.748f, SOLENOID_OFF, 0.0f, 4100},
    {5.750f, SOLENOID_ON, 0.0f, 6144},
    {5.998f, SOLENOID_OFF, 0.0f, 2048},
    {6.000f, SOLENOID_ON, 0.0f, 128},
    {6.248f, SOLENOID_OFF, 0.0f, 128},
    {6.250f, SOLENOID_ON, 0.0f, 4},
    {6.311f, SOLENOID_OFF, 0.0f, 4096},
    {6.312f, SOLENOID_ON, 0.0f, 4096},
    {6.413f, SOLENOID_OFF, 0.0f, 4096},
<<<<<<< HEAD
    {6.418f, MOVE, 135.400f, 0},
=======
    {6.418f, MOVE, 136.000f, 0},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {6.498f, SOLENOID_OFF, 0.0f, 4},
    {6.500f, SOLENOID_ON, 0.0f, 6144},
    {6.748f, SOLENOID_OFF, 0.0f, 2048},
    {6.750f, SOLENOID_ON, 0.0f, 128},
    {6.998f, SOLENOID_OFF, 0.0f, 128},
    {7.000f, SOLENOID_ON, 0.0f, 4},
    {7.248f, SOLENOID_OFF, 0.0f, 4},
    {7.250f, SOLENOID_ON, 0.0f, 2048},
    {7.498f, SOLENOID_OFF, 0.0f, 2048},
    {7.500f, SOLENOID_ON, 0.0f, 128},
    {7.748f, SOLENOID_OFF, 0.0f, 128},
    {7.750f, SOLENOID_ON, 0.0f, 4},
    {7.811f, SOLENOID_OFF, 0.0f, 4096},
    {7.812f, SOLENOID_ON, 0.0f, 4096},
<<<<<<< HEAD
    {7.985f, SOLENOID_OFF, 0.0f, 4096},
    {7.990f, MOVE, 135.400f, 0},
    {7.998f, SOLENOID_OFF, 0.0f, 4},
    {8.000f, SOLENOID_ON, 0.0f, 278529},
=======
    {7.908f, SOLENOID_OFF, 0.0f, 4096},
    {7.913f, MOVE, 89.800f, 0},
    {7.998f, SOLENOID_OFF, 0.0f, 4},
    {8.000f, SOLENOID_ON, 0.0f, 16385},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {8.248f, SOLENOID_OFF, 0.0f, 1},
    {8.250f, SOLENOID_ON, 0.0f, 128},
    {8.498f, SOLENOID_OFF, 0.0f, 128},
    {8.500f, SOLENOID_ON, 0.0f, 16},
<<<<<<< HEAD
    {8.561f, SOLENOID_OFF, 0.0f, 278528},
    {8.562f, SOLENOID_ON, 0.0f, 278528},
    {8.699f, SOLENOID_OFF, 0.0f, 278528},
    {8.704f, MOVE, 157.100f, 0},
=======
    {8.561f, SOLENOID_OFF, 0.0f, 16384},
    {8.562f, SOLENOID_ON, 0.0f, 16384},
    {8.657f, SOLENOID_OFF, 0.0f, 16384},
    {8.662f, MOVE, 157.700f, 0},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {8.748f, SOLENOID_OFF, 0.0f, 16},
    {8.750f, SOLENOID_ON, 0.0f, 4097},
    {8.998f, SOLENOID_OFF, 0.0f, 1},
    {9.000f, SOLENOID_ON, 0.0f, 128},
    {9.221f, SOLENOID_OFF, 0.0f, 4096},
<<<<<<< HEAD
    {9.226f, MOVE, 111.700f, 0},
    {9.248f, SOLENOID_OFF, 0.0f, 128},
    {9.250f, SOLENOID_ON, 0.0f, 16},
    {9.312f, SOLENOID_ON, 0.0f, 4096},
    {9.445f, SOLENOID_OFF, 0.0f, 4096},
    {9.450f, MOVE, 87.600f, 0},
    {9.498f, SOLENOID_OFF, 0.0f, 16},
    {9.500f, SOLENOID_ON, 0.0f, 280576},
=======
    {9.226f, MOVE, 112.300f, 0},
    {9.248f, SOLENOID_OFF, 0.0f, 128},
    {9.250f, SOLENOID_ON, 0.0f, 16},
    {9.312f, SOLENOID_ON, 0.0f, 4096},
    {9.407f, SOLENOID_OFF, 0.0f, 4096},
    {9.412f, MOVE, 42.000f, 0},
    {9.498f, SOLENOID_OFF, 0.0f, 16},
    {9.500f, SOLENOID_ON, 0.0f, 18432},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {9.748f, SOLENOID_OFF, 0.0f, 2048},
    {9.750f, SOLENOID_ON, 0.0f, 128},
    {9.998f, SOLENOID_OFF, 0.0f, 128},
    {10.000f, SOLENOID_ON, 0.0f, 4},
    {10.248f, SOLENOID_OFF, 0.0f, 4},
    {10.250f, SOLENOID_ON, 0.0f, 2048},
    {10.498f, SOLENOID_OFF, 0.0f, 2048},
    {10.500f, SOLENOID_ON, 0.0f, 128},
    {10.748f, SOLENOID_OFF, 0.0f, 128},
    {10.750f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {10.867f, SOLENOID_OFF, 0.0f, 278528},
    {10.872f, MOVE, 158.400f, 0},
    {10.998f, SOLENOID_OFF, 0.0f, 4},
    {11.000f, SOLENOID_ON, 0.0f, 18432},
=======
    {10.828f, SOLENOID_OFF, 0.0f, 16384},
    {10.833f, MOVE, 136.000f, 0},
    {10.998f, SOLENOID_OFF, 0.0f, 4},
    {11.000f, SOLENOID_ON, 0.0f, 6144},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {11.248f, SOLENOID_OFF, 0.0f, 2048},
    {11.250f, SOLENOID_ON, 0.0f, 128},
    {11.498f, SOLENOID_OFF, 0.0f, 128},
    {11.500f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {11.561f, SOLENOID_OFF, 0.0f, 16384},
    {11.562f, SOLENOID_ON, 0.0f, 16384},
    {11.701f, SOLENOID_OFF, 0.0f, 16384},
    {11.706f, MOVE, 178.600f, 0},
=======
    {11.561f, SOLENOID_OFF, 0.0f, 4096},
    {11.562f, SOLENOID_ON, 0.0f, 4096},
    {11.663f, SOLENOID_OFF, 0.0f, 4096},
    {11.668f, MOVE, 179.200f, 0},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {11.748f, SOLENOID_OFF, 0.0f, 4},
    {11.750f, SOLENOID_ON, 0.0f, 6144},
    {11.998f, SOLENOID_OFF, 0.0f, 2048},
    {12.000f, SOLENOID_ON, 0.0f, 128},
    {12.248f, SOLENOID_OFF, 0.0f, 128},
    {12.250f, SOLENOID_ON, 0.0f, 4},
    {12.295f, SOLENOID_OFF, 0.0f, 4096},
<<<<<<< HEAD
    {12.300f, MOVE, 180.100f, 0},
    {12.312f, SOLENOID_ON, 0.0f, 16384},
    {12.407f, SOLENOID_OFF, 0.0f, 16384},
    {12.412f, MOVE, 247.600f, 0},
    {12.498f, SOLENOID_OFF, 0.0f, 4},
    {12.500f, SOLENOID_ON, 0.0f, 280576},
=======
    {12.300f, MOVE, 180.500f, 0},
    {12.312f, SOLENOID_ON, 0.0f, 16384},
    {12.483f, SOLENOID_OFF, 0.0f, 16384},
    {12.488f, MOVE, 179.200f, 0},
    {12.498f, SOLENOID_OFF, 0.0f, 4},
    {12.500f, SOLENOID_ON, 0.0f, 6144},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {12.748f, SOLENOID_OFF, 0.0f, 2048},
    {12.750f, SOLENOID_ON, 0.0f, 128},
    {12.998f, SOLENOID_OFF, 0.0f, 128},
    {13.000f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {13.248f, SOLENOID_OFF, 0.0f, 278532},
    {13.250f, SOLENOID_ON, 0.0f, 264192},
    {13.498f, SOLENOID_OFF, 0.0f, 2048},
    {13.500f, SOLENOID_ON, 0.0f, 128},
    {13.646f, SOLENOID_OFF, 0.0f, 262144},
    {13.651f, MOVE, 157.100f, 0},
    {13.748f, SOLENOID_OFF, 0.0f, 128},
    {13.750f, SOLENOID_ON, 0.0f, 4},
    {13.812f, SOLENOID_ON, 0.0f, 4096},
    {13.915f, SOLENOID_OFF, 0.0f, 4096},
    {13.920f, MOVE, 115.300f, 0},
    {13.998f, SOLENOID_OFF, 0.0f, 4},
    {14.000f, SOLENOID_ON, 0.0f, 32832},
=======
    {13.248f, SOLENOID_OFF, 0.0f, 4100},
    {13.250f, SOLENOID_ON, 0.0f, 6144},
    {13.498f, SOLENOID_OFF, 0.0f, 2048},
    {13.500f, SOLENOID_ON, 0.0f, 128},
    {13.748f, SOLENOID_OFF, 0.0f, 128},
    {13.750f, SOLENOID_ON, 0.0f, 4},
    {13.761f, SOLENOID_OFF, 0.0f, 4096},
    {13.766f, MOVE, 157.700f, 0},
    {13.812f, SOLENOID_ON, 0.0f, 4096},
    {13.953f, SOLENOID_OFF, 0.0f, 4096},
    {13.958f, MOVE, 138.300f, 0},
    {13.998f, SOLENOID_OFF, 0.0f, 4},
    {14.000f, SOLENOID_ON, 0.0f, 135232},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {14.248f, SOLENOID_OFF, 0.0f, 64},
    {14.250f, SOLENOID_ON, 0.0f, 64},
    {14.498f, SOLENOID_OFF, 0.0f, 64},
    {14.500f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {14.665f, SOLENOID_OFF, 0.0f, 32768},
    {14.670f, MOVE, 157.100f, 0},
=======
    {14.703f, SOLENOID_OFF, 0.0f, 135168},
    {14.708f, MOVE, 157.700f, 0},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {14.748f, SOLENOID_OFF, 0.0f, 4},
    {14.750f, SOLENOID_ON, 0.0f, 4608},
    {14.998f, SOLENOID_OFF, 0.0f, 512},
    {15.000f, SOLENOID_ON, 0.0f, 64},
<<<<<<< HEAD
    {15.184f, SOLENOID_OFF, 0.0f, 4096},
    {15.189f, MOVE, 224.600f, 0},
    {15.248f, SOLENOID_OFF, 0.0f, 64},
    {15.250f, SOLENOID_ON, 0.0f, 4},
    {15.312f, SOLENOID_ON, 0.0f, 16384},
    {15.498f, SOLENOID_OFF, 0.0f, 16388},
=======
    {15.223f, SOLENOID_OFF, 0.0f, 4096},
    {15.228f, MOVE, 202.200f, 0},
    {15.248f, SOLENOID_OFF, 0.0f, 64},
    {15.250f, SOLENOID_ON, 0.0f, 4},
    {15.312f, SOLENOID_ON, 0.0f, 4096},
    {15.447f, SOLENOID_OFF, 0.0f, 4096},
    {15.452f, MOVE, 225.200f, 0},
    {15.498f, SOLENOID_OFF, 0.0f, 4},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {15.500f, SOLENOID_ON, 0.0f, 6144},
    {15.748f, SOLENOID_OFF, 0.0f, 2048},
    {15.750f, SOLENOID_ON, 0.0f, 128},
    {15.998f, SOLENOID_OFF, 0.0f, 128},
    {16.000f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {16.086f, SOLENOID_OFF, 0.0f, 4096},
    {16.091f, MOVE, 135.400f, 0},
    {16.248f, SOLENOID_OFF, 0.0f, 4},
    {16.250f, SOLENOID_ON, 0.0f, 264192},
=======
    {16.009f, SOLENOID_OFF, 0.0f, 4096},
    {16.014f, MOVE, 89.800f, 0},
    {16.248f, SOLENOID_OFF, 0.0f, 4},
    {16.250f, SOLENOID_ON, 0.0f, 18432},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {16.498f, SOLENOID_OFF, 0.0f, 2048},
    {16.500f, SOLENOID_ON, 0.0f, 128},
    {16.748f, SOLENOID_OFF, 0.0f, 128},
    {16.750f, SOLENOID_ON, 0.0f, 4},
<<<<<<< HEAD
    {16.836f, SOLENOID_OFF, 0.0f, 262144},
    {16.841f, MOVE, 224.600f, 0},
=======
    {16.759f, SOLENOID_OFF, 0.0f, 16384},
    {16.764f, MOVE, 225.200f, 0},
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7
    {16.998f, SOLENOID_OFF, 0.0f, 4},
    {17.000f, SOLENOID_ON, 0.0f, 4112},
    {17.248f, SOLENOID_OFF, 0.0f, 16},
    {17.250f, SOLENOID_ON, 0.0f, 64},
    {17.498f, SOLENOID_OFF, 0.0f, 64},
    {17.500f, SOLENOID_ON, 0.0f, 128},
    {17.748f, SOLENOID_OFF, 0.0f, 4224},
};

<<<<<<< HEAD
const int SCHEDULE_LENGTH = 180;






#endif
=======
const int SCHEDULE_LENGTH = 182;
>>>>>>> b226372e940c0fbfa87e8531f43d6d4b2d2783f7

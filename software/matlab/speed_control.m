CF = 200;
dt = 1/CF;
PWM_freq = 10e3;
Ka = 24; % 24V DC PWM applied to motor

% Make sure control signal is 0 - 1
Ga = Ka * PWM_freq / (s + PWM_freq);
G = minreal(Ga * Gp * s * H);

% ---------- CONTROLLER DESIGN ----------

% Plant is without integrator for speed
OLTF = G * H;
[Gm, Pm, wxo, ~] = margin(OLTF*s);
K0 = Gm;

z = -wxo;

Kp = -1/z * K0;
Ki = 1   * K0;

K_PID.Kp = Kp;
K_PID.Ki = Ki;
K_PID.Kd = 0;

D = (K_PID.Kp) + (K_PID.Ki / s);
Gc = D;
CLTF = G * D / (1 + G * H * D);
[Tr, Tp, Ts, OSu] = RCG_from_CLTF(CLTF);

Ktune(K_PID, G, H, p, 5, OSu)
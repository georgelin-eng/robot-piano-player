CF = 200;
dt = 1/CF;
PWM_freq = 10e3;
Ka = 24; % 24V DC PWM applied to motor

% Make sure control signal is 0 - 1
Ga = Ka * PWM_freq / (s + PWM_freq);
GHs = Hs * Ga * Gp;

p = pole(minreal(GHs));
tol = 1e-6;                         % numerical tolerance
p_nz = p(abs(real(p)) > tol);       % exclude (near) zero real-part poles
wd = -max(real(p_nz));
Nf = CF/(ceil(wd)*10) - 0.5 - DC;
beta = Nf/(1+Nf); 
tau = -dt / log(beta);

Nc = DC;
Nh = 0.5;

N = Nc + Nh + Nf; 
Hc = CF / (N*s + CF); % scale by sensor gain

H = Hs * Hc;
G = Ga * Gp;

p = pole(Hc);

% ---------- CONTROLLER DESIGN ----------

% Integrator pole needs to be stabalized
% Dp = 1/s * CF / (N * s + CF);
OLTF = Ga * Gp * H;
[Gm, Pm, wxo, ~] = margin(OLTF);
K0 = Gm;

Z1 = -12;
Z2 = -12;

Kp = 1/p - (Z1+Z2) / (Z1*Z2);
Kd = 1/p * Kp + 1/(Z1*Z2);

% Initial Guess
K0 = K0 * 1;
K_PID.Kp = Kp * K0;
K_PID.Ki =   1* K0;
K_PID.Kd = Kd * K0;

K_PID.Kp = K_PID.Kp * 0.9953;
K_PID.Ki = K_PID.Ki * 0.0587;
K_PID.Kd = K_PID.Kd * 1.9847;

K_PID.Kp = K_PID.Kp * 1.0186;
K_PID.Ki = K_PID.Ki * 0.1411;
K_PID.Kd = K_PID.Kd * 1.0398;

D = (K_PID.Kp) + (K_PID.Ki  / s) + (K_PID.Kd * -p * s / (s - p));
Gc = D;
CLTF = G * D / (1 + G * H * D);

iw_CLTF = CLTF / (1/s) / Ym / Km;
w_CLTF  = CLTF / (1/s);
PWM_CLTF = CLTF / Gp / Ga;

[Tr, Tp, Ts, OSu] = RCG_from_CLTF(CLTF);


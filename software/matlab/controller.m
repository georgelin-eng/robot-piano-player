dt = 1/CF;
PWM_freq = 20 * 1e3;

% Make sure control signal is 0 - 1
Ga = Vmotor;
GHs = Hs * Ga * Gp;

p = pole(minreal(GHs));
tol = 1e-6;                         % numerical tolerance
p_nz = p(abs(real(p)) > tol);       % exclude (near) zero real-part poles
wd = -max(real(p_nz));
Nf = CF/(ceil(wd)*10) - 0.5 - DC;
% Nf = CF/(ceil(wd)*10);
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

Z1 = -1.01;
Z2 = -1.01;

Kp = 1/p - (Z1+Z2) / (Z1*Z2);
Kd = 1/p * Kp + 1/(Z1*Z2);

% Initial Guess
K_PID.Kp = Kp * K0;
K_PID.Ki =   1* K0;
K_PID.Kd = Kd * K0;

K_PID = load_PID(motor_name, K_PID);

% Make adjustments

% D = (K_PID.Kp) + (K_PID.Ki  / s) + (K_PID.Kd * -p * s / (s - p));
D = K_PID.Kp + K_PID.Ki * 1/s + K_PID.Kd * CF*s / (N*s+CF);
Gc = D;
CLTF = G * D / (1 + G * H * D);

iw_CLTF = CLTF / (1/s) / Ym / Km;
w_CLTF  = CLTF / (1/s);
tau_CLTF = iw_CLTF * Km;
PWM_CLTF = CLTF / Gp / Ga;

[Tr, Tp, Ts, OSu] = RCG_from_CLTF(CLTF);


% Z = roots([1, (Ki-Kp*p)/(Kp-Kd*p), Ki*p/(Kp-Kd*p)]);
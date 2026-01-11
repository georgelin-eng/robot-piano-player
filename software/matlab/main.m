clear clc;
addpath("scripts/")
motor_name = 'Maxon_ECi40';

Mhand = 0.5; % TEST MASS
CF = 200;
DC = 0.5;
WnRes = 0.1;
ZetaRes = 0.01;

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm] = motor_params(motor_name);
plant_params;

n = 1/Rp;

% TODO: Need to estimate inertia of pulley belt and it's mass

Jj = Jm+Jp+1/n^2*Mhand; % Capcitance
Bj = Bm+Bp+1/n^2*Brail; % Resistance, 1/Rtot, Rtot_par = 1/(sum 1/B))
% Kj = 1/n^2 * Kbelt;       % Inductance
Kj = 1/n^2 * Kbelt;       % Inductance

% QUESITON 4
Rtot = 1/Bj;
Ltot = 1/Kj;
Ctot = Jj;

% Resistor, Inductor, and Cap in parallel
Ztot = RR(Rtot, s*Ltot);
Ztot = RR(Ztot, 1/(s*Ctot));

nKm = Km; % joint space speed is same as motor speed

Ye = 1/(s*Lw + Rw);
Ym = Ztot;
Gp = feedback(Ye*nKm*Ym, nKm) * 1/s;

Kjt = n;
Ktj = 1/Kjt;

Hs = sensor_params;

% ---------- DERIVATIVE FILTER DESIGN ----------

dt = 1/CF;
Ga = 1e3; % Perfect gain % TODO: How to model this???

GHs = Hs * Ga * Gp;
wd = -max(real(pole(minreal(GHs))));
Nf = CF/(ceil(wd)*10) - 0.5 - DC;
beta = Nf/(1+Nf); 
tau = -dt / log(beta);

Nc = DC;
Nh = 0.5;

N = Nc + Nh + Nf; 
Hc = CF / (N*s + CF); % scale by sensor gain

H = Hs * Hc;
G = Ga * Gp;

GH = G*H;

% ---------- CONTROLLER DESIGN ----------

Dp = 1/s * CF / (N * s + CF);
OLTF = GH * Dp;
[Gm, ~] = margin(OLTF);
K0 = Gm;

[Gm, Pm, wxo, ~] = margin(K0*OLTF);

Z1 = -wxo;
Z2 = -wxo;

D = (s-Z1)*(s-Z2)/(Z1*Z2)*Dp;
[Gm,Pm, wxo, ~] = margin(D*G*H);

K = 80;
CLTF = feedback(80*D*G, H);

p = pole(Hc);
Kp = 1/p - (Z1+Z2) / (Z1*Z2);
Kd = 1/p * Kp + 1/(Z1*Z2);

K = K * 0.5680 * 1.1680;
Kp = Kp * K * 0.0261;
Ki =   1* K * 1.1269;
Kd = Kd * K * 0.7702;

Kp = Kp * 1;
Ki = Ki * 2.2145;
Kd = Kd * 1;

K_PID.Kp = 0.001;
K_PID.Ki = Ki;
K_PID.Kd = 0.0001;

% Ktune(K_PID, G, H, p, 0.2, OSu)

D = (K_PID.Kp) + (K_PID.Ki  / s) + (K_PID.Kd * -p * s / (s - p));
CLTF = G * D / (1 + G * H * D);

[y, t] = step(CLTF);

s = tf('s');
addpath("scripts/")

p = (s+40)*(s+500)*(s+1500);
Gp = 1 / p;
G = Gp;
N = 2.0;
CF = 100;
Hc = CF / (N*s + CF);

Dp = 1/s * CF / (N * s + CF);
OLTF = Gp * Hc * Dp;

[Gm, Pm, wxo, ~] = margin(OLTF);

K0 = Gm;

Z1 = -wxo;
Z2 = -wxo;
p = pole(Hc);

Kp = 1/p - (Z1+Z2) / (Z1*Z2);
Kd = 1/p * Kp + 1/(Z1*Z2);

K0 = K0 * 1e-1;
K_PID.Kp = Kp * K0;
K_PID.Ki =   1* K0;
K_PID.Kd = Kd * K0;

D = (K_PID.Kp) + (K_PID.Ki  / s) + (K_PID.Kd * -p * s / (s - p));
CLTF = feedback(G*D, H);


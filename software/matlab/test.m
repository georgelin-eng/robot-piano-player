s = tf('s');
addpath("scripts/")

p = s*(s+40)*(s+500)*(s+1500)*(s+3000);
Gp = 1 / p;
G = Gp;
wxo = 8;

z = -wxo;
D = 1/z * (s-z)/s;

OLTF = Gp * D;

Ki = 1;
Kp = -1/z;

K0 = 1e14;
K_PI.Ki = Ki * K0;
K_PI.Kp = Kp * K0;

D = (K_PI.Ki/s + K_PI.Kp);

% [Gm, Pm, wxo, ~] = margin(OLTF);

CLTF = feedback(G*D, 1);


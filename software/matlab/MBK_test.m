addpath("scripts/")

K = 1e9;
B = 1e-4;
M = 1;
torque = 0.05;
F = torque * 30 * 3.6;

s = tf ('s');


C = M;
L = 1/K;
R = 1/B;

ZC1 = 1/(s*C);
ZC2 = 1/(s*C);
ZL = s*L;
ZR = R;

% No spring damper system
Vout = RR(ZC1, ZC2);

% Spring-damper in parallel in between masses
% Zeq = RR(ZR, ZL);
Zeq = ZL;

V1 = RR(ZC1, ZC2 + Zeq);
V2 = V1 * ZC2 / (Zeq + ZC2);

t = 0:1e-6:1;
[y_Damped, tOut] = impulse(V2/s, t);
[y_noDamp, ~]    = impulse(Vout/s, t);

hold on;
plot(tOut, y_Damped);
plot(tOut, y_noDamp);
legend('Spring+Damping','Undamped')




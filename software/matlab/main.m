% clear clc;
addpath("scripts/")
% motor_name = '60PG-997-4.25-EN';
motor_name = '60PG-6055ZY';

Mhand   = 0.43; % Test mass    [kg]
CF      = 200;  % Control freq [Hz]
DC      = 0.5;  % Duty cycle
WnRes   = 2;    % Frequency search res
ZetaRes = 0.05; % Damping factor search res
TargPM  = 60;   % Target phase margin [degrees]

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm, n, Istall] = motor_params(motor_name);
plant_params;

n1 = n;         % gearbox
n2 = 1/Rpulley; % rotational to linear

% Summing components
J1 = Jm + Jgearbox + Jpulley/n1^2 + Jbelt/n1^2;
J2 = Mhand * 1/n1^2 * 1/n2^2;
B1 = Bm + Bp;
B2 = Brail * 1/n1^2 * 1/n2^2;
K1 = Kbelt * 1/n1^2 * 1/n2^2;

% C1 || R1 || (L + C2 || R2)
ZC1 = 1/(s*J1);
ZC2 = 1/(s*J2);
ZR1 = 1/B1;
ZR2 = 1/B2;
ZL1 = s * 1/K1;

Ztot = 1 / (1/ZR1 + 1/ZR2 + 1/(ZL1 + RR(ZC1, ZR2)));

nKm = Km; % joint space speed is same as motor speed

Ye = 1/(s*Lw + Rw);
Ym = Ztot;
Gp = feedback(Ye*nKm*Ym, nKm) * 1/s;

Kjt = 1/n2 * 1/n1;
Ktj = 1/Kjt;

Hs = sensor_params;

% ---------- CONTROLLER DESIGN ----------
% controller;

% Ktune(K_PID, G, H, p, 5, OSu)

% ---------- TESTING SOFT RAMP ----------
% testbench;
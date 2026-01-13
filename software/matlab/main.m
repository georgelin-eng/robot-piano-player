% clear clc;
addpath("scripts/")
motor_name = 'Maxon_ECi40';

Mhand = 0.5; % TEST MASS
CF = 200;
DC = 0.5;
WnRes = 10;
ZetaRes = 0.05;
TargPM = 60;

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm] = motor_params(motor_name);
plant_params;

n = 1/Rp;

% TODO: Need to estimate inertia of pulley belt and it's mass

Jj = Jm+Jp+1/n^2*Mhand; % Capcitance
Bj = Bm+Bp+1/n^2*Brail; % Resistance, 1/Rtot, Rtot_par = 1/(sum 1/B))
Kj = Kbelt;             % Inductance

Rtot = 1/Bj;
Ltot = 1/Kj;
Ctot = Jj;

% Resistor, Inductor, and Cap in parallel
Ztot = 1 / (1/Rtot + 1/(s*Ltot) + s*Ctot);

nKm = Km; % joint space speed is same as motor speed

Ye = 1/(s*Lw + Rw);
Ym = Ztot;
Gp = feedback(Ye*nKm*Ym, nKm) * 1/s;

Kjt = n;
Ktj = 1/Kjt;

Hs = sensor_params;

% ---------- CONTROLLER DESIGN ----------
controller;

Ktune(K_PID, G, H, p, 0.3, OSu)
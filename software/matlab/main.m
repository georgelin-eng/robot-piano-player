clear clc;
addpath("scripts/")
motor_name = 'Maxon_ECi40';

Mhand = 0.5; % TEST MASS

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm] = motor_params(motor_name);
plant_params;

n = 1/Rp;

% TODO: Need to estimate inertia of pulley belt and it's mass

Jj = Jm+Jp+1/n^2*Mhand; % Capcitance
Bj = Bm+Bp+1/n^2*Brail; % Resistance, 1/Rtot, Rtot_par = 1/(sum 1/B))
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

H = 1; % Assume perfect sensor for now



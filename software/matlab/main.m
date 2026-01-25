% clear clc;
addpath("scripts/")
motor_name = '60PG-997-4.25-EN';
% motor_name = '60PG-6055ZY';
% motor_name = 'Polulu37D_64CPR';
CF      = 80;   % Control freq [Hz]
DC      = 0.5;  % Duty cycle
WnRes   = 2;    % Frequency search res
ZetaRes = 0.05; % Damping factor search res
TargPM  = 60;   % Target phase margin [degrees]
Vmotor  = 24;   % DC voltage of motor

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm, n, Istall] = motor_params(motor_name);
% Rw = Rw * 0.8;
% Lw = Lw * 10;
plant_params;

n1 = n;         % gearbox
n2 = 1/Rpulley; % rotational to linear

% Summing components
J1 = Jm + Jgearbox + Jpulley/n1^2 + Jbelt/n1^2;
J2 = Mhand * 1/n1^2 * 1/n2^2;
B1 = Bm;
B2 = Brail * 1/n1^2 * 1/n2^2;
K1 = Kbelt * 1/n1^2 * 1/n2^2;

ZC1 = 1/(s*J1);
ZC2 = 1/(s*J2);
ZR1 = 1/B1;
ZR2 = 1/B2;
ZL1 = s * 1/K1;

% Zeq1 = RR(ZL1, ZR2) + ZC2;
Zeq = ZC2;
Ztot = (1/ZC1 + 1/ZR1 + 1/Zeq1)^-1;
Ztot = minreal(Ztot);

nKm = Km; % joint space speed is same as motor speed

Ye = 1/(s*Lw + Rw);
Ym = Ztot;
tau_spring = 10 * 1e-3; % Assume 10ms to propogate force through belt. 
spring_del = 1/tau_spring / (s + 1/tau_spring);
% Gp = feedback(Ye*nKm*spring_del*Ym, nKm) * 1/s;
Gp = feedback(Ye*nKm*Ym, nKm) * 1/s;

Kjt = 1/n2 * 1/n1;
Ktj = 1/Kjt;

Hs = sensor_params;

% ---------- CONTROLLER DESIGN ----------

controller;

K_out = Ktune(K_PID, G, H, p, 0.3, OSu);
save_PID(motor_name, K_out);

% ---------- TESTING SOFT RAMP ----------
% testbench;

% ENCODER PARAMETERS
CPR = 14; % counts per revolution
encoder_freq = 20 * 1e3;
T_encoder = 1/encoder_freq;

% load_system('simulink_model.slx')

data = out.simout.Data;
time = out.simout.Time;
plot(time, data);
yline(yd);

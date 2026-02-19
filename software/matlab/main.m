clear clc;
addpath("scripts/")
motor_name = 'Polulu37D_64CPR';
CF      = 80;   % Control freq [Hz]
DC      = 0.5;  % Duty cycle
WnRes   = 2;    % Frequency search res
ZetaRes = 0.05; % Damping factor search res
TargPM  = 60;   % Target phase margin [degrees]
Vmotor  = 12;   % DC voltage of motor

IGNORE_TIMING_BELT = true;

% --------- MECH SYSTEM MODEL ---------
[Rw, Lw, Km, Jm, Bm, n, Istall] = motor_params(motor_name);
plant_params;

n1 = n;         % gearbox

n2 = 1/Rpulley; % rotational to linear

% Summing components
J1 = Jm*1.1 * 0.7 + Jpulley/n1^2 + Jbelt/n1^2;
J2 = Mhand * 1/n1^2 * 1/n2^2;
B1 = Bm;
B2 = Brail * 1/n1^2 * 1/n2^2;
K1 = Kbelt * 1/n1^2 * 1/n2^2;

ZC1 = 1/(s*J1);
ZC2 = 1/(s*J2);
ZR1 = 1/B1;
ZR2 = 1/B2;
ZL1 = s * 1/K1;

switch IGNORE_TIMING_BELT
    case true
        Ztot = RR(RR(ZC1, ZC2), ZR1);
        Ztot = minreal(Ztot);  
    otherwise
        % Accounting for timing belt as spring+damper in parallel
        Zeq1 = RR(ZL1, ZR2) + ZC2;
        Ztot = (1/ZC1 + 1/ZR1 + 1/Zeq1)^-1;
        Ztot = minreal(Ztot);
end

nKm = Km; % joint space speed is same as motor speed

Ye = 1/(s*Lw + Rw);
Ym = Ztot;
Gp = feedback(Ye*nKm*Ym, nKm) * 1/s;

Kjt = 1/n2 * 1/n1;
Ktj = 1/Kjt;

Hs = sensor_params;

% ---------- CONTROLLER DESIGN ----------

controller;
K_out = Ktune(K_PID, G, H, p, 1.0, OSu);
save_PID(motor_name, K_out);

Kp = K_out.Kp;
Ki = K_out.Ki;
Kd = K_out.Kd;

% ---------- TESTING SOFT RAMP ----------
% testbench;

% ENCODER PARAMETERS
ramp_time = 0.1;
yd = 0.02;
CPR = 14; % counts per revolution
encoder_freq = 20 * 1e3;
T_encoder = 1/encoder_freq;

t = 0:1e-3:2;
u = soft_step(yd, ramp_time, t);
simin = timeseries(u, t);

% simulink_parse;
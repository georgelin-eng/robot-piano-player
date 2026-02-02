figure;
data = out.simout.Data;
time = out.simout.Time;
plot(time, data);

figure;
data = out.PWM.Data;
time = out.PWM.Time;
yline(yd);
plot(time, data);

figure;
data = out.iw.Data;
time = out.iw.Time;
plot(time, data);

% Torque calculation
figure;
Torque = abs(out.iw.Data * Km);
speed  = abs(out.w.Data);

plot(speed, Torque);

% Plot max speed
w_NL = 12000 / 60 * 2*pi;
T_stall = Km * Istall;

hold on;
w_vec = linspace(0, w_NL, length(speed));
T_vec = T_stall * (1 - w_vec / w_NL);
plot(w_vec, T_vec);


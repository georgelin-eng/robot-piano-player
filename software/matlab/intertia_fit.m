table = readtable('MotorStoppingTransient.xlsx');

hold on
t = table.("time");
w = table.("speed");

hold on;
scatter(t, w, 15);
J = 0.0000835064;
B = -0.032922;
c0 = 1760.25865;
A = 0.000045;

t = 0:1e-3:2.5;
y = B/A + c0 * exp(-A/J * t);
plot(t, y);
ylabel("speed [rad/s]");
xlabel("Time [s]");
title("Motor Stopping Transient");
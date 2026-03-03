figure;
hold on;
plot(out.simout.Time, out.simout.Data);
m = load('sim_results.mat');
plot(m.out.simout.Time, m.out.simout.Data);
legend('Current', 'Prev Tune');
save('sim_results.mat', 'out')
yline(yd);

figure;
data = out.w.Data;
time = out.w.Time;
plot(time, data);

figure;
data = out.iw.Data;
time = out.iw.Time;
plot(time, data);

figure;
data = out.PWM.Data;
time = out.PWM.Time;
plot(time, data);
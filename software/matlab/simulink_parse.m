figure;
data = out.simout.Data;
time = out.simout.Time;
yline(yd);
plot(time, data);

figure;
data = out.iw.Data;
time = out.iw.Time;
plot(time, data);


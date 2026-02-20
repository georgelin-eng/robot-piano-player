table = readtable('Feb19Response.xlsx');

hold on
t = table.("Time");
ya = table.("Measured");
% Normalize via Ktj and RAD/PULSE
ya = ya - ya(1);
ya = ya * Kjt / 2;
plot(t, ya);

m = load('sim_results.mat');
plot(m.out.simout.Time, m.out.simout.Data);
legend('Measured', 'Simulink Results');
xlabel('Time (s)');
ylabel('Response (m)');
title('Measured vs Simulink Results');
grid on;

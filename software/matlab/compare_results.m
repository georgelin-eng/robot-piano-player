table = readtable('Feb19Response.xlsx');
LWIDTH = 0.5;

hold on
t_max = 1.0;

t = table.("Time");
t_mask = t < t_max;
t = t(t_mask);
ya = table.("Measured");
ya = ya(t_mask);
y_lti = step(CLTF, t) * yd;
% Normalize via Ktj and RAD/PULSE
ya = ya - ya(1);
ya = ya * Kjt / 2;
plot(t, ya, LineWidth=LWIDTH);
plot(t, y_lti, LineWidth=LWIDTH);

m = load('sim_results.mat');
t = m.out.simout.Time;
t = t(t < t_max);
data = m.out.simout.Data;
data = data (t < t_max);

plot(t, data, Color='black', LineWidth=LWIDTH);

yd_line = repmat(yd, length(t), 1);
plot(t, yd_line, LineStyle='--')

legend('Measured', 'Linear', 'Linear + Non-linearity Results', 'Set value', Location='southeast');
xlabel('Time (s)');
ylabel('Response (m)');
title('Measured vs LTI vs Linear + Nonlinearity Simulation');
grid on;


pbaspect([1 1 1])
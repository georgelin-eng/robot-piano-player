% ----------------- [0:1] OCTAVES -----------------

yd_targets = 0.02:0.01:0.12;
ramp_time = 0.15; 

figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, iw_CLTF)
xlabel('Time (s)');
ylabel('Motor Current(A)');
title ('Current at startup: [0:1] octave');
legend('Location','bestoutside');


figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, w_CLTF)
xlabel('Time (s)');
ylabel('speed(rad/s)');
title ('Speed motor at startup');
legend('Location','bestoutside');

% figure;
figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF)
xlabel('Time (s)');
ylabel('rad');
title ('Position vs time for [0:1] octave');
legend('Location','bestoutside');

% ----------------- [1:2] OCTAVES -----------------
yd_targets = 0.12:0.02:0.28;
ramp_time = 0.25; % must use longer ramp to limit inrush

figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, iw_CLTF)
xlabel('Time (s)');
ylabel('Motor Current(A)');
title ('Current at startup: [1:2] octave');
legend('Location','bestoutside');

figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, w_CLTF)
xlabel('Time (s)');
ylabel('speed(rad/s)');
title ('Speed motor at startup');
legend('Location','bestoutside');


figure; hold on; grid on;
lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF)
xlabel('Time (s)');
ylabel('rad');
title ('Position vs time for [1:2] octave');
legend('Location','bestoutside');

% lsim(iw_CLTF, u, t);
% lsim(PWM_CLTF, u, t);
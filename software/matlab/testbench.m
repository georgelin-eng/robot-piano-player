% ----------------- 1-6 cm-----------------
yd_targets = 0.01:0.01:0.06;
ramp_time = 0.15; 
IS_POSITION = 1;

lsim_CLTF(yd_targets, Ktj, ramp_time, iw_CLTF)
ylabel('Motor Current(A)');
title ('Current at startup: 1-6 cm');

lsim_CLTF(yd_targets, Ktj, ramp_time, w_CLTF)
ylabel('speed(rad/s)');
title ('Speed motor at startup: 1-6');


lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF, IS_POSITION)
ylabel('position (m)');
title ('Position vs time: 1-6 cm');

% ----------------- 7 to 13cm -----------------
yd_targets = 0.07:0.01:0.13;
ramp_time = 0.35; 

lsim_CLTF(yd_targets, Ktj, ramp_time, iw_CLTF)
ylabel('Motor Current(A)');
title ('Current at startup: 7-12cm');

lsim_CLTF(yd_targets, Ktj, ramp_time, w_CLTF)
ylabel('speed(rad/s)');
title ('Speed motor at startup: 7-12cm');

lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF, IS_POSITION)
ylabel('position (m)');
title ('Position vs time for 7-12cm');


% ----------------- 14-28cm -----------------
yd_targets = 0.14:0.02:0.28;
ramp_time = 0.60; % must use longer ramp to limit inrush

lsim_CLTF(yd_targets, Ktj, ramp_time, iw_CLTF)
ylabel('Motor Current(A)');
title ('Current at startup: 14-28cm');

lsim_CLTF(yd_targets, Ktj, ramp_time, w_CLTF)
ylabel('speed(rad/s)');
title ('Speed motor at startup: 14-28cm');


lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF, IS_POSITION)
ylabel('position (m)');
title ('Position vs time: 14-28cm');

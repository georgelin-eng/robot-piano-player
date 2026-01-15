% Modelling 512–6400 CPT, quadrature encoder
% 2 channels, pulses are out of phase and we see edge events every 90 deg
% Effective pulses / second is freq * 4
function [Hs] = sensor_params()
    s = tf('s');
    
    freq = 500 * 1e3; % 500KHz
    tau_sensor = 1 / (freq * 4);
    
    f = 1/tau_sensor;
    Hs = f / (s + f);
end

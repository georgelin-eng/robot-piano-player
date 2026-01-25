function u = soft_step(yd, ramp_time, t)

    % S-curve 
    a = 2*pi / ramp_time;
    u = yd / (2*pi) * (a*t - sin(a*t));
    u = min(yd, u);

end

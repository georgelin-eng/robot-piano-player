function lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF)
    t = 0:1e-3:0.5;

    for yd = yd_targets
        yd_eff = yd * Ktj;              % scaled setpoint
        % slope  = yd_eff / ramp_time;
    
        % S-curve 
        a = 2*pi / ramp_time;
        u = yd_eff / (2*pi) * (a*t - sin(a*t));
        u = min(yd_eff, u);
    
        % Linear ramp
        % u = min(yd_eff, slope * t);     % ramp with saturation
        
        y = lsim(CLTF, u, t);
        plot(t, y, 'DisplayName', sprintf('yd = %.2f', yd));
    end
end
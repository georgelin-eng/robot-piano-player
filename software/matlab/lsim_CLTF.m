function lsim_CLTF(yd_targets, Ktj, ramp_time, CLTF, IS_POSITION)
    t = 0:2e-4:1.0;
    figure; hold on; grid on;
    TOLERANCE = 2 * 1e-3; % 2mm settling in each direction.

    for yd = yd_targets
        yd_eff = yd * Ktj;              % scaled setpoint

        % S-curve 
        a = 2*pi / ramp_time;
        u = yd_eff / (2*pi) * (a*t - sin(a*t));
        u = min(yd_eff, u);
    
        y = lsim(CLTF, u, t);
        if (nargin == 5)
            if (IS_POSITION == 1)
                hplot = plot(t, y * 1/Ktj, 'DisplayName', sprintf('yd = %.2f', yd));
                
                % Get the color of the plot line
                lineColor = hplot.Color;
                yline(yd + TOLERANCE, 'Color', lineColor, 'LineStyle', ':', 'HandleVisibility', 'off');
                yline(yd - TOLERANCE, 'Color', lineColor, 'LineStyle', ':', 'HandleVisibility', 'off');
            end
        else
            plot(t, y, 'DisplayName', sprintf('yd = %.2f', yd));
        end
    end
    xlabel('Time (s)');
    legend('Location','bestoutside');
    
end
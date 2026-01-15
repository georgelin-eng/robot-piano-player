function [zeta, wn] = PID_sweep(OLTF, p)
    %   PID_SWEEP Summary of this function goes here
    %   If phase margin starts at 180 degrees when adding in partial dynamics
    %   it's impossible to get cross over sweep of partial dynamics
    %   Need to do a coarse sweep
    s = tf('s');

    % Sweep ranges
    zeta_vec = linspace(0.1, 1.0, 50);
    wn_vec   = linspace(5, 5000, 30);

    % Preallocate result
    PM = NaN(length(zeta_vec), length(wn_vec));
    for i = 1:length(zeta_vec)
        for j = 1:length(wn_vec)

            zeta = zeta_vec(i);
            wn   = wn_vec(j);

            Num = s^2 + 2*zeta*wn*s + wn^2;
            D = -p/wn^2 * Num / (s*(s - p));

            % Margins
            try
                [~, Pm] = margin(D * OLTF);
                PM(i,j) = Pm;
            catch
                PM(i,j) = NaN;
            end
        end
    end

    % Plot surface
    figure
    surf(wn_vec, zeta_vec, PM, 'EdgeColor','none')
    xlabel('\omega_n (rad/s)')
    ylabel('\zeta')
    zlabel('Phase Margin (deg)')
    title('Phase Margin vs \zeta and \omega_n')
    colorbar
    view(45,30)
end
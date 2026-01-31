
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Ktune()
% GUI for fast PID Tuning.
% IMPORTANT: To adjust slider range, edit the code below
%
% Syntax:
%   Ktune(K, G, H, p, Ts, OSu)
%
% Example Usage:
%   Ktune(Q13, Q9.G, Q9.H, p,  0.7*Q14.Ts,  0.4*Q14.OSu)
% 
% Input:
%   K   (struct)    Struct containing old Kp, Ki, Kd to be tuned. 
%   G               Forward path gain
%   H               Feedback path gain
%   p               Derivative pole
%   Ts              Rise time target
%   OSu             Overshoot target
%
% GUI elements:
%   Blue line       - Always stays at 1
%   Yellow lines    - Represents +/- 2% error margin from 1.
%   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function K_out = Ktune(K, G, H, p, Ts, OSu)
    % main gui config
    fig = uifigure('Name', 'Ktune', 'Position', [100, 100, 800, 900]);
    ax = uiaxes(fig, 'Position', [50, 300, 600, 400]);
    ax.Title.String = 'CLTF Step Response';
    ax.XLabel.String = 'Time (s)';
    ax.YLabel.String = 'Response';
        
    % NOTE: IF THE SLIDER RANGE ISNT ENOUGH,
    % CHANGE [0,2] ------------------------------------------------------
    %                                                                   |
    % TO YOUR DESIRED RANGE                                             \/
    %
    
    % K0 SLIDER CONFIG
    sliderK0 = uislider(fig, 'Position', [100, 300, 600, 4], 'Limits', [0.1, 3], 'Value', 1);
    labelK0  = uilabel(fig, 'Position', [100, 310, 200, 20], 'Text', 'K0 Multiplier: 1');

    % Kp SLIDER CONFIG
    sliderKp = uislider(fig, 'Position', [100, 220, 600, 4], 'Limits', [0, 2], 'Value', 1);
    labelKp  = uilabel(fig, 'Position', [100, 230, 300, 20], 'Text', 'Kp Multiplier: 1');
    
    % Ki SLIDER CONFIG
    sliderKi = uislider(fig, 'Position', [100, 140, 600, 4], 'Limits', [0, 2], 'Value', 1);
    labelKi  = uilabel(fig, 'Position', [100, 150, 200, 20], 'Text', 'Ki Multiplier: 1');
    
    % Kd SLIDER CONFIG
    sliderKd = uislider(fig, 'Position', [100, 60, 600, 4], 'Limits', [0, 2], 'Value', 1);
    labelKd  = uilabel(fig, 'Position', [100, 70, 200, 20], 'Text', 'Kd Multiplier: 1');

    % init plot
    updatePlot();

    % slider call back funcs
    sliderK0.ValueChangedFcn = @(src, event) updatePlot();
    sliderKp.ValueChangedFcn = @(src, event) updatePlot();
    sliderKi.ValueChangedFcn = @(src, event) updatePlot();
    sliderKd.ValueChangedFcn = @(src, event) updatePlot();

    % CloseRequestFcn: resume execution when user closes
    fig.CloseRequestFcn = @(src,event) uiresume(fig);

    % Wait until GUI closes
    uiwait(fig);
    onCloseGUI();
    delete(fig);

    % plotting func
    function updatePlot()
        t = 0:1e-3:Ts*2;
        K0Multiplier = sliderK0.Value;
        KpMultiplier = sliderKp.Value*K0Multiplier;
        KiMultiplier = sliderKi.Value*K0Multiplier;
        KdMultiplier = sliderKd.Value*K0Multiplier;

        labelK0.Text = sprintf('K0 Multiplier: %.4f', K0Multiplier);
        labelKp.Text = sprintf('Kp Multiplier: %.4f', KpMultiplier);
        labelKi.Text = sprintf('Ki Multiplier: %.4f', KiMultiplier);
        labelKd.Text = sprintf('Kd Multiplier: %.4f', KdMultiplier);

        s = tf('s');
        D = (K.Kp * KpMultiplier) + (K.Ki * KiMultiplier / s) + (K.Kd * KdMultiplier * -p * s / (s - p));
        cltf = G * D / (1 + G * H * D);

        [y, t] = step(cltf, t);

        % plot
        cla(ax); % clear axes
        plot(ax, t, y, 'k-', 'LineWidth', 1.5);
        hold(ax, 'on');
        plot(ax, xlim(ax), [1, 1], 'Color', [0.3010 0.7450 0.9330], 'LineStyle', '-', 'LineWidth', 1.1);
        plot(ax, xlim(ax), [1.15, 1.15], 'Color', [0.9290 0.6940 0.1250], 'LineStyle', '-', 'LineWidth', 1.1);
        plot(ax, xlim(ax), [0.85, 0.85], 'Color', [0.9290 0.6940 0.1250], 'LineStyle', '-', 'LineWidth', 1.1);
        plot(ax, xlim(ax), [(OSu / 100 + 1), (OSu / 100 + 1)], 'r--', 'LineWidth', 1.5);
        plot(ax, [Ts, Ts], ylim(ax), 'r--', 'LineWidth', 1.5);
        hold(ax, 'off');

        % auto axis adjust
        xLimits = [min(t), max(t)];
        yLimits = [min(y), max(y)];
        ylim(ax, [yLimits(1) - 0.05 * abs(yLimits(1)), yLimits(2) + 0.05 * abs(yLimits(2))]);
        xlim(ax, xLimits);

        grid(ax, 'on');
    end

    function onCloseGUI()
        % Read current slider values
        K0Multiplier = sliderK0.Value;
        KpMultiplier = sliderKp.Value * K0Multiplier;
        KiMultiplier = sliderKi.Value * K0Multiplier;
        KdMultiplier = sliderKd.Value * K0Multiplier;
    
        % Build the final PID struct
        K_out.Kp = K.Kp * KpMultiplier;
        K_out.Ki = K.Ki * KiMultiplier;
        K_out.Kd = K.Kd * KdMultiplier;        
    end
end


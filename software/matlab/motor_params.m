function [Rw, Lw, Km, Jm, Bm, n, Istall] = motor_params(motor_name)
    % MOTOR_PARAMS Returns motor parameters for a given motor name.
    %
    % INPUTS:
    %   motor_name : string 
    %       Supported
    %           Maxon_EC90
    %           Maxon_RE65 
    %           Maxon_RE50
    %           Maxon_ECi40
    %
    % OUTPUTS:
    %   Rw  : winding resistance [Ohm]
    %   Lw  : winding inductance [H]
    %   Km  : torque constant    [Nm/A]
    %   Jm  : rotor inertia      [kg*m^2]
    %   Bm  : viscous friction   [N*m*s/rad]

    motor_table = readtable('motors.xlsx');
    row = getMotorRow(motor_name, motor_table);
    GEAR_BOX_LOSS = 0.9;
    
    Rw = row.("Rw_Ohm_");
    Lw = row.("Lw_mH_") * 1e-3;
    Km = row.("Km_mNm_A_") * 1e-3;
    Jm = row.("Jm_gcm_2_") * 1e-3 * 1e-4;
    rpm_NL = row.("rpm_NL_rpm_");
    I_NL = row.("I_NL_mA_") * 1e-3;
    gear_box_ratio = row.("GearboxRatio");
    Istall = row.("StallCurrent_A_");

    if (gear_box_ratio ~= 1)
        Km = Km * GEAR_BOX_LOSS;
    end

    % Friction is calculated from no load current no load speed
    w_NL = rpm_NL / 60 * 2*pi;
    Bm   = Km * I_NL / w_NL;
    n = gear_box_ratio;
end

% ---------------- HELPER FUNCTIONS ----------------

function motor_row = getMotorRow(motor_name, motor_table)
    % Find the row where Motor column matches the given name
    idx = strcmp(motor_table.Motor, motor_name);
    
    if any(idx)
        motor_row = motor_table(idx, :);  % returns a table with that row
    else
        error('Motor "%s" not found.', motor_name);
    end
end
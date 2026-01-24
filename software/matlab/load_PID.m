function [K_PID] = load_PID(motor_name, K_PID)
    filename = sprintf('PID_vals/%s_PID.mat', motor_name);
    
    % return code: 
    % https://www.mathworks.com/help/matlab/ref/exist.html
    FILE_W_EXTENSION = 2; 

    if exist(filename, 'file') == FILE_W_EXTENSION 
        data = load(filename, 'K_PID');  % loads variable K_PID from file
        K_PID.Kp = data.K_PID.Kp;       
        K_PID.Ki = data.K_PID.Ki;       
        K_PID.Kd = data.K_PID.Kd;       
        fprintf('Loaded PID gains from %s\n', filename);
    end
end
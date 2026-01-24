function save_PID(motor_name, K_PID)
    % Construct filename
    filename = sprintf('PID_vals/%s_PID.mat', motor_name);

    % Save the struct to the file
    save(filename, 'K_PID');

    fprintf('PID gains saved to %s\n', filename);
end
function [K] = K_for_PM(K0, OLTF, target_PM, print)
    % OLTF = K0 * D * Gp * H
    threshold = 0.01;
    K_step = K0;
   
    if(nargin < 4)
        print = "OFF";
    end

    K = K0;
    open_loop = K * OLTF;
    [~,Pm] = margin(open_loop);
    
    if (Pm > target_PM)
        dir = 1;
    else
        dir = -1;
    end


    while ( abs(Pm - target_PM) > threshold ) 
        prv_dir = dir;
        open_loop = K * OLTF ;
        [~,Pm] = margin(open_loop);
        K = K + dir * K_step;

        if (Pm > target_PM)
            dir = 1;
        else
            dir = -1;
        end

        if (prv_dir ~= dir)
            K_step = K_step /2;
        end
        
        if (print~="OFF")
            fprintf("K=%0.5f, Pm = %0.5f\n", K, Pm)
        end
    end
end
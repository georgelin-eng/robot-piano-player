function [wz] = newton(OLTF, wxo, wn_res, TYPE)
    % OLTF = K0 * Dp * Gp * H
    % Optimizing for phase margin
    MAX_ITER = 100;
    iter_cnt = 0;
    wz = wxo;
    s = tf('s');
    previous_Pm = nan;
    previous_dir = nan;

    Pm_hist = [];
    wz_hist = [];

    assert(wxo > 0, "Expecting positive cross-over freq")
    
    % wz > 0
    while (iter_cnt < MAX_ITER) 
        if (TYPE == "PD")
            Dz = (s + wz)/wz;
        elseif (TYPE =="PI")
            Dz = (s + wz)/wz;
        else
            error("Expecting TYPE = PI or TYPE=PD")
        end
        
        open_loop = Dz * OLTF ;
        [~,Pm] = margin(open_loop);

        % if (Pm < previous_Pm)
        %     dir = dir*-1; 
        % end
        % 
        % fprintf("Pm = %0.5f @ iter %0d", Pm, iter_cnt)
       
        % if (previous_dir ~= dir && iter_cnt > 2)
        %     fprintf("Pm = %0.5f @ iter %0d", Pm, iter_cnt)
        %     break
        % end
        
        % increasing wz may improve PM
        wz = wz + wn_res;
        previous_Pm = Pm;
        previous_dir = dir;
        iter_cnt = iter_cnt+1;

        Pm_hist = [Pm_hist, Pm];
        wz_hist = [wz_hist, wz];
    end
    
    plot(wz_hist, Pm_hist)
    [~, idx] = max(Pm_hist);
    wz = wz_hist(idx);
end
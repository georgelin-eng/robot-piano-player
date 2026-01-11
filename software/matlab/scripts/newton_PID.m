function [wz] = newton_PID(OLTF, wxo, zeta_res, wn_res)
    % OLTF = K0 * Dp * Gp * H
    % Optimizing for phase margin

    wn = wxo;
    s = tf('s');

    assert(wxo > 0, "Expecting positive cross-over freq")
    
    % Sweep zeta 1 -> 0 (complex poles)
    fprintf("Running Newton PID");
    best_Pm = -inf;
    zeta = 1;
    for zeta = 0:zeta_res:10*zeta
        Dz = get_Dz(zeta, wn);
    
        open_loop = Dz * OLTF ;
        [~,Pm] = margin(open_loop);
          
        if (Pm > best_Pm)
            best_zeta = zeta;
            best_Pm = Pm;
        end

        fprintf("zeta = %0.5f, Pm = %0.5f\n", zeta, Pm)
    end
    fprintf("BEST: zeta = %0.5f, wn = %0.5f, Pm = %0.5f\n", best_zeta, wn, best_Pm)

    zeta = best_zeta;
        % Get best zeta and sweep wn around wxo in range +/- wn_res*MAX_ITER
    best_Pm = -inf;
    for wn = wxo:wn_res:10*wxo
        Dz = get_Dz(zeta, wn);
        
        open_loop = Dz * OLTF ;
        [~,Pm] = margin(open_loop);

        fprintf("wn = %0.5f, Pm = %0.5f\n", wn, Pm)

        if (Pm > best_Pm)
            best_wn = wn;
            best_Pm = Pm;
        end

    end
    wn = best_wn;
    fprintf("BEST: zeta = %0.5f, wn = %0.5f, Pm = %0.5f\n", zeta, wn, best_Pm)
    
    wz = transpose(zero(s^2 + 2*zeta*wn*s + wn^2));
end

function [Dz_norm] = get_Dz(zeta, wn)
    s = tf('s');
    Dz = s^2 + 2*zeta*wn*s + wn^2;
    z = zero(Dz);
    Dz_norm = Dz / (z(1)*z(2));
end
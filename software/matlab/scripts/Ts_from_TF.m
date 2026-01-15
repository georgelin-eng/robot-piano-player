function [Ts] = Ts_from_TF(CLTF)
    [y,t] = step(CLTF);
    y = y/dcgain(CLTF);

    L = [0.98 1.02];
    
    cross = ...
        (y(1:end-1)-L(1)).*(y(2:end)-L(1)) <= 0 | ...
        (y(1:end-1)-L(2)).*(y(2:end)-L(2)) <= 0;
    
    Ts = t(find(cross,1,'last'));

    % 
    % If the peak value < 1.02 then do this
    if (isempty(Ts)) 
        mask = (y>dcgain(CLTF));
        [~, idx] = max(y);
    
        OS = max(y) - 1.0;
        zeta = OS_to_zeta(OS);
        beta = sqrt(1-zeta^2);
    
        % Estimate wns via Tp and Tr to calculate Ts via O2 appx
        Tp = t(idx);
        Tr = min(t(mask));
        theta = pi - atan(beta/zeta);
    
        wnr = theta / (beta * Tr);
        wnp = pi / (beta * Tp);
    
        wns = (wnr+wnp) / 2;
        Ts = 4 / (zeta * wns);
    end
end
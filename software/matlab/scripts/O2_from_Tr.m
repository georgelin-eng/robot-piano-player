function [Ga] = O2_from_Tr(FV, peak, Tr)
    s = tf('s');

    OS = (peak - FV)/FV;
    zeta = OS_to_zeta(OS);
    
    beta = sqrt(1-zeta^2);
    theta = pi - atan(beta/zeta);
    
    wn = theta / (beta * Tr);

    Ga = FV * wn^2 / (s^2 + 2*zeta*wn*s + wn^2);
end
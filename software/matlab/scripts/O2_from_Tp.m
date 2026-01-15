function [Ga] = O2_from_Tp(FV, peak, Tp)
    s = tf('s');

    OS = (peak - FV)/FV;
    zeta = OS_to_zeta(OS);
    
    beta = sqrt(1-zeta^2);

    wn = pi / (beta*Tp);

    Ga = FV * wn^2 / (s^2 + 2*zeta*wn*s + wn^2);
end
function  [Ga] = O2_from_Ts(FV, peak, Ts)
    s = tf('s');

    OS = (peak - FV)/FV;
    zeta = OS_to_zeta(OS);
    
    wn = 4 / (zeta*Ts);

    Ga = FV * wn^2 / (s^2 + 2*zeta*wn*s + wn^2);

end
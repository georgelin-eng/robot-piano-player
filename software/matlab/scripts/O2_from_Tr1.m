function  [Ga] = O2_from_Tr1(FV, Tr1, Te)
    s = tf('s');

    if (Tr1/Te < 2) % O2 approx
        zeta = Te / (3.86*Te - 1.83 * Tr1);
        wn = 2.1*zeta / Te;
        
        Ga = FV * wn^2 / (s^2 + 2*zeta*wn*s + wn^2);
    else %1st order approx is better
        wn = 1/Te;
        Ga = FV * wn / (s+wn);
    end


end
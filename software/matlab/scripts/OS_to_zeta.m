function zeta = OS_to_zeta(OS)
    ln_OS2 = log(OS)^2;
    zeta = sqrt(ln_OS2 / (pi^2 + ln_OS2));
end
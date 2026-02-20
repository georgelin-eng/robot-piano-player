H = Hs * Hc;
G = Ga * Gp;

p = pole(Hc);

Dp = 1/s * CF / (N * s + CF);

pzmap(Dp * G);

z1 = -21;
z2 = -21;

D = Dp * (s - z1) * (s-z2);
K0 = 0.0025 * 1/10;

[K0, ~] = margin(feedback(K0*G*D, H));


% 10 to 100 for freq
wn_vec = logspace(1, log10(500), 20);
margin(K0 * D * G * H);

zeta_vec = ZetaRes:ZetaRes:1;
% zeta from 1 to 10
zeta_vec = [zeta_vec linspace(1, 10, 20)];

Pm_mat = zeros(length(zeta_vec), length(wn_vec));

[WN_VEC, ZETA_VEC] = meshgrid(wn_vec, zeta_vec);
for i = 1:length(zeta_vec)
    for j = 1:length(wn_vec)
        zeta = zeta_vec(i);
        wn = wn_vec(j);

        z = zero(s^2 + 2*zeta*wn*s + wn^2);
        D = Dp * (s - z(1)) * (s-z(2));
        [Gm, Pm] = margin(K0 * D * G * H);

        Pm_mat(i,j) = Pm;
    end
end

surf(WN_VEC, ZETA_VEC, Pm_mat)
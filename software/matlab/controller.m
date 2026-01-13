CF = 1000;
dt = 1/CF;
Ka = 100; % Perfect gain % TODO: How to model this???

tau_Ga = 1e-3; % 1ms;

Ga = O2_from_Ts(1, 1.05, tau_Ga);
GHs = Hs * Ga * Gp;

p = pole(minreal(GHs));
tol = 1e-6;                         % numerical tolerance
p_nz = p(abs(real(p)) > tol);       % exclude (near) zero real-part poles
wd = -max(real(p_nz));
Nf = CF/(ceil(wd)*10) - 0.5 - DC;
beta = Nf/(1+Nf); 
tau = -dt / log(beta);

Nc = DC;
Nh = 0.5;

N = Nc + Nh + Nf; 
Hc = CF / (N*s + CF); % scale by sensor gain

H = Hs * Hc;
G = 1 * Gp;

GH = G*H;
p = pole(Hc);

% ---------- CONTROLLER DESIGN ----------



% Dp = 1/s * CF / (N * s + CF);
% OLTF = Gp * Dp * H;
% [Gm, Pm, wxo, ~] = margin(OLTF);
% K0 = Gm;
% 
% WnRes = 0.1 * 10^(round(log10(wxo))-1);
% 
% [PID.Z, PID.PM, PID.D] = MaxPM(wxo, Dp, K0, G, H, WnRes, ZetaRes, 2);
% 
% Z1 = PID.Z(1);
% Z2 = PID.Z(2);
% 
% Kp = 1/p - (Z1+Z2) / (Z1*Z2);
% Kd = 1/p * Kp + 1/(Z1*Z2);
% 
% K0 = K_for_PM(K0, OLTF, 60);
% 
% 
% K_PID.Kp = Kp * K0;
% K_PID.Ki =   1* K0;
% K_PID.Kd = Kd * K0;

% K0 = 1.246;
% K_PID.Kp = 9.4871e+04 * K0;
% K_PID.Ki = 2.7513e+07 * K0*1.3;
% K_PID.Kd = 3.5400e+05 * K0*0.0573;

% ITERATION 1
% K_PID.Kp = 1.1821e+05;
% K_PID.Ki = 4.4566e+07;
% K_PID.Kd = 2.5274e+05;

% ITERATION 2
% K_PID.Kp = 9.4570e+06;
% K_PID.Ki = 1.1849e+08;
% K_PID.Kd = 2.1791e+05;

% ITERATION 3, for CF = 200
% TODO: These are way too high...
% Reason being Kbelt is a way diff. order of magnitude
% need to have zeros that come from RLC to cancel 1/s term (w -> rad)
K_PID.Kp = 9.6103e+06;
K_PID.Ki = 1.0254e+08;
K_PID.Kd = 4.4522e+04;

% K = 1.0510;
% K_PID.Kp = K_PID.Kp * K *0.9669;
% K_PID.Ki = K_PID.Ki * K *0.8234;
% K_PID.Kd = K_PID.Kd * K *0.1944;

D = (K_PID.Kp) + (K_PID.Ki  / s) + (K_PID.Kd * -p * s / (s - p));
CLTF = G * D / (1 + G * H * D);

% [Tr, Tp, Ts, OSu] = RCG_from_CLTF(CLTF);

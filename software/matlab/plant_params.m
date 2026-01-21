s = tf('s');

% ----------- PLANT PARAMS ------------------
% Pulley = 3764N137
% 9mm belt vs 6mm is 50% stiffer due to cross section
% Using modulus of fibreglass
belt_width = 9 * 1e-3; % 
A = 1.49*1e-3 *belt_width;  % Cross section  [m^2]
Y = 72 * 1e9;               % Young's modulus of fibreglass [Pa]
L = 537 * 1e-3;             % Length of pulling belt [m]

Rpulley = 34.4/2 * 1e-3;            % Radius of pulley
Kbelt = Y*A/L;                      % Spring constant of belt in middle
Bp = 1e-4;                          % Damping of belt with pulley (guess)
Brail = 1e-4;                       % Damping of platform moving on rails (guess)
Jpulley = 5419.195 * 1e-3 * 1e-6;   % Lzz of using Alumimum for mass estimation
Jbelt = 1256     * 1e-3 * 1e-6;

Rs = 11.5 * 1e-3;
Rp = 10   * 1e-3;
Js = 1864.14773 * 1e-3 * 1e-6;
Jp = 1001.016 * 1e-3 * 1e-6;
Jc = 4826.18637  * 1e-3 * 1e-6;
Np = 3;
mp = 18.73111 * 1e-3;

Jgearbox = Js + Np * Rs^2/4 * (1/Rp^2*Jp + mp) + (Rs/(2*(Rs+Rp)))^2*Jc;
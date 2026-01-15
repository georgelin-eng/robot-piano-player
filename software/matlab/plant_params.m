s = tf('s');

% ----------- PLANT PARAMS ------------------
% Pulley = 3764N137
% 9mm belt vs 6mm is 50% stiffer due to cross section
% Using modulus of fibreglass
belt_width = 9 * 1e-3; % 
A = 1.49*1e-3 *belt_width;  % Cross section  [m^2]
Y = 72 * 1e9;               % Young's modulus of fibreglass [Pa]
L = 537 * 1e-3;             % Length of pulling belt [m]

Rp = 34.4/2 * 1e-3;             % Radius of pulley
Kbelt = Y*A/L;                  % Spring constant of belt in middle
Bp = 1e-4;                      % Damping of belt with pulley (guess)
Brail = 1e-4;                   % Damping of platform moving on rails (guess)
Jp    = 5419.195 * 1e-3 * 1e-6; % Lzz of using Alumimum for mass estimation
Jbelt = 1256     * 1e-3 * 1e-6;

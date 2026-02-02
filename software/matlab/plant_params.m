s = tf('s');

% ----------- PLANT PARAMS ------------------
% Pulley = 3764N137
% 9mm belt vs 6mm is 50% stiffer due to cross section
% Using modulus of fibreglass
belt_width = 9 * 1e-3; % 
A = 1.49*1e-3 *belt_width;  % Cross section  [m^2]
Y = 72 * 1e9;               % Young's modulus of fibreglass [N/m^2]
L = 537 * 1e-3;             % Length of pulling belt [m]
L1 = L/2;
L2 = L/4;

Dpulley = 19.4;                       % Diameter of pulley (mm)
Rpulley = Dpulley/2 * 1e-3;         % Radius of pulley (m)
Kbelt = RR(Y*A/L1, Y*A/L2);         % Spring constant of belt in middle
Bp = 1e-4;                          % Damping of belt with pulley (guess)
Brail = 1e-4;                       % Damping of platform moving on rails (guess)
Jpulley = 5419.195 * 1e-3 * 1e-6;   % Lzz of using Alumimum for mass estimation
Jbelt = 1256     * 1e-3 * 1e-6;

% Calculating effective inertia of a planetary gearbox
% http://bordersengineering.com/tech_ref/planetary/planetary_analysis.pdf
Rs = 11.5 * 1e-3;
Rp = 10   * 1e-3;
Js = 1864.14773 * 1e-3 * 1e-6;
Jp = 1001.016 * 1e-3 * 1e-6;
Jc = 4826.18637  * 1e-3 * 1e-6;
Np = 3;
mp = 18.73111 * 1e-3;

Jgearbox = Js + Np * Rs^2/4 * (1/Rp^2*Jp + mp) + (Rs/(2*(Rs+Rp)))^2*Jc;

% Calculation of test mass
Mbolt = 47;
Mwire = 43.5000;
Macrylic = 4;
Msolenoid = Mbolt + Mwire + Macrylic;
SAFTEY_FACTOR = 1.0;

Mhand = Msolenoid * 9 * 1e-3 * SAFTEY_FACTOR;
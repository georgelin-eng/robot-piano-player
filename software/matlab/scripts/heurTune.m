%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% heurTune()
% Heuristic tuning of PID controller gains
%
% Syntax:
%   [CLTF OLTF] = heurTune(order, tMax, G, H, GAIN, p)
% 
% Input:
%   order :  string of 4 characters - K, P, I & D
%            gains adjusted in the specifiec order
%            eg. 'KPID', 'DIPK', etc.
%
%   tMax  :  max time to simulate
%
%   G     :  fwd path
%   H     :  fb path
%
%   GAIN   :  struct
%     O    :  struct of ORIGINAL gains
%       K  : master gain
%       Kp : prop gain
%       Ki : integ gain
%       Kd : deriv gain
%     N    :  struct of NEW gains
%       K  : master gain
%       Kp : prop gain
%       Ki : integ gain
%       Kd : deriv gain
%
%   p    :  derivative pole (optional)
%           Kd = 0 if missing
% Output:
%   CLTF :  vector of closed-loop xfer funcs
%   OLTF :  vector of open-loop xfer funcs
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [CLTF OLTF] = heurTune(order, tMax, G, H, GAIN, varargin)

  % Params
  if nargin == 5
    [CLTF OLTF] = heurTuneHelper(order, tMax, G, H, GAIN);
  elseif nargin == 6
    [CLTF OLTF] = heurTuneHelper(order, tMax, G, H, GAIN, varargin{1});
  else
    display('Wrong number of input parameters.');
  end

end % function

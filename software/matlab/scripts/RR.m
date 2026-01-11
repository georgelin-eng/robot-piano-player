% function Zeq = RR(Z)
%     Ytot = 0;
%     for n = 1 : length(Z)
%         Ytot = Ytot + 1/Z(n);
%     end
% 
%     Zeq = 1/Ytot;
% end

function Zeq = RR(Z1,Z2)
    Zeq = (Z1 * Z2)/(Z1+Z2);
end
function [Z_max, PM_max, D_max] = MaxPM(wxo, Dp, K0, G, H, WnRes, ZetaRes, numZeros)
    search = 1; % if search = 1, keep searching
    st = tf('s');
    
    % initial values
    z1_max = 0; z2_max = 0;
    wn_max = round(wxo,0);
    zeta_max = 1;
    if numZeros == 2
        r = roots([1 2*zeta_max*wn_max wn_max^2]);
        z1 = r(1); z2 = r(2);
        D = (st-z1)*(st-z2)/(z1*z2)*Dp;

        [~,PM_max] = margin(K0*D*G*H);

        % newtonian search
        while search == 1
            wn_current = wn_max;
            zeta_current = zeta_max;
            search = 0;
        
            % right point (increase wn)
            wn = wn_current + WnRes;
            zeta = zeta_current;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % left point (decrease wn)
            wn = wn_current - WnRes;
            zeta = zeta_current;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % top point (increase zeta)
            wn = wn_current;
            zeta = zeta_current + ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % bottom point (decrease zeta)
            wn = wn_current;
            zeta = zeta_current - ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
            
            % top left point (increase zeta, decrease wn)
            wn = wn_current - WnRes;
            zeta = zeta_current + ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % top right point (increase zeta, increase wn)
            wn = wn_current + WnRes;
            zeta = zeta_current + ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % bottom left point (decrease zeta, decrease wn)
            wn = wn_current - WnRes;
            zeta = zeta_current - ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        
            % bottom right point (decrease zeta, increase wn)
            wn = wn_current + WnRes;
            zeta = zeta_current - ZetaRes;
            r = roots([1 2*zeta*wn wn^2]);
            z1 = r(1); z2 = r(2);
            D = (st-z1)*(st-z2)/(z1*z2)*Dp;
            [~,PM] = margin(K0*D*G*H);
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; z2_max = z2;
                wn_max = wn; zeta_max = zeta;
                search = 1;
            end
        end
        
        Z_max = [z1_max z2_max];
        
        D_max = Dp * (st - z1_max) * (st - z2_max) / (z1_max * z2_max);
        [num,den] = tfdata(D_max, 'v');
        D_max = tf(real(num), real(den));
    end 
    if numZeros == 1
        z1 = -wn_max;
        D = (st-z1)/(z1)*Dp;

        [~,PM_max] = margin(K0*D*G*H);
        while PM_max < 0
            PM_max = PM_max + 360;
        end

        % newtonian search
        while search == 1
            wn_current = wn_max;
            search = 0;
        
            % right point (increase wn)
            wn = wn_current + WnRes;
            z1 = -wn;
            D = (st-z1)/(-z1)*Dp;
            [~,PM] = margin(K0*D*G*H);
            while PM < 0
                PM = PM + 360;
            end
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; 
                wn_max = wn; 
                search = 1;
            end
        
            % left point (decrease wn)
            wn = wn_current - WnRes;
            z1 = -wn;
            D = (st-z1)/(-z1)*Dp;
            [~,PM] = margin(K0*D*G*H);
            while PM < 0
                PM = PM + 360;
            end
            if PM > PM_max && search == 0
                PM_max = PM;
                z1_max = z1; 
                wn_max = wn; 
                search = 1;
            end
        end

        Z_max = z1_max;
        
        D_max = Dp * (st - z1_max) / (-z1_max);
        [num,den] = tfdata(D_max, 'v');
        D_max = tf(real(num), real(den));
    end
  
end
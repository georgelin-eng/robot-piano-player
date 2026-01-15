function [Tr, Tp, Ts, OSu] = RCG_from_CLTF(CLTF)
    [y, t] = step(CLTF);
    OSu = (max(y) - 1.0)*100;

    mask = (y>dcgain(CLTF));
    Tr = min(t(mask));    y = y/dcgain(CLTF);
    
    Ts = Ts_from_TF(CLTF);

    [~, idx] = max(y);
    Tp = t(idx);
end
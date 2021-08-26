
th = 0:1:90;
d = .025;   %[m]
t =  d*1.5 / (3*10^8);
OPL = @(th_1,n_2) (d./(cosd(asind(sind(th_1) ./ n_2) ) ) ) .* n_2 + ...
    ( t - (d ./ (cosd(asind(sind(th_1) ./ n_2) ) ) ) .* (cosd(th_1 - asind(sind(th_1) ./ n_2) ) ./ 1) );

f100 = @(n_2) OPL(th,n_2) - 100;
f200 = @(n_2) OPL(th,n_2) - 200;

n2_100 = fzero(f100,1)
n2_200 = fzero(f200,1)
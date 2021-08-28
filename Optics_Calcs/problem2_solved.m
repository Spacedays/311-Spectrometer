clc;clear;close all
theta = 0:1:90;  % [deg]


% a = @(n_2,th_1) 

OPD = @(th) ( (0.2*10^-6)./(cosd(asind(sind(th) / 1.5) ) ) ) .* (1.5 - (cosd(th - asind(sind(th) / 1.5) )/1) );
n = @(th) OPD(th) ./ (0.2*10^-6) + 1;

n_out = n(theta);
figure(1)
plot(theta,n_out)
grid on
xlabel('\theta')
ylabel('n(\theta)')
title('\theta VS n(\theta)')

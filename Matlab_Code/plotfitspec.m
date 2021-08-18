
clear, clc

load("Data001")

% hold on
% plot(Data001(:, 1), Data001(:, 2)/100,"o")

funct = @(x) (interp1(Data001(:, 1), Data001(:, 2), x, 'spline'))/100;
fplot(funct, [400, 1100])


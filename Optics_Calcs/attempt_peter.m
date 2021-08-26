%% Problem: find the refractive index (n) at each angle from 0 to 90 for OPL = 100cm and 200cm
% Also why is this important
clc;clear;close all
testing=true
% OPL = h*n+ (t-z);     OPL is the theoretical optical path length
theta = 0:1:90;  % [deg]
d = .025;       % [m]
n = 1.5;         % Refractive index of glass
t = d*n/(3*10^8);    % t = d*n/c, where n=1.5 (refractive idx of glass) and c = speed of light [m/s]


% a = @(n_2,th_1) 

OPD = @(th) ( (0.2*10^-6)./(cosd(asind(sind(th) / 1.5) ) ) ) .* (1.5 - (cosd(th - asind(sind(th) / 1.5) )/1) ) ;

% f1 = OPL_eqn

% f100 = @(n_2) OPL_eqn(n_2,theta) - 100;

f1_out = zeros(size(theta));
f2_out = f1_out;

if testing
    figure(1)
    subplot(1,2,1)
%     plot(theta,f1_out)
    title('OPL = 100cm')
    ylabel('OPL1 - 1m')%'\theta')
    xlabel('n_2')
    hold on
    
    subplot(1,2,2)
%     plot(theta,f2_out)
    title('OPL = 200cm')
    ylabel('OPL2 - 2m')%'\theta')
    xlabel('n_2')
    hold on
end
n_test = .9:0.01:3;

for i = theta
    f1 = @(n_2) OPL_eqn(n_2,i) - 1;      % make an anon fxn at th_1 = i for fzero and OPL = 0.1m [100 cm]
    f2 = @(n_2) OPL_eqn(n_2,i) - 2;      % make an anon fxn at th_1 = i for fzero and OPL = 0.2m [200 cm]
%     f1_out(i+1) = fzero(f1,1.3);
%     f2_out(i+1) = fzero(f2,1.3);
    
    if testing
        subplot(1,2,1)
        OPL1 = f1(n_test) + 1;
        plot(n_test,OPL1)
        
        subplot(1,2,2)
        OPL2 = f2(n_test) + 2;
        plot(n_test,OPL2)
    end
    
end
if ~testing
    clc;close all
    subplot(1,2,1)
    plot(theta,f1_out)
    title('OPL = 100cm')
    xlabel('\theta'), ylabel('n_2')

    subplot(1,2,2)
    plot(theta,f2_out)
    title('OPL = 200cm')
    xlabel('\theta'), ylabel('n_2')
end
% n2_100 = fzero(f100,1)

%% OPL = 100


%% OPL = 200

% function fxn_gen(th1)
%     return  
% end
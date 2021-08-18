% This is code for taking data off of the Arduino and displaying it

clear, clc

port = serialportlist("available");
disp("Connecting to "+port(end))
arduinoObj = serialport(port(end), 115200); % Selects the COM port for the Arduino
% arduinoCom = serial(port(end), 'BaudRate', 115200);
configureTerminator(arduinoObj, "LF")
flush(arduinoObj)
% fopen(arduinoCom)
% fprintf(arduinoCom, '%s', 'L')

load("Data001")
load('Calibrated_Wavelength')
load('Noise_Profile')
calibData = @(x) (interp1(Data001(:, 1), Data001(:, 2), x, 'spline'))/100;
intensityCalib = @(lambda, intensity) intensity/calibData(lambda);
%writeline(arduinoObj, " 2")

% [RGB, style] = wavelengthToRGB(wavelength)

avgLen = 15;
raw = zeros(256,1);
idx_reading = 1;

plotData = 1:256;

p = plot(plotData, wavelength);
% axis([150, 1000, -10, 2200])

p.XDataSource = 'wavelength';
p.YDataSource = "plotData";
% p.Color = 
title('Relative Intensity vs. Wavelength')
xlabel('Wavelength [nm]')
ylabel('Relative Intensity')

while ishghandle(p)
    idx_sample = 1;
    while idx_sample <= 256
        if arduinoObj.NumBytesAvailable>0
            raw(idx_sample,idx_reading) = str2double(readline(arduinoObj));
            idx_sample = idx_sample + 1;
        end
        pause(0.00001)
    end
    idx_reading = idx_reading + 1;
    if idx_reading > avgLen
        idx_reading = idx_reading - 1;
        raw(:,1) = [];
    end
    plotData = mean(raw,2)-Noise;
    refreshdata
    drawnow
    % disp("Update")
    pause(0.01)
end

p = plot(wavelength, plotData);
% axis([150, 1000, -10, 2200])
title('Relative Intensity vs. Wavelength')
xlabel('Wavelength [nm]')
ylabel('Relative Intensity')

selection = questdlg('Save Last Dataset?', 'Figure Closed', 'Yes', 'No', 'Yes');
switch selection
    case 'Yes'
      DataSave = [transpose(wavelength) plotData];
      writematrix(DataSave, input('File Name: ', 's'))
    case 'No'
end

selection = questdlg('Save Last Figure?', 'Figure Closed', 'Yes', 'No', 'Yes');
switch selection
    case 'Yes'
      saveas(p, input('Figure Name: ', 's'))
    case 'No'
end

clear arduinoObj
close all

function calibratedWavelength =  calibrateHydrogen(intensity)
[i_a i_b i_c] = findTriPeaks2(intensity);
wavelength = findWavelength(1:size(intensity),[i_c,i_a]);	% using default LAM values for hydrogen
save('Calibrated Wavelength',wavelength)
end
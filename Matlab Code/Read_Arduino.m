% This is code for taking data off of the Arduino and displaying it

clear, clc

port = serialportlist("available");
disp("Connecting to "+port(end))
arduinoObj = serialport(port(end), 115200); % Selects the COM port for the Arduino
configureTerminator(arduinoObj, "LF")
flush(arduinoObj)

load("Data001")
load('Calibrated_Wavelength')
calibData = @(x) (interp1(Data001(:, 1), Data001(:, 2), x, 'spline'))/100;
intensityCalib = @(lambda, intensity) intensity/calibData(lambda);
%writeline(arduinoObj, " 2")

raw = 1:256;

p = plot(raw, wavelength);
p.XDataSource = 'wavelength';
p.YDataSource = "raw";
title('Relative Intensity vs. Wavelength')
xlabel('Wavelength [nm]')
ylabel('Relative Intensity')

while ishghandle(p)
    raw = [];
    while length(raw) < 256
        if arduinoObj.NumBytesAvailable>0
            %read(arduinoObj,1,"char")
            raw(end+1) = str2double(readline(arduinoObj));
        end
        pause(0.00001)
    end
    refreshdata
    drawnow
    % disp("Update")
    pause(0.01)
end

p = plot(wavelength, raw);
title('Relative Intensity vs. Wavelength')
xlabel('Wavelength [nm]')
ylabel('Relative Intensity')

selection = questdlg('Save Last Dataset?', 'Figure Closed', 'Yes', 'No', 'Yes');
switch selection
    case 'Yes'
      writematrix(raw, input('File Name: ', 's'))
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

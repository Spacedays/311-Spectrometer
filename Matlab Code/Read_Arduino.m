% This is code for taking data off of the Arduino and displaying it

clear, clc

port = serialportlist("available");
arduinoObj = serialport(port(end), 9600); % Selects the COM port for the Arduino
configureTerminator(arduinoObj, "CR/LF")
flush(arduinoObj)
arduinoObj.UserData = struct("Data", [], "Count", 1);

% Read the Arduino Function

configureCallback(arduinoObj, "terminator", @readSpectrum)

load("Data001")
calibData = @(x) (interp1(Data001(:, 1), Data001(:, 2), x, 'spline'))/100;
intensityCalib = @(lambda, intensity) intensity/calibData(lambda);

while arduinoObj.UserData.Count <= 1024
    % Do Nothing
end

load("Hydrogen")

raw = arduinoObj.UserData.Data;

function readSpectrum(src, ~)

% Read data from the serialport object
data = readline(src);

% Convert the string data to numeric type and save it to UserData property
% of the serialport object,
src.UserData.Data(end+1) = str2double(data);

% Update the Count value of the serialport object
src.UserData.Count = src.UserData.Count+1;

% If 1001 data points have been collected from the Arduino, switch off the
% callbacks and plot the data

if src.UserData.Count > 1024
    configureCallback(src, "off");
end
end


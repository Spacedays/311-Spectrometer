%% Serial Port Communications 
clear

serialportlist("available") %establishing a communication

ard = serialport("COM5", 9600)

configureTerminator(ard,"CR/LF")

flush(ard)

ard.UserData = struct("Data",[],"Count",1)


configureCallback(ard,"terminator", @readArduinoData);
% 
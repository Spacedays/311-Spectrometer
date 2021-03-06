
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
load('Noise_Profile')
calibData = @(x) (interp1(Data001(:, 1), Data001(:, 2), x, 'spline'))/100;
intensityCalib = @(lambda, intensity) intensity/calibData(lambda);
%writeline(arduinoObj, " 2")

pixels = 1:256;
avgLen = 10;
raw = zeros(256,1);
idx_reading = 1;

plotData = 1:256;

p = plot(plotData, pixels);
% axis([150, 1000, -10, 2200])

p.XDataSource = 'pixels';
p.YDataSource = "plotData";
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

p = plot(pixels, plotData);
selection = questdlg('Use this profile?', 'Figure Closed', 'Yes', 'No', 'Yes');
switch selection
    case 'Yes'
      LAM = [656.3 486.1];	% approx peaks of the grow lights
      [c_a c_b] = findDualPeaks(plotData,20);
      wavelength = findWavelength(pixels,[pixels(c_a) pixels(c_b)], LAM);
      save('Calibrated_Wavelength', 'wavelength');
    case 'No'
end

clear arduinoObj
close all



% %%
% % Example call: 
% pixels = 1:1:256;
% intensity = raw;
% 
% LAM = [700 450];	% approx peaks of the grow lights
% [c_a c_b] = findDualPeaks(intensity,20);
% wavelength = findWavelength(pixels,[pixels(c_a) pixels(c_b)], LAM);


function lambda = findWavelength(idx,calib_idx,LAM)
% For use with calibration; for hydrogen, i_a  should be the 656nm peak, i_b should be @ 486.1nm,
% where lambda is the wavelength to be found & idx is the pixel index of the EPC to use
%
% The findTriPeaks function can be used to make calib_idx from emission data.
%
% To determine which peak is which, locate peak c at 434nm (see table below) and compare its index to a & b
%
%		 Measurement:	 a		  b		 c
%	 Wavelength [nm]:	656.2	486.1	434.0
% Relative intensity:	~~1		~~1		~~.4
%
% idx: index(es) to convert to wavelength
% calib_idx: Indeces of two peak wavelengths, in order of wavelength Highest to lowest

% optional arg LAM
if ~exist('LAM','var')
	LAM_a = 656.2;	% hydrogen main peak 1 [nm]
	LAM_b = 486.1;	% hydrogen main peak 2 [nm]
%	LAM_c = 434.0;	% hydrogen tertiary peak [nm]
else
	LAM_a = LAM(1);	% Greater wavelength peak
	LAM_b = LAM(2);	% Lesser wavelength peak
end

i_a = calib_idx(1);
i_b = calib_idx(2);
%i_c = calib_idx(3);

slope = (LAM_a - LAM_b)./ (i_a-i_b);
offset = LAM_b;

lambda =  slope*(idx-i_b) + offset;
end

function [idx_a,idx_b] = findDualPeaks(intensity,peakWidth,greater)
% Goal: return the index of the higher wavelength as idx_a
%
% Assumes there will be two primary peaks with a tertiary smaller peak
%   Measurement:	  a				  c
%    Wavelength:	 high			 low
% Rel Intensity: (VAR)greater    (VAR)greater
%   Pixel index:	 i_a			 i_b
%
% intensity: vector with length of # of pixels
% peakWidth: exclusion zone on each side of the peak [ number of indeces ]
% greater: which of the two peaks has a higher intensity. 1 = a (larger wavelength), 2 = b (smaller wavelength)

if ~exist('peakWidth','var')
	peakWidth = 7;	% a rather conservative guess at the maximum peak width
end
if ~exist('higher','var')
	greater = 1;		% Defaults to the first provided value being higher
end

% Primary peak & pixel index
[~, idx(1)] = max(intensity);
intensity((idx(1)-peakWidth):(idx(1)+peakWidth)) = 0;		

% find bounds for erase zone
erase_zone_bottom = (idx(1)-peakWidth);
erase_zone_top = (idx(1)+peakWidth);

% correct bounds for erase zone
if( erase_zone_bottom < 0)
	erase_zone_bottom = 0;
end
if(erase_zone_top > length(intensity))
	erase_zone_top = length(intensity);
end

% Erase data around first peak so search can find next largest peaks
intensity(erase_zone_bottom:erase_zone_top) = 0;

% Secondary peak & pixel index
[~, idx(2)] = max(intensity);


if(greater == 1)		% larger wavelength has greater intensity
	idx_a = max(idx);
	idx_b = min(idx);
else					% smaller wavelength has greater intensity
	idx_a = min(idx);
	idx_b = max(idx);
end

end

function [idx_a,idx_b,idx_c] = findTriPeaks(intensity,peakWidth)
% Goal: return the peak indeces sorted by wavelength, highest to lowest
%
% Assumes there will be two primary peaks with a tertiary smaller peak
% Measurement:    a		  b		  c
%  Wavelength: highest	middle	lowest
%   Intensity:   high    high   lowest
% Pixel index:	 i_a	 i_b	 i_c
% intensity: vector with length of # of pixels

if ~exist('peakWidth','var')
	peakWidth = 7;	% a rather conservative guess at the maximum peak width
end

% Primary peak & pixel index
[values(1), idx(1)] = max(intensity);

% find bounds for erase zone
erase_zone_bottom = (idx(1)-peakWidth);
erase_zone_top = (idx(1)+peakWidth);

% correct bounds for erase zone
if( erase_zone_bottom < 0)
	erase_zone_bottom = 0;
end
if(erase_zone_top > length(intensity))
	erase_zone_top = length(intensity);
end

% Erase data around first peak so search can find next largest peaks
intensity(erase_zone_bottom:erase_zone_top) = 0;		% Erase Primary peak

% Secondary peak & pixel index
[~, idx(2)] = max(intensity);


% find bounds for erase zone
erase_zone_bottom = (idx(2)-peakWidth);
erase_zone_top = (idx(2)+peakWidth);

% correct bounds for erase zone
if( erase_zone_bottom < 0)
	erase_zone_bottom = 0;
end
if(erase_zone_top > length(intensity))
	erase_zone_top = length(intensity);
end

intensity(erase_zone_bottom:erase_zone_top) = 0;		% Erase Secondary peak

% Tertiary peak & pixel index
[~, idx_c] = max(intensity);


if(idx(1) > idx_c)	% idx 1 is either a or b, and both have larger intensities than c;
					%  if index idx1 is greater than idx c, then the direction is standard
					%  ==> + change in pixel index = + change in wavelength
	idx_a = max(idx);
	idx_b = min(idx);
else
	idx_a = min(idx);
	idx_b = max(idx);
end

end
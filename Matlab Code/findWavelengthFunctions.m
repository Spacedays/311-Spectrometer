%%
HIGHER = 2; LOWER = 1; % Constants for which peak has greater intensity
% Example call: 
pixels =	[60 70	80	90	100 110 120 130 140 ];
intensity = [5	100 7	6	8	60	500	120	0	];

LAM = [450 700];	% approx peaks of the grow lights
%% First example (Not ideal; simple peak finding algorithm)
% [idx_low idx_high] = findDualPeaks(intensity,2,2);
% wavelength = findWavelength(pixels,[pixels(idx_low) pixels(idx_high)], LAM)
%plot(wavelength, intensity)

%% Second example (Ideal; uses Matlab's signal processing toolbox peak finding agorighm)
[idx_low idx_high] = findDualPeaks2(intensity,2,HIGHER);
wavelength = findWavelength(pixels,[pixels(idx_low),pixels(idx_high)],LAM);
figure(2)
plot(wavelength,intensity)

%% Third example (using findTriPeaks2)
pixels3 =	[60 70	80	90	100 110 120 130 140 150 160 170 180 190 200	210];
intensity3 = [5	100 7	6	8	60	500	120	0	10	5	10	50	600	250	0];

[i_a i_b i_c] = findTriPeaks2(intensity3);
wavelength3 = findWavelength(pixels3,[pixels3(i_c),pixels3(i_a)]);	% using default LAM values for hydrogen
figure(3)
plot(wavelength3,intensity3)


%% functions
function lambda = findWavelength(pixelArray,calib_idx,LAM)
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
% calib_idx: Indeces of two peak wavelengths, in order of wavelength lowest to highest
% optional arg LAM: LAM(1) = lower wavelength peak, 2 = higher wavelength peak

if ~exist('LAM','var')
	% Use as large of a difference as is feasbile for better accuracy (I think)
	% 	LAM_ab = 486.1;	% hydrogen main peak 2 [nm]
	LAM_a = 434.0;	% hydrogen tertiary peak [nm] 	
	LAM_b = 656.2;	% hydrogen main peak 1 [nm]
	
else
	LAM_a = LAM(1);	% Lesser wavelength peak
	LAM_b = LAM(2);	% Greater wavelength peak
end

i_a = calib_idx(1);		% index of lesser wavelength peak
i_b = calib_idx(2);		% index of greater wavelength peak
%i_c = calib_idx(3);

slope = (LAM_b - LAM_a)./ (i_b-i_a);

offset = LAM_a;

lambda =  slope*(pixelArray-i_a) + offset;
end


function [idx_low,idx_high] = findDualPeaks2(intensity,peakSpacing,greater)
% Goal: return the index of the lower wavelength as idx_low USING THE SIGNAL PROCESSING TOOLBOX
%
% Assumes there will be a primary and a secondary peak
% inputs:
	%   Measurement:	  a				  b
	%    Wavelength:	 low			 high
% outputs:
	%  Rel Intensity:   low/high		low/high
	%   Pixel index:	i_a/i_b			 i_a/i_b
%
% intensity: vector with length of # of pixels
% peakWidth: exclusion zone on each side of the peak [ number of indeces ]
% greater: which of the two peaks has a higher intensity. 1 = a (lower wavelength), 2 = b (higher wavelength)

if ~exist('peakSpacing','var')
	peakSpacing = 4;	% a guess at the maximum peak width we care about
end
if ~exist('greater','var')
	greater = 1;		% Defaults to lower wavelength having a larger peak
end

[~, idx] = findpeaks(intensity,'MinPeakDistance',peakSpacing,'SortStr','descend');

if(greater == 1)		% lower wavelength has greater intensity
	idx_low = idx(1);
	idx_high = idx(2);
else					% higher wavelength has greater intensity
	idx_low = idx(2);
	idx_high = idx(1);
end

end

function [idx_a,idx_b,idx_c] = findTriPeaks2(intensity,peakSpacing)
% Goal: return the peak indeces sorted by wavelength, highest to lowest
%
% Assumes there will be two primary peaks with a tertiary smaller peak
% Knowns/inputs:
	% Measurement:    a		  b		  c
	%  Wavelength: lowest	middle	highest
	%   Intensity: lowest    high   high,   where intensity for a,b,c are from the intensity input vector
% Returns:
	% Pixel index:	 i_a	 i_b	 i_c
	

if ~exist('peakSpacing','var')
	peakSpacing = 4;	% a guess at the maximum peak width we care about
end

[~, idx] = findpeaks(intensity,'MinPeakDistance',peakSpacing,'SortStr','descend','NPeaks',3);
idx_a = idx(1);
idx_b = idx(2);
idx_c = idx(3);

end

%% Old peak functions
function [idx_low,idx_high] = findDualPeaks(intensity,peakSpacing,greater)
% Goal: return the index of the lower wavelength as idx_low
%
% Assumes there will be two primary peaks with a tertiary smaller peak
%   Measurement:	  a				  c
%    Wavelength:	 low			 high
% Rel Intensity: input var greater  input var greater
%   Pixel index:	 i_a			 i_b
%
% intensity: vector with length of # of pixels
% peakWidth: exclusion zone on each side of the peak [ number of indeces ]
% greater: which of the two peaks has a higher intensity. 1 = a (smaller wavelength), 2 = b (larger wavelength)

if ~exist('peakWidth','var')
	peakSpacing = 7;	% a rather conservative guess at the maximum peak width
end
% if ~exist('higher','var')
% 	greater = 1;		% Defaults to the first provided value being higher
% end

% Primary peak & pixel index
[~, idx(1)] = max(intensity);
intensity((idx(1)-peakSpacing):(idx(1)+peakSpacing)) = 0;		

% find bounds for erase zone
erase_zone_bottom = (idx(1)-peakSpacing);
erase_zone_top = (idx(1)+peakSpacing);

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


if(greater == 1)		% smaller wavelength has greater intensity
	idx_low = idx(1);
	idx_high = idx(2);
else					% larger wavelength has greater intensity
	idx_low = idx(2);
	idx_high = idx(1);
end

end

function [idx_a,idx_b,idx_c] = findTriPeaks(intensity,peakSpacing)
% Goal: return the peak indeces sorted by wavelength, highest to lowest
%
% Assumes there will be two primary peaks with a tertiary smaller peak
% Measurement:    a		  b		  c
%  Wavelength: highest	middle	lowest
%   Intensity:   high    high   lowest
% Pixel index:	 i_a	 i_b	 i_c
% intensity: vector with length of # of pixels

if ~exist('peakWidth','var')
	peakSpacing = 7;	% a rather conservative guess at the maximum peak width
end

% Primary peak & pixel index
[values(1), idx(1)] = max(intensity);

% find bounds for erase zone
erase_zone_bottom = (idx(1)-peakSpacing);
erase_zone_top = (idx(1)+peakSpacing);

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
erase_zone_bottom = (idx(2)-peakSpacing);
erase_zone_top = (idx(2)+peakSpacing);

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

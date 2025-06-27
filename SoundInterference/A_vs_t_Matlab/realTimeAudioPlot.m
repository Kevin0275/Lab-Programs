% Real-Time Sound Level Meter in MATLAB

% Parameters
Fs = 44100;              % Sampling rate (Hz)
frameSize = 1024;        % Samples per frame
duration = 10000;          % Total duration in seconds (5 minutes)
timeWindow = 0.001;         % Seconds of scrolling window in plot

% Audio input
deviceReader = audioDeviceReader('SampleRate', Fs, ...
    'SamplesPerFrame', frameSize);

% Plot settings
scope = dsp.TimeScope( ...
    'SampleRate', Fs / frameSize, ...         % One dB value per frame
    'TimeSpan', timeWindow, ...
    'YLimits', [0, 100], ...
    'Title', 'Real-Time Sound Level Meter (RMS dB)', ...
    'YLabel', 'Sound Level (dB)', ...
    'TimeUnits', 'Seconds', ...
    'ShowGrid', true);

% Preallocate audio buffer (optional for saving)
maxFrames = floor(duration * Fs / frameSize);
dBLog = zeros(maxFrames, 1);

disp('Sound meter running...');
frameCount = 1;
tic;

while toc < duration
    % Get audio frame
    audioFrame = deviceReader();

    % Compute RMS of the frame
    rmsVal = sqrt(mean(audioFrame.^2));

    % Convert to dBFS (decibels relative to full scale)
    if rmsVal < 1e-12
        rmsVal = 1e-12; % Prevent log(0)
    end
    dB = 20 * log10(rmsVal);      % dBFS, max 0 dB if signal is full scale

    % Convert to 0–100 display scale (0 = silence, 100 = full-scale)
    dB_display = dB + 100;        % Shift range from [-100, 0] to [0, 100]
    
    % Log and display
    dBLog(frameCount) = dB_display;
    scope(dB_display);
    frameCount = frameCount + 1;
end

disp('Measurement finished.');
release(deviceReader);
release(scope);
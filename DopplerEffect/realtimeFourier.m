% Real-Time FFT Spectrum Display with 0–150 dB Range

% Parameters
Fs = 100000;             % Sampling rate
frameSize = 2048;       % Samples per frame
duration = 1000;         % Duration in seconds

% Create audio input
deviceReader = audioDeviceReader('SampleRate', Fs, ...
    'SamplesPerFrame', frameSize);

% Create spectrum analyzer object (FFT viewer)
spectrumScope = dsp.SpectrumAnalyzer( ...
    'SampleRate', Fs, ...
    'SpectrumType', 'Power', ...
    'PlotAsTwoSidedSpectrum', false, ...
    'Title', 'Real-Time Audio Spectrum', ...
    'ShowLegend', false, ...
    'AveragingMethod', 'Running', ...
    'ForgettingFactor', 0.9, ...
    'YLimits', [0, 150]);   % <-- Updated range here

% Start real-time analysis
disp('Running real-time FFT spectrum...');
tic;
while toc < duration
    audioFrame = deviceReader();         % Get audio
    spectrumScope(audioFrame);          % Plot spectrum
end

% Cleanup
release(deviceReader);
release(spectrumScope);
disp('Measurement complete.');
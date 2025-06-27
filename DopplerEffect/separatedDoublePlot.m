%% Clear workspace
clear; close all; clc;

%% Select two audio files
[file1, path1] = uigetfile({'*.wav;*.dat;*.mp3;*.ogg;*.flac;*.m4a;*.aac', ...
                   'Audio Files (*.wav, *.mp3, *.ogg, *.flac, *.m4a, *.aac)'}, ...
                   'Select STATIONARY Speaker File');
if isequal(file1, 0), return; end

[file2, path2] = uigetfile({'*.wav;*.dat;*.mp3;*.ogg;*.flac;*.m4a;*.aac', ...
                   'Audio Files (*.wav, *.mp3, *.ogg, *.flac, *.m4a, *.aac)'}, ...
                   'Select SPINNING Speaker File');
if isequal(file2, 0), return; end

%% Load and process files
[y1,Fs1] = audioread(fullfile(path1,file1)); 
[y2,Fs2] = audioread(fullfile(path2,file2));

% Mono conversion and normalization
if size(y1,2) > 1, y1 = mean(y1,2); end
if size(y2,2) > 1, y2 = mean(y2,2); end
y1 = y1/max(abs(y1));
y2 = y2/max(abs(y2));

%% Compute FFTs with proper windowing
N1 = length(y1);
N2 = length(y2);
N = max([N1, N2, 2^16]); % Minimum of 2^16 points for better resolution

% Create separate windows for each signal
win1 = hann(N1);
win2 = hann(N2);

% Compute FFTs with zero-padding
Y1 = abs(fft(y1.*win1, N));
Y2 = abs(fft(y2.*win2, N));

% Frequency axis (use Fs1 assuming both files have same sample rate)
f = Fs1*(0:N/2-1)/N;
Y1 = Y1(1:N/2);
Y2 = Y2(1:N/2);

%% Find peaks and bandwidths
[~,idx1] = max(Y1); f1_peak = f(idx1);
[~,idx2] = max(Y2); f2_peak = f(idx2);

% Find -3dB points
threshold1 = max(Y1)/sqrt(2);
threshold2 = max(Y2)/sqrt(2);

bw1 = sum(Y1 > threshold1)*(f(2)-f(1));
bw2 = sum(Y2 > threshold2)*(f(2)-f(1));

%% Plot results
figure('Color','w','Position',[100 100 900 600]);

subplot(2,1,1);
semilogx(f,20*log10(Y1/max(Y1)+eps),'b','LineWidth',1.5);
title(sprintf('Stationary: %s (Peak: %.1f Hz)',file1,f1_peak),'Interpreter','none');
xlabel('Frequency (Hz)'); ylabel('Magnitude (dB)');
xlim([20 20000]); ylim([-60 5]); grid on;
set(gca,'XTick',[20 50 100 200 500 1e3 2e3 5e3 10e3 20e3]);

subplot(2,1,2);
semilogx(f,20*log10(Y2/max(Y2)+eps),'r','LineWidth',1.5);
title(sprintf('Spinning: %s (Peak: %.1f Hz)',file2,f2_peak),'Interpreter','none');
xlabel('Frequency (Hz)'); ylabel('Magnitude (dB)');
xlim([20 20000]); ylim([-60 5]); grid on;
set(gca,'XTick',[20 50 100 200 500 1e3 2e3 5e3 10e3 20e3]);

%% Display quantitative results
fprintf('\n=== Broadening Results ===\n');
fprintf('Stationary speaker:\n');
fprintf('  Peak frequency: %.1f Hz\n', f1_peak);
fprintf('  -3dB bandwidth: %.1f Hz\n', bw1);
fprintf('Spinning speaker:\n');
fprintf('  Peak frequency: %.1f Hz\n', f2_peak);
fprintf('  -3dB bandwidth: %.1f Hz\n', bw2);
fprintf('  Bandwidth increase factor: %.1fx\n', bw2/bw1);
if abs(f1_peak - f2_peak) > 10
    fprintf('  Warning: Large peak frequency difference detected\n');
elseif bw2/bw1 > 1.5
    fprintf('  → Clear Doppler broadening detected\n');
end
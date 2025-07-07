function analyzeDopplerEffect()
    % Let user select an audio file
    [filename, pathname] = uigetfile({'*.wav;*.dat;*.mp4;*.m4a;*.mp3;*.ogg;*.flac;*.aac',...
                                     'Audio Files (*.wav, *.mp3, *.ogg, *.flac, *.aac)'},...
                                     'Select Audio File for Doppler Analysis');
    if isequal(filename, 0)
        disp('User canceled file selection');
        return;
    end
    audioFile = fullfile(pathname, filename);
    
    % Read audio file
    try
        [audioData, sampleRate] = audioread(audioFile);
    catch ME
        errordlg(sprintf('Error reading audio file:\n%s', ME.message), 'File Error');
        return;
    end
    
    % If stereo, convert to mono by averaging channels
    if size(audioData, 2) == 2
        audioData = mean(audioData, 2);
    end
    
    % Parameters for STFT analysis
    windowSize = 2048;       % Size of FFT window
    overlap = 0.90;          % Overlap between windows (90%)
    hopSize = round(windowSize * (1 - overlap)); % Samples between analyses
    nfft = windowSize;       % Number of FFT points
    
    % Calculate number of time segments
    numSegments = floor((length(audioData) - windowSize) / hopSize) + 1;
    
    % Frequency vector
    freqVector = (0:nfft/2) * (sampleRate / nfft);
    
    % Initialize spectrogram matrix
    spectrogramData = zeros(nfft/2 + 1, numSegments);
    timeStamps = zeros(numSegments, 1);
    
    % Create window function
    window = hann(windowSize);
    
    % Process each time segment
    for i = 1:numSegments
        % Get current segment
        startIdx = (i-1)*hopSize + 1;
        endIdx = startIdx + windowSize - 1;
        segment = audioData(startIdx:endIdx) .* window;
        
        % Compute FFT
        fftResult = abs(fft(segment, nfft));
        spectrogramData(:,i) = fftResult(1:nfft/2+1);
        
        % Store time stamp (middle of window)
        timeStamps(i) = (startIdx + windowSize/2) / sampleRate;
    end
    
    % Convert to RELATIVE sound intensity (I ∝ A²)
    maxFFT = max(spectrogramData(:));
    relativeIntensity = (spectrogramData / maxFFT).^2;
    
    % Scale to plausible real-world range (hearing threshold to jet engine)
    minRealWorld = 1e-12;  % W/m² (hearing threshold)
    maxRealWorld = 10;     % W/m² (jet engine)
    scaledIntensity = relativeIntensity * (maxRealWorld - minRealWorld) + minRealWorld;
    
    % Create figure
    fig = figure('Name', 'Doppler Effect Analysis', 'NumberTitle', 'off',...
                'Position', [100 100 1400 900], 'Color', [0.15 0.15 0.15]);
    
    % Plot 1: 2D Frequency-Time Heatmap (Log W/m²)
    subplot(4,1,1);
    imagesc(timeStamps, freqVector/1000, log10(scaledIntensity));
    axis xy;
    colormap('jet');
    hcb = colorbar;
    hcb.Label.String = 'Log Intensity (log₁₀ W/m²)';
    xlabel('Time (s)');
    ylabel('Frequency (kHz)');
    title('Frequency Content vs Time (Relative Sound Intensity)');
    set(gca, 'Color', [0.2 0.2 0.2], 'XColor', [0.8 0.8 0.8], 'YColor', [0.8 0.8 0.8]);
    
    % Plot 2: 3D Surface Plot (Log W/m²)
    subplot(4,1,2);
    surf(timeStamps, freqVector/1000, log10(scaledIntensity), 'EdgeColor', 'none');
    view(45, 30);
    colormap('jet');
    hcb = colorbar;
    hcb.Label.String = 'Log Intensity (log₁₀ W/m²)';
    xlabel('Time (s)');
    ylabel('Frequency (kHz)');
    zlabel('Log Intensity (log₁₀ W/m²)');
    title('3D Frequency Spectrum (Relative Sound Intensity)');
    set(gca, 'Color', [0.2 0.2 0.2], 'XColor', [0.8 0.8 0.8], 'YColor', [0.8 0.8 0.8], 'ZColor', [0.8 0.8 0.8]);
    grid on;
    
    % Plot 3: 2D Plot with Dual Axes (Frequency and Intensity vs Time)
    subplot(4,1,3);
    
    % Let user select frequency range
    defaultFreq = [1, 5]; % Default 1-5 kHz range
    prompt = {'Enter minimum frequency (kHz):', 'Enter maximum frequency (kHz):'};
    dlgtitle = 'Select Frequency Band';
    dims = [1 35];
    definput = {num2str(defaultFreq(1)), num2str(defaultFreq(2))};
    answer = inputdlg(prompt, dlgtitle, dims, definput);
    
    if isempty(answer)
        return;
    end
    
    minFreq = str2double(answer{1}) * 1000; % Convert kHz to Hz
    maxFreq = str2double(answer{2}) * 1000;
    
    % Find frequency indices
    freqIndices = find(freqVector >= minFreq & freqVector <= maxFreq);
    
    if isempty(freqIndices)
        errordlg('Invalid frequency range selected', 'Error');
        return;
    end
    
    % Find peak frequency and corresponding intensity in band at each time point
    peakFrequencies = zeros(1, numSegments);
    peakIntensities = zeros(1, numSegments);
    for i = 1:numSegments
        [maxIntensity, maxIdx] = max(scaledIntensity(freqIndices, i));
        peakFrequencies(i) = freqVector(freqIndices(maxIdx));
        peakIntensities(i) = maxIntensity;
    end
    
    % Create plot with dual y-axes
    yyaxis left;
    plot(timeStamps, peakFrequencies/1000, 'b-', 'LineWidth', 2);
    ylabel('Frequency (kHz)');
    ylim([minFreq/1000, maxFreq/1000]);
    
    yyaxis right;
    plot(timeStamps, log10(peakIntensities), 'r-', 'LineWidth', 2);
    ylabel('Log Intensity (log₁₀ W/m²)');
    
    xlabel('Time (s)');
    title(sprintf('Frequency and Intensity Tracking: %.1f-%.1f kHz', minFreq/1000, maxFreq/1000));
    grid on;
    set(gca, 'Color', [0.2 0.2 0.2], 'XColor', [0.8 0.8 0.8], 'YColor', [0.8 0.8 0.8]);
    legend('Frequency (kHz)', 'Log Intensity (log₁₀ W/m²)', 'Location', 'best');
    
    % Plot 4: Frequency changes and peak-to-peak analysis
    subplot(4,1,4);
    
    % Calculate frequency differences
    freqChanges = diff(peakFrequencies);
    
    % Find maximum absolute frequency change
    [maxChange, maxChangeIdx] = max(abs(freqChanges));
    maxChangeTime = timeStamps(maxChangeIdx);
    maxChangeFreq = freqChanges(maxChangeIdx);
    
    % Find peak-to-peak variation
    [maxFreq, maxIdx] = max(peakFrequencies);
    [minFreq, minIdx] = min(peakFrequencies);
    peakToPeak = maxFreq - minFreq;
    
    % Get corresponding intensity values
    maxIntensity = peakIntensities(maxIdx);
    minIntensity = peakIntensities(minIdx);
    
    % Plot frequency changes
    plot(timeStamps(1:end-1), freqChanges/1000, 'g-', 'LineWidth', 1.5);
    hold on;
    
    % Mark maximum change point
    plot(maxChangeTime, maxChangeFreq/1000, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    
    % Mark peak and trough points
    plot(timeStamps(maxIdx), maxFreq/1000, 'm^', 'MarkerSize', 10, 'LineWidth', 2);
    plot(timeStamps(minIdx), minFreq/1000, 'mv', 'MarkerSize', 10, 'LineWidth', 2);
    
    xlabel('Time (s)');
    ylabel('Frequency Change (kHz)');
    title(sprintf(['Frequency Changes\nMax Change: %.2f kHz at %.2fs | Peak-to-Peak: %.2f kHz\n' ...
                  'Max Intensity: %.2e W/m² | Min Intensity: %.2e W/m²'],...
                 maxChange/1000, maxChangeTime, peakToPeak/1000, maxIntensity, minIntensity));
    legend('Changes', 'Max Change', 'Peak', 'Trough');
    grid on;
    set(gca, 'Color', [0.2 0.2 0.2], 'XColor', [0.8 0.8 0.8], 'YColor', [0.8 0.8 0.8]);
    
    % Save data to Excel
    [excelFile, excelPath] = uiputfile('*.xlsx', 'Save Analysis Results', 'analysis_results.xlsx');
    if ~isequal(excelFile, 0)
        % Create output tables
        fullSpectrumTable = table(...
            repmat(timeStamps', length(freqVector), 1), ...
            repmat(freqVector, 1, length(timeStamps))', ...
            scaledIntensity(:), ...
            log10(scaledIntensity(:)), ...
            'VariableNames', {'Time_s', 'Frequency_Hz', 'Intensity_Wm2', 'LogIntensity_log10Wm2'});
        
        peakTable = table(...
            timeStamps, ...
            peakFrequencies', ...
            peakIntensities', ...
            log10(peakIntensities'), ...
            'VariableNames', {'Time_s', 'PeakFrequency_Hz', 'PeakIntensity_Wm2', 'LogPeakIntensity_log10Wm2'});
        
        summaryTable = table(...
            {'Max Frequency'; 'Min Frequency'; 'Peak-to-Peak'; 'Max Intensity'; 'Min Intensity'; 'Max Change'; 'Time of Max Change'}, ...
            [maxFreq; minFreq; peakToPeak; maxIntensity; minIntensity; maxChange; maxChangeTime], ...
            {'Hz'; 'Hz'; 'Hz'; 'W/m²'; 'W/m²'; 'Hz'; 's'}, ...
            'VariableNames', {'Metric', 'Value', 'Unit'});
        
        % Write to Excel
        try
            writetable(fullSpectrumTable, fullfile(excelPath, excelFile), 'Sheet', 'Full Spectrum');
            writetable(peakTable, fullfile(excelPath, excelFile), 'Sheet', 'Peak Frequencies');
            writetable(summaryTable, fullfile(excelPath, excelFile), 'Sheet', 'Summary');
            
            msgbox(sprintf('Analysis complete!\nResults saved to:\n%s', fullfile(excelPath, excelFile)),...
                  'Analysis Complete');
        catch ME
            errordlg(sprintf('Error saving Excel file:\n%s', ME.message), 'Export Error');
        end
    end
end
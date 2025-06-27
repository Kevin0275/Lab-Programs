function audioProcessingDemo()
    % Ask user whether to record or open a file
    choice = questdlg('Would you like to record audio or open an existing file?', ...
                     'Audio Processing', ...
                     'Record', 'Open File', 'Cancel', 'Record');
    
    if strcmp(choice, 'Cancel')
        disp('Operation cancelled.');
        return;
    end
    
    if strcmp(choice, 'Record')
        % Audio Acquisition Parameters
        Fs = 400;       % Sampling frequency (Hz)
        channel = 1;    % Number of audio channels (1=mono, 2=stereo)
        bits = 16;      % Bit depth
        duration = 5;   % Recording duration in seconds
        
        % Create audio recorder object
        r = audiorecorder(Fs, bits, channel);
        
        % Display recording information
        disp(['Starting ', num2str(duration), ' second recording at ', num2str(Fs), ' Hz...']);
        
        % Record audio
        recordblocking(r, duration);
        disp('Recording complete.');
        
        % Get audio data
        X = getaudiodata(r);
        
        % Save audio file with timestamp
        filename = ['audio_', datestr(now, 'yyyymmdd_HHMMSS'), '_', num2str(Fs), 'Hz.wav'];
        audiowrite(filename, X, Fs);
        disp(['Audio saved as: ', filename]);
        
    else % Open File
        [file, path] = uigetfile({'*.wav;*.dat;*.m4a;*.mp3;*.ogg;*.flac;*.au;*.aiff;*.aif;*.aifc', ...
                                 'Audio Files (*.wav, *.mp3, *.ogg, *.flac, *.au, *.aiff)';
                                 '*.*', 'All Files (*.*)'}, ...
                                'Select an audio file');
        
        if isequal(file, 0)
            disp('No file selected. Operation cancelled.');
            return;
        end
        
        fullpath = fullfile(path, file);
        [X, Fs] = audioread(fullpath);
        
        % Convert stereo to mono if needed
        if size(X, 2) > 1
            X = mean(X, 2);
            disp('Stereo file converted to mono.');
        end
        
        disp(['File loaded: ', fullpath]);
        disp(['Sampling rate: ', num2str(Fs), ' Hz']);
    end
    
    % Play back the audio
    disp('Playing back audio...');
    sound(X, Fs);
    pause(length(X)/Fs + 1); % Wait for playback to finish plus 1 second
    
    % Time vector
    t = (0:length(X)-1)/Fs;
    
    % Frequency analysis
    n = length(X);
    Y = fft(X, n);
    Y_0 = fftshift(Y);
    AY_0 = abs(Y_0);
    F_0 = (-n/2:n/2-1)*(Fs/n);  % Frequency vector
    
    % Plotting
    figure('Name', 'Audio Analysis', 'NumberTitle', 'off', 'Position', [100, 100, 800, 800]);
    
    % Time domain plot
    subplot(3,1,1);
    plot(t, X, 'LineWidth', 1.5);
    xlabel('Time (s)'); 
    ylabel('Amplitude');
    title(['Time Domain Plot (', num2str(Fs), ' Hz sampling)']);
    grid on;
    xlim([0, t(end)]);
    
    % Frequency domain plot (linear scale)
    subplot(3,1,2);
    plot(F_0, AY_0, 'LineWidth', 1.5);
    xlabel('Frequency (Hz)'); 
    ylabel('Magnitude');
    title('Frequency Domain Plot (Linear Scale)');
    grid on;
    xlim([-Fs/2, Fs/2]);
    
    % Frequency domain plot (logarithmic scale)
    subplot(3,1,3);
    semilogy(F_0(F_0 >= 0), AY_0(F_0 >= 0), 'LineWidth', 1.5); % Only positive frequencies
    xlabel('Frequency (Hz)'); 
    ylabel('Magnitude (log scale)');
    title('Frequency Domain Plot (Logarithmic Scale)');
    grid on;
    xlim([0, Fs/2]);
    
    % Display additional information
    disp(['Audio length: ', num2str(length(X)/Fs), ' seconds']);
    disp(['Number of samples: ', num2str(length(X))]);
    disp(['Max amplitude: ', num2str(max(abs(X)))]);
    
    % Spectrogram analysis
    figure('Name', 'Spectrogram Analysis', 'NumberTitle', 'off');
    window = hamming(512);
    noverlap = 256;
    nfft = 1024;
    spectrogram(X, window, noverlap, nfft, Fs, 'yaxis');
    title('Spectrogram');
    colorbar;
end
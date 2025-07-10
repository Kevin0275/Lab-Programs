#include <portaudio.h>
#include <iostream>

int main() {
    Pa_Initialize();
    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0) {
        std::cerr << "ERROR: Pa_GetDeviceCount returned " << numDevices << "\n";
        return 1;
    }

    const PaDeviceInfo* info;
    for (int i = 0; i < numDevices; ++i) {
        info = Pa_GetDeviceInfo(i);
        if (info->maxInputChannels > 0) {
            std::cout << "Input Device Index " << i
                      << ": " << info->name
                      << " | Channels: " << info->maxInputChannels << "\n";
        }
    }

    Pa_Terminate();
    return 0;
}

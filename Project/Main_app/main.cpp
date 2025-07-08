#include <portaudio.h>
#include <vector>
#include <cmath>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256

float getMicRMS() {
    PaStream *stream;
    PaStreamParameters inputParameters;
    std::vector<float> buffer(FRAMES_PER_BUFFER);
    float rms = 0;

    if (Pa_Initialize() != paNoError)
        return -1;

    inputParameters.device = 13; // ← your Conexant SmartAudio HD
    if (inputParameters.device == paNoDevice) {
        Pa_Terminate();
        return -1;
    }

    inputParameters.channelCount = 1; // or 2 if you want stereo input
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    if (Pa_OpenStream(&stream, &inputParameters, nullptr,
                      SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff,
                      nullptr, nullptr) != paNoError) {
        Pa_Terminate();
        return -1;
    }

    Pa_StartStream(stream);
    Pa_ReadStream(stream, buffer.data(), FRAMES_PER_BUFFER);
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    float sum = 0;
    for (float s : buffer)
        sum += s * s;

    rms = std::sqrt(sum / buffer.size());
    return rms;
}

 

#include <QCoreApplication>
#include <QSerialPort>
#include <QTextStream>
#include <QTimer>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSerialPort serial;

    serial.setPortName("COM3");           // <-- Replace with your actual COM port
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open port:" << serial.errorString();
        return 1;
    }

    QObject::connect(&serial, &QSerialPort::readyRead, [&]() {
    QByteArray line = serial.readLine();
    float micRMS = getMicRMS();
    QTextStream(stdout) << "Distance: " << line.trimmed()
                        << " | Mic RMS: " << micRMS << "\n";
    });


    return a.exec();
}

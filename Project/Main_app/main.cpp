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
        QTextStream(stdout) << "Distance: " << line;
    });

    return a.exec();
}

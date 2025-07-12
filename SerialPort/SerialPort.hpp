/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
* Updated: 2025-07-12 by F3lda
*/

#ifndef SERIALPORT_H
#define SERIALPORT_H

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

class SerialPort
{
private:
    HANDLE handler;
    bool connected;
    int error;
    COMSTAT status;
    DWORD errors;
public:
    SerialPort(char *portName, DWORD baudRate);
    ~SerialPort();

    int readSerialPort(char *buffer, unsigned int buf_size);
    bool writeSerialPort(char *buffer, unsigned int buf_size);
    void disconnect();
    bool isConnected();
    int getError();
};

#endif // SERIALPORT_H

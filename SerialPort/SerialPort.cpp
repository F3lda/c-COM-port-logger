/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
* Updated: 2025-07-12 by F3lda
*/

#include "SerialPort.hpp"

SerialPort::SerialPort(char *portName, DWORD baudRate = CBR_9600)
{
    this->connected = false;
    this->error = 0;

    this->handler = CreateFileA(static_cast<LPCSTR>(portName),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if (this->handler == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            printf("ERROR: Port %s is not available!\n", portName);
            this->error = -1;
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            printf("ERROR: Access to %s was denied! Maybe used by another process.\n", portName);
            this->error = -2;
        } else {
            printf("ERROR: %lu",GetLastError());
            this->error = GetLastError();
        }
    } else {
        DCB dcbSerialParameters = {0};

        if (!GetCommState(this->handler, &dcbSerialParameters)) {
            printf("failed to get current serial parameters");
        } else {
            dcbSerialParameters.BaudRate = baudRate;
            dcbSerialParameters.ByteSize = 8;
            dcbSerialParameters.StopBits = ONESTOPBIT;
            dcbSerialParameters.Parity = NOPARITY;
            dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

            if (!SetCommState(handler, &dcbSerialParameters)) {
                printf("ALERT: could not set Serial port parameters\n");
            } else {
                this->connected = true;
                PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                Sleep(ARDUINO_WAIT_TIME);
            }
        }
    }
}

SerialPort::~SerialPort()
{
    this->disconnect();
}

int SerialPort::readSerialPort(char *buffer, unsigned int buf_size)
{
    DWORD bytesRead;
    unsigned int toRead = 0;

    ClearCommError(this->handler, &this->errors, &this->status);

    if (this->status.cbInQue > 0) {
        if (this->status.cbInQue > buf_size) {
            toRead = buf_size;
        } else toRead = this->status.cbInQue;
    }

    memset(buffer, 0, buf_size);

    if (ReadFile(this->handler, buffer, toRead, &bytesRead, NULL)) return bytesRead;

    return 0;
}

bool SerialPort::writeSerialPort(char *buffer, unsigned int buf_size)
{
    DWORD bytesSend;

    if (!WriteFile(this->handler, (void*) buffer, buf_size, &bytesSend, 0)) {
        ClearCommError(this->handler, &this->errors, &this->status);
        return false;
    } else return true;
}

void SerialPort::disconnect()
{
	if (this->connected){
        this->connected = false;
        CloseHandle(this->handler);
        this->handler = NULL;
        printf("Serial disconnected.");
    }
}

bool SerialPort::isConnected()
{
    return this->connected;
}

int SerialPort::getError()
{
    return this->error;
}

#include <iostream>
#include "./include/SerialPort.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


// Serial Port number
char portName[] = "\\\\.\\COM8";

// Buffer size for receiving data
#define MAX_DATA_LENGTH 255

// Control signals for turning on and turning off the LED
char ledON[] = "ON\n";
char ledOFF[] = "OFF\n";

// Blinking Delay
#define BLINKING_DELAY_SECS 1

// Arduino SerialPort object
SerialPort *arduino = NULL;



void exampleReceiveData()
{
    char incomingData[MAX_DATA_LENGTH];
    memset(incomingData, 0, sizeof(incomingData));
    unsigned int charsRead = arduino->readSerialPort(incomingData, MAX_DATA_LENGTH);
    if (charsRead > 0) {printf("<%s>(%u)\n", incomingData, charsRead);}
    Sleep(10);
}

void exampleWriteData()
{
    static char state = 0;
    if (state == 0) {
        arduino->writeSerialPort(ledON, strlen(ledON));
    } else {
        arduino->writeSerialPort(ledOFF, strlen(ledOFF));
    }
    state = !state;
}

int main()
{
    arduino = new SerialPort(portName);

    // Checking if Arduino is connected or not
    if (arduino->isConnected()){
        printf("Connection established at port %s\n", portName);
        printf("Press ESCAPE to finish:\n");
    }

    // Loop for receiving data and sending commands
    time_t clk = time(NULL);
    while(arduino->isConnected() && !(GetAsyncKeyState(VK_ESCAPE) & 0x0001)) {
        exampleReceiveData();
        if (time(NULL) - clk >= BLINKING_DELAY_SECS) {
            exampleWriteData();
            clk = time(NULL);
        }
    }

    delete arduino;

    printf("Press ENTER to finish...");
    scanf("%*c");

    return 0;
}
